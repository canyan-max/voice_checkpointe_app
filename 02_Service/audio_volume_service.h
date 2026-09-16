/**
 ******************************************************************************
 *@file               :   audio_volume_service.h
 *@brief              :   Manage the logical 0..100 audio volume state.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef AUDIO_VOLUME_SERVICE_H
#define AUDIO_VOLUME_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"

#define AUDIO_VOLUME_SERVICE_LEVEL_MIN      0U
#define AUDIO_VOLUME_SERVICE_LEVEL_MAX    100U
#define AUDIO_VOLUME_SERVICE_LEVEL_DEFAULT  0U

typedef struct AUDIO_VOLUME_SERVICE_T
{
    uint8_t level;
    uint8_t is_initialized;
} audio_volume_service_t;

platform_err_t audio_volume_service_init(
    audio_volume_service_t *p_service,
    uint32_t                timeout_ms);

platform_err_t audio_volume_service_step(
    audio_volume_service_t *p_service,
    int8_t                  step,
    uint32_t                timeout_ms,
    uint8_t                *p_level);

platform_err_t audio_volume_service_level_get(
    const audio_volume_service_t *p_service,
    uint8_t                      *p_level);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_VOLUME_SERVICE_H */
