/**
 ******************************************************************************
 *@file               :   bsp_audio_volume.c
 *@brief              :   Bind board I2C2 to the CAT5171 audio volume device.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "board_resources.h"
#include "bsp_audio_volume.h"
#include "cat5171.h"
#include "plat_i2c.h"

#define BSP_AUDIO_VOLUME_I2C_ADDRESS_7BIT  0x2CU
#define BSP_AUDIO_VOLUME_LEVEL_SCALE       100U

static cat5171_device_t audio_volume_device;

_Static_assert(BSP_AUDIO_VOLUME_POSITION_MAX >
                   BSP_AUDIO_VOLUME_POSITION_MIN,
               "Audio volume position range is invalid");
_Static_assert(BSP_AUDIO_VOLUME_LEVEL_MAX ==
                   BSP_AUDIO_VOLUME_LEVEL_SCALE,
               "Audio volume level scale mismatch");

static uint8_t bsp_audio_volume_level_to_position(uint8_t level)
{
    uint32_t position_range;
    uint32_t scaled_position;

    position_range = (uint32_t)BSP_AUDIO_VOLUME_POSITION_MAX -
                     BSP_AUDIO_VOLUME_POSITION_MIN;
    scaled_position = ((uint32_t)level * position_range) +
                      (BSP_AUDIO_VOLUME_LEVEL_SCALE / 2U);
    return (uint8_t)(BSP_AUDIO_VOLUME_POSITION_MIN +
                     (scaled_position / BSP_AUDIO_VOLUME_LEVEL_SCALE));
}

static platform_err_t bsp_audio_volume_position_to_level(
    uint8_t  position,
    uint8_t *p_level)
{
    uint32_t position_range;
    uint32_t scaled_level;

    if((NULL == p_level) ||
       (position < BSP_AUDIO_VOLUME_POSITION_MIN) ||
       (position > BSP_AUDIO_VOLUME_POSITION_MAX))
    {
        return PLATFORM_ERR_HW;
    }

    position_range = (uint32_t)BSP_AUDIO_VOLUME_POSITION_MAX -
                     BSP_AUDIO_VOLUME_POSITION_MIN;
    scaled_level = ((uint32_t)(position - BSP_AUDIO_VOLUME_POSITION_MIN) *
                    BSP_AUDIO_VOLUME_LEVEL_SCALE) +
                   (position_range / 2U);
    *p_level = (uint8_t)(scaled_level / position_range);
    return PLATFORM_ERR_OK;
}

static cat5171_ret_t bsp_audio_volume_ret_from_platform(platform_err_t ret)
{
    switch(ret)
    {
        case PLATFORM_ERR_OK:
            return CAT5171_RET_OK;
        case PLATFORM_ERR_PARAM:
            return CAT5171_RET_PARAM;
        case PLATFORM_ERR_BUSY:
            return CAT5171_RET_BUSY;
        case PLATFORM_ERR_TIMEOUT:
            return CAT5171_RET_TIMEOUT;
        default:
            return CAT5171_RET_IO;
    }
}

static platform_err_t bsp_audio_volume_ret_to_platform(cat5171_ret_t ret)
{
    switch(ret)
    {
        case CAT5171_RET_OK:
            return PLATFORM_ERR_OK;
        case CAT5171_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case CAT5171_RET_BUSY:
            return PLATFORM_ERR_BUSY;
        case CAT5171_RET_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;
        default:
            return PLATFORM_ERR_HW;
    }
}

static cat5171_ret_t bsp_audio_volume_i2c_write(void          *p_context,
                                                const uint8_t *p_data,
                                                uint16_t       size,
                                                uint32_t       timeout_ms)
{
    (void)p_context;
    return bsp_audio_volume_ret_from_platform(
        plat_i2c_write(BOARD_I2C_SHARED,
                       BSP_AUDIO_VOLUME_I2C_ADDRESS_7BIT,
                       p_data,
                       size,
                       timeout_ms));
}

static cat5171_ret_t bsp_audio_volume_i2c_read(void     *p_context,
                                               uint8_t  *p_data,
                                               uint16_t  size,
                                               uint32_t  timeout_ms)
{
    (void)p_context;
    return bsp_audio_volume_ret_from_platform(
        plat_i2c_read(BOARD_I2C_SHARED,
                      BSP_AUDIO_VOLUME_I2C_ADDRESS_7BIT,
                      p_data,
                      size,
                      timeout_ms));
}

platform_err_t bsp_audio_volume_init(uint32_t timeout_ms)
{
    static const cat5171_io_t io =
    {
        bsp_audio_volume_i2c_write,
        bsp_audio_volume_i2c_read,
        NULL
    };
    cat5171_ret_t ret;
    uint8_t readback;

    if(0U == timeout_ms)
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = cat5171_init(&audio_volume_device, &io);
    if(CAT5171_RET_OK == ret)
    {
        ret = cat5171_wiper_write(&audio_volume_device,
                                  BSP_AUDIO_VOLUME_POSITION_DEFAULT,
                                  timeout_ms);
    }
    if(CAT5171_RET_OK == ret)
    {
        ret = cat5171_wiper_read(&audio_volume_device,
                                 &readback,
                                 timeout_ms);
    }
    if((CAT5171_RET_OK == ret) &&
       (BSP_AUDIO_VOLUME_POSITION_DEFAULT != readback))
    {
        ret = CAT5171_RET_IO;
    }
    return bsp_audio_volume_ret_to_platform(ret);
}

platform_err_t bsp_audio_volume_position_set(uint8_t  position,
                                              uint32_t timeout_ms)
{
    if((position < BSP_AUDIO_VOLUME_POSITION_MIN) ||
       (position > BSP_AUDIO_VOLUME_POSITION_MAX) ||
       (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_audio_volume_ret_to_platform(
        cat5171_wiper_write(&audio_volume_device, position, timeout_ms));
}

platform_err_t bsp_audio_volume_position_get(uint8_t  *p_position,
                                              uint32_t  timeout_ms)
{
    if((NULL == p_position) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_audio_volume_ret_to_platform(
        cat5171_wiper_read(&audio_volume_device, p_position, timeout_ms));
}

platform_err_t bsp_audio_volume_level_set(uint8_t  level,
                                           uint32_t timeout_ms)
{
    if((level > BSP_AUDIO_VOLUME_LEVEL_MAX) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_audio_volume_position_set(
        bsp_audio_volume_level_to_position(level),
        timeout_ms);
}

platform_err_t bsp_audio_volume_level_get(uint8_t  *p_level,
                                           uint32_t  timeout_ms)
{
    platform_err_t ret;
    uint8_t position;

    if((NULL == p_level) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = bsp_audio_volume_position_get(&position, timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    return bsp_audio_volume_position_to_level(position, p_level);
}
