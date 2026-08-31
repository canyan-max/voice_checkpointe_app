/**
 ******************************************************************************
 *@file               :   cs4344.h
 *@brief              :   Describe the portable CS4344 stream contract.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef CS4344_H
#define CS4344_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* typedef ------------------------------------------------------------------*/
typedef enum CS4344_RET_T
{
    CS4344_RET_OK = 0U,
    CS4344_RET_PARAM,
    CS4344_RET_UNSUPPORTED
} cs4344_ret_t;

typedef enum CS4344_SERIAL_FORMAT_T
{
    CS4344_SERIAL_FORMAT_I2S = 0U
} cs4344_serial_format_t;

typedef struct CS4344_STREAM_CONFIG_T
{
    uint32_t               sample_rate_hz;
    uint16_t               mclk_lrck_ratio;
    uint8_t                sample_bits;
    uint8_t                channel_count;
    cs4344_serial_format_t serial_format;
} cs4344_stream_config_t;

/* function  ----------------------------------------------------------------*/
/**
  * @brief            : Validate a CS4344 external-SCLK stream contract.
  * @retval           : CS4344_RET_OK, CS4344_RET_PARAM or
  *                     CS4344_RET_UNSUPPORTED.
  * @param[in]        : p_config Stream properties produced by the host.
  * @note             : This function does not start clocks or own a buffer.
  */
cs4344_ret_t cs4344_stream_config_validate(
    const cs4344_stream_config_t *p_config);

#ifdef __cplusplus
}
#endif

#endif /* CS4344_H */
