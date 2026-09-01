/**
 ******************************************************************************
 *@file               :   at32f435_i2s.c
 *@brief              :   Provide the AT32F435 I2S transmit implementation.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "plat_i2s.h"
#include "wk_edma.h"
#include "wk_i2s.h"

/* define   -----------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef struct AT32F435_I2S_CFG_T
{
    spi_type         *p_i2s;
    edma_stream_type *p_tx_dma;
    uint32_t          dma_half_flag;
    uint32_t          dma_full_flag;
    uint32_t          dma_error_flag;
} at32f435_i2s_cfg_t;

/* variables ----------------------------------------------------------------*/
static const at32f435_i2s_cfg_t i2s_cfg[BOARD_I2S_RESOURCE_NUM] =
{
    [BOARD_I2S_AUDIO_OUTPUT] =
    {
        BOARD_I2S_AUDIO_OUTPUT_INSTANCE,
        BOARD_I2S_AUDIO_OUTPUT_TX_DMA,
        BOARD_I2S_AUDIO_OUTPUT_TX_DMA_HALF,
        BOARD_I2S_AUDIO_OUTPUT_TX_DMA_FULL,
        BOARD_I2S_AUDIO_OUTPUT_TX_DMA_ERROR
    }
};
static plat_i2s_cb_t i2s_callback[BOARD_I2S_RESOURCE_NUM];
static uint8_t       i2s_is_started[BOARD_I2S_RESOURCE_NUM];

_Static_assert((sizeof(i2s_cfg) / sizeof(i2s_cfg[0])) ==
                   BOARD_I2S_RESOURCE_NUM,
               "I2S resource table size mismatch");

/* Private  functions  ------------------------------------------------------*/
static platform_err_t
at32f435_i2s_rate_get(uint32_t                      sample_rate_hz,
                      i2s_audio_sampling_freq_type *p_rate)
{
    if(NULL == p_rate)
    {
        return PLATFORM_ERR_PARAM;
    }

    switch(sample_rate_hz)
    {
        case 8000U:
            *p_rate = I2S_AUDIO_FREQUENCY_8K;
            break;
        case 11025U:
            *p_rate = I2S_AUDIO_FREQUENCY_11_025K;
            break;
        case 16000U:
            *p_rate = I2S_AUDIO_FREQUENCY_16K;
            break;
        case 22050U:
            *p_rate = I2S_AUDIO_FREQUENCY_22_05K;
            break;
        case 32000U:
            *p_rate = I2S_AUDIO_FREQUENCY_32K;
            break;
        case 44100U:
            *p_rate = I2S_AUDIO_FREQUENCY_44_1K;
            break;
        case 48000U:
            *p_rate = I2S_AUDIO_FREQUENCY_48K;
            break;
        case 96000U:
            *p_rate = I2S_AUDIO_FREQUENCY_96K;
            break;
        case 192000U:
            *p_rate = I2S_AUDIO_FREQUENCY_192K;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static platform_err_t
at32f435_i2s_protocol_get(plat_i2s_protocol_t       protocol,
                         i2s_audio_protocol_type   *p_protocol)
{
    if(NULL == p_protocol)
    {
        return PLATFORM_ERR_PARAM;
    }
    switch(protocol)
    {
        case PLAT_I2S_PROTOCOL_I2S:
            *p_protocol = I2S_AUDIO_PROTOCOL_PHILLIPS;
            break;
        case PLAT_I2S_PROTOCOL_MSB_JUSTIFIED:
            *p_protocol = I2S_AUDIO_PROTOCOL_MSB;
            break;
        case PLAT_I2S_PROTOCOL_LSB_JUSTIFIED:
            *p_protocol = I2S_AUDIO_PROTOCOL_LSB;
            break;
        case PLAT_I2S_PROTOCOL_PCM_SHORT:
            *p_protocol = I2S_AUDIO_PROTOCOL_PCM_SHORT;
            break;
        case PLAT_I2S_PROTOCOL_PCM_LONG:
            *p_protocol = I2S_AUDIO_PROTOCOL_PCM_LONG;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static platform_err_t
at32f435_i2s_data_format_get(plat_i2s_data_format_t          data_format,
                            i2s_data_channel_format_type    *p_data_format)
{
    if(NULL == p_data_format)
    {
        return PLATFORM_ERR_PARAM;
    }
    switch(data_format)
    {
        case PLAT_I2S_DATA_16BIT_CHANNEL_16BIT:
            *p_data_format = I2S_DATA_16BIT_CHANNEL_16BIT;
            break;
        case PLAT_I2S_DATA_16BIT_CHANNEL_32BIT:
            *p_data_format = I2S_DATA_16BIT_CHANNEL_32BIT;
            break;
        case PLAT_I2S_DATA_24BIT_CHANNEL_32BIT:
            *p_data_format = I2S_DATA_24BIT_CHANNEL_32BIT;
            break;
        case PLAT_I2S_DATA_32BIT_CHANNEL_32BIT:
            *p_data_format = I2S_DATA_32BIT_CHANNEL_32BIT;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static platform_err_t
at32f435_i2s_clock_polarity_get(plat_i2s_clock_polarity_t  clock_polarity,
                                i2s_clock_polarity_type    *p_clock_polarity)
{
    if(NULL == p_clock_polarity)
    {
        return PLATFORM_ERR_PARAM;
    }
    switch(clock_polarity)
    {
        case PLAT_I2S_CLOCK_POLARITY_LOW:
            *p_clock_polarity = I2S_CLOCK_POLARITY_LOW;
            break;
        case PLAT_I2S_CLOCK_POLARITY_HIGH:
            *p_clock_polarity = I2S_CLOCK_POLARITY_HIGH;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

/* Exported functions -------------------------------------------------------*/
platform_err_t plat_i2s_configure(plat_i2s_id_t            id,
                                  const plat_i2s_config_t *p_config)
{
    i2s_init_type                 init;
    i2s_audio_sampling_freq_type  rate;
    i2s_audio_protocol_type       protocol;
    i2s_data_channel_format_type  data_format;
    i2s_clock_polarity_type       clock_polarity;

    if((id >= BOARD_I2S_RESOURCE_NUM) || (NULL == p_config))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != i2s_is_started[id])
    {
        return PLATFORM_ERR_BUSY;
    }
    if((PLATFORM_ERR_OK !=
        at32f435_i2s_rate_get(p_config->sample_rate_hz, &rate)) ||
       (PLATFORM_ERR_OK !=
        at32f435_i2s_protocol_get(p_config->protocol, &protocol)) ||
       (PLATFORM_ERR_OK !=
        at32f435_i2s_data_format_get(p_config->data_format, &data_format)) ||
       (PLATFORM_ERR_OK != at32f435_i2s_clock_polarity_get(
                               p_config->clock_polarity,
                               &clock_polarity)))
    {
        return PLATFORM_ERR_PARAM;
    }

    i2s_enable(i2s_cfg[id].p_i2s, FALSE);
    i2s_default_para_init(&init);
    init.operation_mode      = I2S_MODE_MASTER_TX;
    init.audio_protocol      = protocol;
    init.data_channel_format = data_format;
    init.audio_sampling_freq = rate;
    init.clock_polarity      = clock_polarity;
    init.mclk_output_enable  = (0U != p_config->mclk_output_enable) ? TRUE : FALSE;
    i2s_init(i2s_cfg[id].p_i2s, &init);
    spi_i2s_dma_transmitter_enable(i2s_cfg[id].p_i2s, TRUE);
    return PLATFORM_ERR_OK;
}

platform_err_t
plat_i2s_start(plat_i2s_id_t id, uint16_t *p_samples, uint16_t sample_count)
{
    const at32f435_i2s_cfg_t *p_cfg;

    if((id >= BOARD_I2S_RESOURCE_NUM) || (NULL == p_samples) ||
       (0U == sample_count) ||
       (0U != (sample_count & 1U)))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != i2s_is_started[id])
    {
        return PLATFORM_ERR_BUSY;
    }

    p_cfg = &i2s_cfg[id];
    edma_stream_enable(p_cfg->p_tx_dma, FALSE);
    edma_flag_clear(p_cfg->dma_half_flag | p_cfg->dma_full_flag |
                    p_cfg->dma_error_flag);
    wk_edma_stream_config(p_cfg->p_tx_dma, (uint32_t)&p_cfg->p_i2s->dt,
                          (uint32_t)p_samples, sample_count);
    p_cfg->p_tx_dma->ctrl_bit.lm = TRUE;
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_HDT_INT, TRUE);
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_FDT_INT, TRUE);
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_DTERR_INT, TRUE);
    i2s_is_started[id] = 1U;
    edma_stream_enable(p_cfg->p_tx_dma, TRUE);
    i2s_enable(p_cfg->p_i2s, TRUE);
    return PLATFORM_ERR_OK;
}

platform_err_t plat_i2s_stop(plat_i2s_id_t id)
{
    const at32f435_i2s_cfg_t *p_cfg;

    if(id >= BOARD_I2S_RESOURCE_NUM)
    {
        return PLATFORM_ERR_PARAM;
    }
    p_cfg = &i2s_cfg[id];
    edma_stream_enable(p_cfg->p_tx_dma, FALSE);
    i2s_enable(p_cfg->p_i2s, FALSE);
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_HDT_INT, FALSE);
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_FDT_INT, FALSE);
    edma_interrupt_enable(p_cfg->p_tx_dma, EDMA_DTERR_INT, FALSE);
    edma_flag_clear(p_cfg->dma_half_flag | p_cfg->dma_full_flag |
                    p_cfg->dma_error_flag);
    i2s_is_started[id] = 0U;
    return PLATFORM_ERR_OK;
}

platform_err_t plat_i2s_set_callback(plat_i2s_id_t id, plat_i2s_cb_t callback)
{
    if(id >= BOARD_I2S_RESOURCE_NUM)
    {
        return PLATFORM_ERR_PARAM;
    }
    i2s_callback[id] = callback;
    return PLATFORM_ERR_OK;
}

void plat_i2s_tx_dma_irq_handler(plat_i2s_id_t id)
{
    const at32f435_i2s_cfg_t *p_cfg;
    plat_i2s_event_t          event = PLAT_I2S_EVENT_NONE;

    if(id >= BOARD_I2S_RESOURCE_NUM)
    {
        return;
    }
    p_cfg = &i2s_cfg[id];
    if(RESET != edma_interrupt_flag_get(p_cfg->dma_half_flag))
    {
        edma_flag_clear(p_cfg->dma_half_flag);
        event = (plat_i2s_event_t)(event | PLAT_I2S_EVENT_TX_HALF);
    }
    if(RESET != edma_interrupt_flag_get(p_cfg->dma_full_flag))
    {
        edma_flag_clear(p_cfg->dma_full_flag);
        event = (plat_i2s_event_t)(event | PLAT_I2S_EVENT_TX_COMPLETE);
    }
    if(RESET != edma_interrupt_flag_get(p_cfg->dma_error_flag))
    {
        edma_flag_clear(p_cfg->dma_error_flag);
        event = (plat_i2s_event_t)(event | PLAT_I2S_EVENT_TX_ERROR);
    }
    if((PLAT_I2S_EVENT_NONE != event) && (NULL != i2s_callback[id]))
    {
        i2s_callback[id](id, event);
    }
}

/* end of file --------------------------------------------------------------*/
