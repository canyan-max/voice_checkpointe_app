/**
 ******************************************************************************
 *@file               :   cat5171.c
 *@brief              :   Implement CAT5171 wiper register access.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "cat5171.h"

#define CAT5171_INSTRUCTION_WIPER_WRITE  0x00U

static cat5171_ret_t cat5171_validate(const cat5171_device_t *p_device)
{
    if(NULL == p_device)
    {
        return CAT5171_RET_PARAM;
    }
    if((0U == p_device->is_initialized) ||
       (NULL == p_device->io.write) ||
       (NULL == p_device->io.read))
    {
        return CAT5171_RET_NOT_READY;
    }
    return CAT5171_RET_OK;
}

cat5171_ret_t cat5171_init(cat5171_device_t   *p_device,
                           const cat5171_io_t *p_io)
{
    if((NULL == p_device) || (NULL == p_io) ||
       (NULL == p_io->write) || (NULL == p_io->read))
    {
        return CAT5171_RET_PARAM;
    }

    p_device->io = *p_io;
    p_device->is_initialized = 1U;
    return CAT5171_RET_OK;
}

cat5171_ret_t cat5171_wiper_write(cat5171_device_t *p_device,
                                  uint8_t           position,
                                  uint32_t          timeout_ms)
{
    uint8_t frame[2];
    cat5171_ret_t ret;

    ret = cat5171_validate(p_device);
    if(CAT5171_RET_OK != ret)
    {
        return ret;
    }
    if(0U == timeout_ms)
    {
        return CAT5171_RET_PARAM;
    }

    frame[0] = CAT5171_INSTRUCTION_WIPER_WRITE;
    frame[1] = position;
    return p_device->io.write(p_device->io.p_context,
                              frame,
                              (uint16_t)sizeof(frame),
                              timeout_ms);
}

cat5171_ret_t cat5171_wiper_read(cat5171_device_t *p_device,
                                 uint8_t          *p_position,
                                 uint32_t          timeout_ms)
{
    cat5171_ret_t ret;

    if(NULL == p_position)
    {
        return CAT5171_RET_PARAM;
    }
    ret = cat5171_validate(p_device);
    if(CAT5171_RET_OK != ret)
    {
        return ret;
    }
    if(0U == timeout_ms)
    {
        return CAT5171_RET_PARAM;
    }

    return p_device->io.read(p_device->io.p_context,
                             p_position,
                             1U,
                             timeout_ms);
}
