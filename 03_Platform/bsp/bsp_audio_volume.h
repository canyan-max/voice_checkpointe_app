/**
 ******************************************************************************
 *@file               :   bsp_audio_volume.h
 *@brief              :   Board audio volume control capability.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef BSP_AUDIO_VOLUME_H
#define BSP_AUDIO_VOLUME_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"

/* Board-safe working range; the device's electrical range is wider. */
#define BSP_AUDIO_VOLUME_POSITION_MIN      20U
#define BSP_AUDIO_VOLUME_POSITION_MAX     200U
#define BSP_AUDIO_VOLUME_POSITION_DEFAULT BSP_AUDIO_VOLUME_POSITION_MIN

platform_err_t bsp_audio_volume_init(uint32_t timeout_ms);

platform_err_t bsp_audio_volume_position_set(uint8_t  position,
                                              uint32_t timeout_ms);

platform_err_t bsp_audio_volume_position_get(uint8_t  *p_position,
                                              uint32_t  timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_AUDIO_VOLUME_H */
