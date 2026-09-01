/**
 ******************************************************************************
 *@file               :   tpa3116.c
 *@brief              :   Implement portable TPA3116 GPIO control semantics.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "tpa3116.h"

/* define   -----------------------------------------------------------------*/
#define TPA3116_LEVEL_LOW   0U
#define TPA3116_LEVEL_HIGH  1U

/* private functions --------------------------------------------------------*/
static tpa3116_ret_t tpa3116_device_validate(
    const tpa3116_device_t *p_device)
{
    if(NULL == p_device)
    {
        return TPA3116_RET_PARAM;
    }
    if((0U == p_device->is_initialized) ||
       (NULL == p_device->io.write) ||
       (NULL == p_device->io.read))
    {
        return TPA3116_RET_NOT_READY;
    }
    return TPA3116_RET_OK;
}

/* exported functions -------------------------------------------------------*/
tpa3116_ret_t tpa3116_init(tpa3116_device_t   *p_device,
                           const tpa3116_io_t *p_io)
{
    if((NULL == p_device) || (NULL == p_io) ||
       (NULL == p_io->write) || (NULL == p_io->read))
    {
        return TPA3116_RET_PARAM;
    }

    p_device->io = *p_io;
    p_device->is_initialized = 1U;
    return TPA3116_RET_OK;
}

tpa3116_ret_t tpa3116_enable(tpa3116_device_t *p_device)
{
    tpa3116_ret_t ret;

    ret = tpa3116_device_validate(p_device);
    if(TPA3116_RET_OK != ret)
    {
        return ret;
    }

    ret = p_device->io.write(p_device->io.p_context,
                             TPA3116_PIN_MUTE,
                             TPA3116_LEVEL_HIGH);
    if(TPA3116_RET_OK != ret)
    {
        return TPA3116_RET_IO;
    }
    ret = p_device->io.write(p_device->io.p_context,
                             TPA3116_PIN_SDZ,
                             TPA3116_LEVEL_HIGH);
    return (TPA3116_RET_OK == ret) ? TPA3116_RET_OK : TPA3116_RET_IO;
}

tpa3116_ret_t tpa3116_shutdown(tpa3116_device_t *p_device)
{
    tpa3116_ret_t mute_ret;
    tpa3116_ret_t shutdown_ret;
    tpa3116_ret_t ret;

    ret = tpa3116_device_validate(p_device);
    if(TPA3116_RET_OK != ret)
    {
        return ret;
    }

    mute_ret = p_device->io.write(p_device->io.p_context,
                                  TPA3116_PIN_MUTE,
                                  TPA3116_LEVEL_HIGH);
    shutdown_ret = p_device->io.write(p_device->io.p_context,
                                      TPA3116_PIN_SDZ,
                                      TPA3116_LEVEL_LOW);
    return ((TPA3116_RET_OK == mute_ret) &&
            (TPA3116_RET_OK == shutdown_ret)) ?
           TPA3116_RET_OK : TPA3116_RET_IO;
}

tpa3116_ret_t tpa3116_mute_set(tpa3116_device_t *p_device,
                               uint8_t           is_muted)
{
    tpa3116_ret_t ret;
    uint8_t level;

    ret = tpa3116_device_validate(p_device);
    if(TPA3116_RET_OK != ret)
    {
        return ret;
    }
    if(is_muted > 1U)
    {
        return TPA3116_RET_PARAM;
    }

    level = (0U != is_muted) ? TPA3116_LEVEL_HIGH : TPA3116_LEVEL_LOW;
    ret = p_device->io.write(p_device->io.p_context,
                             TPA3116_PIN_MUTE,
                             level);
    return (TPA3116_RET_OK == ret) ? TPA3116_RET_OK : TPA3116_RET_IO;
}

tpa3116_ret_t tpa3116_fault_get(tpa3116_device_t *p_device,
                                uint8_t          *p_is_fault)
{
    tpa3116_ret_t ret;
    uint8_t level;

    if(NULL == p_is_fault)
    {
        return TPA3116_RET_PARAM;
    }
    ret = tpa3116_device_validate(p_device);
    if(TPA3116_RET_OK != ret)
    {
        return ret;
    }

    ret = p_device->io.read(p_device->io.p_context,
                            TPA3116_PIN_FAULTZ,
                            &level);
    if(TPA3116_RET_OK != ret)
    {
        return TPA3116_RET_IO;
    }
    if(level > TPA3116_LEVEL_HIGH)
    {
        return TPA3116_RET_IO;
    }

    *p_is_fault = (TPA3116_LEVEL_LOW == level) ? 1U : 0U;
    return TPA3116_RET_OK;
}

/* end of file --------------------------------------------------------------*/
