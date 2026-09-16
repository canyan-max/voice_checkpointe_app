/**
 ******************************************************************************
 *@file               :   audio_volume_service.c
 *@brief              :   Apply and verify logical audio volume changes.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "audio_volume_service.h"
#include "bsp_audio_volume.h"

_Static_assert(AUDIO_VOLUME_SERVICE_LEVEL_MIN ==
                   BSP_AUDIO_VOLUME_LEVEL_MIN,
               "Audio volume minimum mismatch");
_Static_assert(AUDIO_VOLUME_SERVICE_LEVEL_MAX ==
                   BSP_AUDIO_VOLUME_LEVEL_MAX,
               "Audio volume maximum mismatch");

platform_err_t audio_volume_service_init(
    audio_volume_service_t *p_service,
    uint32_t                timeout_ms)
{
    platform_err_t ret;
    uint8_t level = AUDIO_VOLUME_SERVICE_LEVEL_DEFAULT;

    if((NULL == p_service) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_service->level = AUDIO_VOLUME_SERVICE_LEVEL_DEFAULT;
    p_service->is_initialized = 0U;
    ret = bsp_audio_volume_init(timeout_ms);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_audio_volume_level_get(&level, timeout_ms);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        p_service->level = level;
        p_service->is_initialized = 1U;
    }
    return ret;
}

platform_err_t audio_volume_service_step(
    audio_volume_service_t *p_service,
    int8_t                  step,
    uint32_t                timeout_ms,
    uint8_t                *p_level)
{
    platform_err_t ret;
    int16_t requested_level;
    uint8_t readback;

    if((NULL == p_service) || (NULL == p_level) ||
       (0 == step) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == p_service->is_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    requested_level = (int16_t)p_service->level + step;
    if(requested_level < (int16_t)AUDIO_VOLUME_SERVICE_LEVEL_MIN)
    {
        requested_level = (int16_t)AUDIO_VOLUME_SERVICE_LEVEL_MIN;
    }
    else if(requested_level > (int16_t)AUDIO_VOLUME_SERVICE_LEVEL_MAX)
    {
        requested_level = (int16_t)AUDIO_VOLUME_SERVICE_LEVEL_MAX;
    }

    if((uint8_t)requested_level == p_service->level)
    {
        *p_level = p_service->level;
        return PLATFORM_ERR_OK;
    }

    ret = bsp_audio_volume_level_set((uint8_t)requested_level, timeout_ms);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_audio_volume_level_get(&readback, timeout_ms);
    }
    if((PLATFORM_ERR_OK == ret) &&
       (readback != (uint8_t)requested_level))
    {
        ret = PLATFORM_ERR_HW;
    }
    if(PLATFORM_ERR_OK == ret)
    {
        p_service->level = readback;
        *p_level = readback;
    }
    else
    {
        p_service->is_initialized = 0U;
    }
    return ret;
}

platform_err_t audio_volume_service_level_get(
    const audio_volume_service_t *p_service,
    uint8_t                      *p_level)
{
    if((NULL == p_service) || (NULL == p_level))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == p_service->is_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    *p_level = p_service->level;
    return PLATFORM_ERR_OK;
}
