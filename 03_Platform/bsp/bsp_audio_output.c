/**
 ******************************************************************************
 *@file               :   bsp_audio_output.c
 *@brief              :   Bind the board audio output to CS4344 and MCU I2S.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_resources.h"
#include "bsp_audio_output.h"
#include "cs4344.h"
#include "plat_i2s.h"

/* define   -----------------------------------------------------------------*/
#define BSP_AUDIO_OUTPUT_MCLK_LRCK_RATIO    256U

/* variables ----------------------------------------------------------------*/
static bsp_audio_output_cb_t audio_output_callback;
static uint8_t audio_output_is_started;

/* Private  functions  ------------------------------------------------------*/
static void bsp_audio_output_i2s_callback(plat_i2s_id_t    id,
                                          plat_i2s_event_t event)
{
    bsp_audio_output_event_t output_event = BSP_AUDIO_OUTPUT_EVENT_NONE;

    (void)id;
    if(0U != ((uint32_t)event & (uint32_t)PLAT_I2S_EVENT_TX_HALF))
    {
        output_event = (bsp_audio_output_event_t)(
            output_event | BSP_AUDIO_OUTPUT_EVENT_FIRST_HALF_WRITABLE);
    }
    if(0U != ((uint32_t)event & (uint32_t)PLAT_I2S_EVENT_TX_COMPLETE))
    {
        output_event = (bsp_audio_output_event_t)(
            output_event | BSP_AUDIO_OUTPUT_EVENT_SECOND_HALF_WRITABLE);
    }
    if(0U != ((uint32_t)event & (uint32_t)PLAT_I2S_EVENT_TX_ERROR))
    {
        output_event = (bsp_audio_output_event_t)(
            output_event | BSP_AUDIO_OUTPUT_EVENT_ERROR);
    }
    if((BSP_AUDIO_OUTPUT_EVENT_NONE != output_event) &&
       (NULL != audio_output_callback))
    {
        audio_output_callback(output_event);
    }
}

/* Exported functions -------------------------------------------------------*/
platform_err_t bsp_audio_output_set_callback(bsp_audio_output_cb_t callback)
{
    if(0U != audio_output_is_started)
    {
        return PLATFORM_ERR_BUSY;
    }
    audio_output_callback = callback;
    return PLATFORM_ERR_OK;
}

platform_err_t bsp_audio_output_stream_start(uint32_t  sample_rate_hz,
                                             uint16_t *p_samples,
                                             uint16_t  sample_count)
{
    const cs4344_stream_config_t device_config = {
        sample_rate_hz,
        BSP_AUDIO_OUTPUT_MCLK_LRCK_RATIO,
        16U,
        2U,
        CS4344_SERIAL_FORMAT_I2S};
    const plat_i2s_config_t i2s_config = {
        sample_rate_hz,
        PLAT_I2S_PROTOCOL_I2S,
        PLAT_I2S_DATA_16BIT_CHANNEL_16BIT,
        PLAT_I2S_CLOCK_POLARITY_LOW,
        1U};
    platform_err_t ret;

    if((NULL == p_samples) || (0U == sample_count) ||
       (0U != (sample_count & 1U)))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != audio_output_is_started)
    {
        return PLATFORM_ERR_BUSY;
    }
    if(CS4344_RET_OK != cs4344_stream_config_validate(&device_config))
    {
        return PLATFORM_ERR_PARAM;
    }
    ret = plat_i2s_set_callback(BOARD_I2S_AUDIO_OUTPUT,
                                bsp_audio_output_i2s_callback);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = plat_i2s_configure(BOARD_I2S_AUDIO_OUTPUT, &i2s_config);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = plat_i2s_start(BOARD_I2S_AUDIO_OUTPUT,
                             p_samples,
                             sample_count);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        audio_output_is_started = 1U;
    }
    return ret;
}

platform_err_t bsp_audio_output_stream_stop(void)
{
    platform_err_t ret;

    ret = plat_i2s_stop(BOARD_I2S_AUDIO_OUTPUT);
    audio_output_is_started = 0U;
    return ret;
}

/* end of file --------------------------------------------------------------*/
