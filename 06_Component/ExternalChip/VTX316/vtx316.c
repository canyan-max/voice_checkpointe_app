/**
 ******************************************************************************
 *@file               :   vtx316.c
 *@brief              :   Implement VTX316 UART command framing.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "vtx316.h"

/* define -------------------------------------------------------------------*/
#define VTX316_FRAME_HEAD        0xFDU
#define VTX316_COMMAND_SPEAK     0x01U
#define VTX316_COMMAND_STOP      0x02U
#define VTX316_COMMAND_SIZE      1U
#define VTX316_ENCODING_SIZE     1U
#define VTX316_CONTROL_HEAD_SIZE 4U
#define VTX316_SPEAK_HEAD_SIZE   5U
#define VTX316_COMMAND_NONE      0x00U

#define VTX316_RESPONSE_ACCEPTED    0x41U
#define VTX316_RESPONSE_SLEEPING    0x4BU
#define VTX316_RESPONSE_SPEAKING    0x4EU
#define VTX316_RESPONSE_IDLE        0x4FU
#define VTX316_RESPONSE_INITIALIZED 0x4AU
#define VTX316_RESPONSE_REJECTED    0x45U

/* private functions --------------------------------------------------------*/
static vtx316_ret_t vtx316_device_validate(
    const vtx316_device_t *p_device)
{
    if(NULL == p_device)
    {
        return VTX316_RET_PARAM;
    }
    if((0U == p_device->is_initialized) ||
       (NULL == p_device->io.transmit) ||
       (NULL == p_device->io.busy_read))
    {
        return VTX316_RET_NOT_READY;
    }
    return VTX316_RET_OK;
}

static uint8_t vtx316_encoding_is_valid(vtx316_encoding_t encoding)
{
    return (uint8_t)((VTX316_ENCODING_GB2312 == encoding) ||
                     (VTX316_ENCODING_GBK == encoding) ||
                     (VTX316_ENCODING_UNICODE_LITTLE == encoding) ||
                     (VTX316_ENCODING_UNICODE_BIG == encoding) ||
                     (VTX316_ENCODING_UTF8 == encoding));
}

static vtx316_ret_t vtx316_control_send(vtx316_device_t *p_device,
                                        uint8_t          command,
                                        uint32_t         timeout_ms)
{
    uint8_t frame[VTX316_CONTROL_HEAD_SIZE];
    vtx316_ret_t ret;

    ret = vtx316_device_validate(p_device);
    if(VTX316_RET_OK != ret)
    {
        return ret;
    }
    if(0U == timeout_ms)
    {
        return VTX316_RET_PARAM;
    }

    frame[0] = VTX316_FRAME_HEAD;
    frame[1] = 0U;
    frame[2] = VTX316_COMMAND_SIZE;
    frame[3] = command;
    return p_device->io.transmit(p_device->io.p_context,
                                 frame,
                                 (uint16_t)sizeof(frame),
                                 NULL,
                                 0U,
                                 timeout_ms);
}

/* exported functions -------------------------------------------------------*/
vtx316_ret_t vtx316_init(vtx316_device_t *p_device, const vtx316_io_t *p_io)
{
    if((NULL == p_device) || (NULL == p_io) ||
       (NULL == p_io->transmit) || (NULL == p_io->busy_read))
    {
        return VTX316_RET_PARAM;
    }

    p_device->io = *p_io;
    p_device->state = VTX316_STATE_UNKNOWN;
    p_device->active_command = VTX316_COMMAND_NONE;
    p_device->is_initialized = 1U;
    return VTX316_RET_OK;
}

vtx316_ret_t vtx316_speak(vtx316_device_t  *p_device,
                          vtx316_encoding_t  encoding,
                          const uint8_t      *p_text,
                          uint16_t            text_size,
                          uint32_t            timeout_ms)
{
    uint8_t frame_head[VTX316_SPEAK_HEAD_SIZE];
    uint16_t data_size;
    uint16_t text_limit;
    vtx316_ret_t ret;

    ret = vtx316_device_validate(p_device);
    if(VTX316_RET_OK != ret)
    {
        return ret;
    }
    if((NULL == p_text) || (0U == text_size) || (0U == timeout_ms) ||
       (0U == vtx316_encoding_is_valid(encoding)))
    {
        return VTX316_RET_PARAM;
    }
    if((VTX316_COMMAND_NONE != p_device->active_command) ||
       (VTX316_STATE_SPEAKING == p_device->state))
    {
        return VTX316_RET_BUSY;
    }

    text_limit = (VTX316_ENCODING_UTF8 == encoding) ?
                 VTX316_UTF8_TEXT_MAX_BYTES : VTX316_TEXT_MAX_BYTES;
    if(text_size > text_limit)
    {
        return VTX316_RET_PARAM;
    }

    data_size = (uint16_t)(text_size + VTX316_COMMAND_SIZE +
                           VTX316_ENCODING_SIZE);
    frame_head[0] = VTX316_FRAME_HEAD;
    frame_head[1] = (uint8_t)(data_size >> 8U);
    frame_head[2] = (uint8_t)data_size;
    frame_head[3] = VTX316_COMMAND_SPEAK;
    frame_head[4] = (uint8_t)encoding;
    ret = p_device->io.transmit(p_device->io.p_context,
                                frame_head,
                                (uint16_t)sizeof(frame_head),
                                p_text,
                                text_size,
                                timeout_ms);
    if(VTX316_RET_OK == ret)
    {
        p_device->active_command = VTX316_COMMAND_SPEAK;
        p_device->state = VTX316_STATE_COMMAND_PENDING;
    }
    return ret;
}

vtx316_ret_t vtx316_stop(vtx316_device_t *p_device, uint32_t timeout_ms)
{
    vtx316_ret_t ret;

    ret = vtx316_control_send(p_device, VTX316_COMMAND_STOP, timeout_ms);
    if(VTX316_RET_OK == ret)
    {
        p_device->active_command = VTX316_COMMAND_NONE;
        p_device->state = VTX316_STATE_IDLE;
    }
    return ret;
}

vtx316_ret_t vtx316_busy_get(vtx316_device_t *p_device,
                             uint8_t          *p_is_busy)
{
    vtx316_ret_t ret;

    if(NULL == p_is_busy)
    {
        return VTX316_RET_PARAM;
    }
    ret = vtx316_device_validate(p_device);
    if(VTX316_RET_OK != ret)
    {
        return ret;
    }
    ret = p_device->io.busy_read(p_device->io.p_context, p_is_busy);
    if((VTX316_RET_OK == ret) && (*p_is_busy > 1U))
    {
        return VTX316_RET_IO;
    }
    return ret;
}

vtx316_ret_t vtx316_response_process(vtx316_device_t *p_device,
                                     const uint8_t   *p_data,
                                     uint16_t         size,
                                     vtx316_event_t  *p_event)
{
    uint16_t index;
    vtx316_ret_t ret;

    if((NULL == p_data) || (0U == size) || (NULL == p_event))
    {
        return VTX316_RET_PARAM;
    }
    ret = vtx316_device_validate(p_device);
    if(VTX316_RET_OK != ret)
    {
        return ret;
    }

    *p_event = VTX316_EVENT_NONE;
    for(index = 0U; index < size; index++)
    {
        switch(p_data[index])
        {
            case VTX316_RESPONSE_INITIALIZED:
                p_device->active_command = VTX316_COMMAND_NONE;
                p_device->state = VTX316_STATE_IDLE;
                *p_event = (vtx316_event_t)(*p_event |
                                           VTX316_EVENT_INITIALIZED);
                break;

            case VTX316_RESPONSE_ACCEPTED:
                *p_event = (vtx316_event_t)(*p_event |
                                           VTX316_EVENT_COMMAND_ACCEPTED);
                if(VTX316_COMMAND_STOP == p_device->active_command)
                {
                    p_device->active_command = VTX316_COMMAND_NONE;
                    p_device->state = VTX316_STATE_IDLE;
                }
                break;

            case VTX316_RESPONSE_REJECTED:
                p_device->active_command = VTX316_COMMAND_NONE;
                p_device->state = VTX316_STATE_ERROR;
                *p_event = (vtx316_event_t)(*p_event |
                                           VTX316_EVENT_COMMAND_REJECTED);
                break;

            case VTX316_RESPONSE_SPEAKING:
                p_device->state = VTX316_STATE_SPEAKING;
                *p_event = (vtx316_event_t)(*p_event |
                                           VTX316_EVENT_SPEAKING);
                break;

            case VTX316_RESPONSE_IDLE:
                p_device->active_command = VTX316_COMMAND_NONE;
                p_device->state = VTX316_STATE_IDLE;
                *p_event = (vtx316_event_t)(*p_event | VTX316_EVENT_IDLE);
                break;

            case VTX316_RESPONSE_SLEEPING:
                p_device->active_command = VTX316_COMMAND_NONE;
                p_device->state = VTX316_STATE_SLEEPING;
                *p_event = (vtx316_event_t)(*p_event |
                                           VTX316_EVENT_SLEEPING);
                break;

            default:
                break;
        }
    }
    return VTX316_RET_OK;
}

vtx316_ret_t vtx316_state_get(vtx316_device_t *p_device,
                              vtx316_state_t  *p_state)
{
    vtx316_ret_t ret;

    if(NULL == p_state)
    {
        return VTX316_RET_PARAM;
    }
    ret = vtx316_device_validate(p_device);
    if(VTX316_RET_OK != ret)
    {
        return ret;
    }
    *p_state = p_device->state;
    return VTX316_RET_OK;
}

/* end of file --------------------------------------------------------------*/
