/**
 ******************************************************************************
 *@file               :   bsp_audio_output.h
 *@brief              :   Provide the fixed board audio output capability.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BSP_AUDIO_OUTPUT_H
#define BSP_AUDIO_OUTPUT_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"

/* typedef ------------------------------------------------------------------*/
typedef enum BSP_AUDIO_OUTPUT_EVENT_T
{
    BSP_AUDIO_OUTPUT_EVENT_NONE                 = 0U,
    BSP_AUDIO_OUTPUT_EVENT_FIRST_HALF_WRITABLE  = (1U << 0),
    BSP_AUDIO_OUTPUT_EVENT_SECOND_HALF_WRITABLE = (1U << 1),
    BSP_AUDIO_OUTPUT_EVENT_ERROR                = (1U << 2)
} bsp_audio_output_event_t;

typedef void (*bsp_audio_output_cb_t)(bsp_audio_output_event_t event);

/* function  ----------------------------------------------------------------*/
/**
  * @brief            : Set the output event callback while stopped.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_BUSY.
  * @param[in]        : callback Callback invoked from the I2S DMA ISR;
  *                     NULL removes the callback.
  */
platform_err_t bsp_audio_output_set_callback(bsp_audio_output_cb_t callback);

/**
  * @brief            : Start the board's 16-bit interleaved stereo output.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM,
  *                     PLATFORM_ERR_BUSY or PLATFORM_ERR_HW.
  * @param[in]        : sample_rate_hz PCM sample rate.
  * @param[in]        : p_samples Caller-owned interleaved stereo buffer.
  * @param[in]        : sample_count Total number of 16-bit channel samples.
  * @note             : The buffer must stay valid and writable until
  *                     bsp_audio_output_stream_stop().
  */
platform_err_t bsp_audio_output_stream_start(uint32_t  sample_rate_hz,
                                             uint16_t *p_samples,
                                             uint16_t  sample_count);

platform_err_t bsp_audio_output_stream_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_AUDIO_OUTPUT_H */
