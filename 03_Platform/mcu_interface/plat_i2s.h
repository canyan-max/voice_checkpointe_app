/**
 ******************************************************************************
 *@file               :   plat_i2s.h
 *@brief              :   Provide logical I2S transmit APIs.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef PLAT_I2S_H
#define PLAT_I2S_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"
#include "plat_resource.h"

/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef enum PLAT_I2S_EVENT_T
{
    PLAT_I2S_EVENT_NONE        = 0U,
    PLAT_I2S_EVENT_TX_HALF     = (1U << 0),
    PLAT_I2S_EVENT_TX_COMPLETE = (1U << 1),
    PLAT_I2S_EVENT_TX_ERROR    = (1U << 2)
} plat_i2s_event_t;

typedef enum PLAT_I2S_PROTOCOL_T
{
    PLAT_I2S_PROTOCOL_I2S = 0U,
    PLAT_I2S_PROTOCOL_MSB_JUSTIFIED,
    PLAT_I2S_PROTOCOL_LSB_JUSTIFIED,
    PLAT_I2S_PROTOCOL_PCM_SHORT,
    PLAT_I2S_PROTOCOL_PCM_LONG
} plat_i2s_protocol_t;

typedef enum PLAT_I2S_DATA_FORMAT_T
{
    PLAT_I2S_DATA_16BIT_CHANNEL_16BIT = 0U,
    PLAT_I2S_DATA_16BIT_CHANNEL_32BIT,
    PLAT_I2S_DATA_24BIT_CHANNEL_32BIT,
    PLAT_I2S_DATA_32BIT_CHANNEL_32BIT
} plat_i2s_data_format_t;

typedef enum PLAT_I2S_CLOCK_POLARITY_T
{
    PLAT_I2S_CLOCK_POLARITY_LOW = 0U,
    PLAT_I2S_CLOCK_POLARITY_HIGH
} plat_i2s_clock_polarity_t;

typedef struct PLAT_I2S_CONFIG_T
{
    uint32_t                  sample_rate_hz;
    plat_i2s_protocol_t       protocol;
    plat_i2s_data_format_t    data_format;
    plat_i2s_clock_polarity_t clock_polarity;
    uint8_t                   mclk_output_enable;
} plat_i2s_config_t;

typedef void (*plat_i2s_cb_t)(plat_i2s_id_t id, plat_i2s_event_t event);

/* variables ----------------------------------------------------------------*/

/* function  ----------------------------------------------------------------*/
platform_err_t plat_i2s_configure(plat_i2s_id_t            id,
                                  const plat_i2s_config_t *p_config);
platform_err_t plat_i2s_start(plat_i2s_id_t id,
                              uint16_t     *p_samples,
                              uint16_t      sample_count);
platform_err_t plat_i2s_stop(plat_i2s_id_t id);
platform_err_t plat_i2s_set_callback(plat_i2s_id_t id, plat_i2s_cb_t callback);
void plat_i2s_tx_dma_irq_handler(plat_i2s_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_I2S_H */
