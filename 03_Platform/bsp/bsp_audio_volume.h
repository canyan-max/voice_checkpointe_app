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
#define BSP_AUDIO_VOLUME_POSITION_MIN       (5U)
#define BSP_AUDIO_VOLUME_POSITION_MAX       (245U)
#define BSP_AUDIO_VOLUME_POSITION_DEFAULT BSP_AUDIO_VOLUME_POSITION_MIN
#define BSP_AUDIO_VOLUME_LEVEL_MIN           (0U)
#define BSP_AUDIO_VOLUME_LEVEL_MAX           (100U)
#define BSP_AUDIO_VOLUME_LEVEL_DEFAULT       BSP_AUDIO_VOLUME_LEVEL_MIN

platform_err_t bsp_audio_volume_init(uint32_t timeout_ms);

platform_err_t bsp_audio_volume_level_set(uint8_t  level,
                                           uint32_t timeout_ms);

platform_err_t bsp_audio_volume_level_get(uint8_t  *p_level,
                                           uint32_t  timeout_ms);

/* Raw CAT5171 position access retained for board-level diagnostics. */
platform_err_t bsp_audio_volume_position_set(uint8_t  position,
                                              uint32_t timeout_ms);

platform_err_t bsp_audio_volume_position_get(uint8_t  *p_position,
                                              uint32_t  timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_AUDIO_VOLUME_H */
