/**
 ******************************************************************************
 *@file               :   bsp_voice_synthesis.c
 *@brief              :   Bind board voice synthesis to VTX316 hardware.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_resources.h"
#include "bsp_voice_synthesis.h"
#include "plat_gpio.h"
#include "plat_uart.h"
#include "vtx316.h"

/* define -------------------------------------------------------------------*/
#define BSP_VOICE_SYNTHESIS_RX_READ_SIZE  16U

/* variables ----------------------------------------------------------------*/
static vtx316_device_t vtx316_device;
static uint8_t vtx316_is_initialized;

/* private functions --------------------------------------------------------*/
static vtx316_ret_t bsp_voice_synthesis_uart_transmit(
    void          *p_context,
    const uint8_t *p_header,
    uint16_t       header_size,
    const uint8_t *p_payload,
    uint16_t       payload_size,
    uint32_t       timeout_ms)
{
    platform_err_t ret;

    (void)p_context;
    if((NULL == p_header) || (0U == header_size) || (0U == timeout_ms) ||
       ((payload_size > 0U) && (NULL == p_payload)))
    {
        return VTX316_RET_PARAM;
    }

    ret = plat_uart_send(BOARD_UART_VTX316,
                         p_header,
                         header_size,
                         timeout_ms);
    if(PLATFORM_ERR_OK == ret && payload_size > 0U)
    {
        ret = plat_uart_send(BOARD_UART_VTX316,
                             p_payload,
                             payload_size,
                             timeout_ms);
    }

    if(PLATFORM_ERR_OK == ret)
    {
        return VTX316_RET_OK;
    }
    return (PLATFORM_ERR_TIMEOUT == ret) ?
           VTX316_RET_TIMEOUT : VTX316_RET_IO;
}

static vtx316_ret_t bsp_voice_synthesis_busy_read(void    *p_context,
                                                   uint8_t *p_is_busy)
{
    plat_gpio_state_t state;

    (void)p_context;
    if(NULL == p_is_busy)
    {
        return VTX316_RET_PARAM;
    }
    if(PLATFORM_ERR_OK != plat_gpio_read(BOARD_GPIO_TTS_R_B, &state))
    {
        return VTX316_RET_IO;
    }

    *p_is_busy = (PLAT_GPIO_SET == state) ? 1U : 0U;
    return VTX316_RET_OK;
}

static platform_err_t bsp_voice_synthesis_ret_map(vtx316_ret_t ret)
{
    switch(ret)
    {
        case VTX316_RET_OK:
            return PLATFORM_ERR_OK;
        case VTX316_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case VTX316_RET_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;
        case VTX316_RET_BUSY:
            return PLATFORM_ERR_BUSY;
        case VTX316_RET_IO:
        case VTX316_RET_NOT_READY:
        default:
            return PLATFORM_ERR_HW;
    }
}

static platform_err_t bsp_voice_synthesis_ready_check(void)
{
    return (0U != vtx316_is_initialized) ?
           PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}

static platform_err_t bsp_voice_synthesis_encoding_map(
    bsp_voice_synthesis_encoding_t encoding,
    vtx316_encoding_t             *p_vtx316_encoding)
{
    if(NULL == p_vtx316_encoding)
    {
        return PLATFORM_ERR_PARAM;
    }

    switch(encoding)
    {
        case BSP_VOICE_SYNTHESIS_ENCODING_GB2312:
            *p_vtx316_encoding = VTX316_ENCODING_GB2312;
            break;
        case BSP_VOICE_SYNTHESIS_ENCODING_GBK:
            *p_vtx316_encoding = VTX316_ENCODING_GBK;
            break;
        case BSP_VOICE_SYNTHESIS_ENCODING_UNICODE_LITTLE:
            *p_vtx316_encoding = VTX316_ENCODING_UNICODE_LITTLE;
            break;
        case BSP_VOICE_SYNTHESIS_ENCODING_UNICODE_BIG:
            *p_vtx316_encoding = VTX316_ENCODING_UNICODE_BIG;
            break;
        case BSP_VOICE_SYNTHESIS_ENCODING_UTF8:
            *p_vtx316_encoding = VTX316_ENCODING_UTF8;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static bsp_voice_synthesis_event_t bsp_voice_synthesis_event_map(
    vtx316_event_t vtx316_event)
{
    bsp_voice_synthesis_event_t event = BSP_VOICE_SYNTHESIS_EVENT_NONE;

    if(0U != (vtx316_event & VTX316_EVENT_INITIALIZED))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_INITIALIZED);
    }
    if(0U != (vtx316_event & VTX316_EVENT_COMMAND_ACCEPTED))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_COMMAND_ACCEPTED);
    }
    if(0U != (vtx316_event & VTX316_EVENT_COMMAND_REJECTED))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_COMMAND_REJECTED);
    }
    if(0U != (vtx316_event & VTX316_EVENT_SPEAKING))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_SPEAKING);
    }
    if(0U != (vtx316_event & VTX316_EVENT_IDLE))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_IDLE);
    }
    if(0U != (vtx316_event & VTX316_EVENT_SLEEPING))
    {
        event = (bsp_voice_synthesis_event_t)(
            event | BSP_VOICE_SYNTHESIS_EVENT_SLEEPING);
    }
    return event;
}

static bsp_voice_synthesis_state_t bsp_voice_synthesis_state_map(
    vtx316_state_t state)
{
    switch(state)
    {
        case VTX316_STATE_IDLE:
            return BSP_VOICE_SYNTHESIS_STATE_IDLE;
        case VTX316_STATE_COMMAND_PENDING:
            return BSP_VOICE_SYNTHESIS_STATE_COMMAND_PENDING;
        case VTX316_STATE_SPEAKING:
            return BSP_VOICE_SYNTHESIS_STATE_SPEAKING;
        case VTX316_STATE_SLEEPING:
            return BSP_VOICE_SYNTHESIS_STATE_SLEEPING;
        case VTX316_STATE_ERROR:
            return BSP_VOICE_SYNTHESIS_STATE_ERROR;
        case VTX316_STATE_UNKNOWN:
        default:
            return BSP_VOICE_SYNTHESIS_STATE_UNKNOWN;
    }
}

/* exported functions -------------------------------------------------------*/
platform_err_t bsp_voice_synthesis_init(void)
{
    static const vtx316_io_t io =
    {
        bsp_voice_synthesis_uart_transmit,
        bsp_voice_synthesis_busy_read,
        NULL
    };
    vtx316_ret_t ret;
    platform_err_t plat_ret;

    if(0U != vtx316_is_initialized)
    {
        return PLATFORM_ERR_OK;
    }

    plat_ret = plat_uart_receive_start(BOARD_UART_VTX316);
    if(PLATFORM_ERR_OK != plat_ret)
    {
        return plat_ret;
    }

    ret = vtx316_init(&vtx316_device, &io);
    if(VTX316_RET_OK == ret)
    {
        vtx316_is_initialized = 1U;
    }
    return bsp_voice_synthesis_ret_map(ret);
}

platform_err_t bsp_voice_synthesis_speak(
    bsp_voice_synthesis_encoding_t encoding,
    const uint8_t                 *p_text,
    uint16_t                       text_size,
    uint32_t                       timeout_ms)
{
    uint8_t is_busy;
    vtx316_encoding_t vtx316_encoding;
    vtx316_ret_t ret;
    platform_err_t plat_ret;

    if(PLATFORM_ERR_OK != bsp_voice_synthesis_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    plat_ret = bsp_voice_synthesis_encoding_map(encoding,
                                                &vtx316_encoding);
    if(PLATFORM_ERR_OK != plat_ret)
    {
        return plat_ret;
    }
    ret = vtx316_busy_get(&vtx316_device, &is_busy);
    if(VTX316_RET_OK != ret)
    {
        return bsp_voice_synthesis_ret_map(ret);
    }
    if(0U != is_busy)
    {
        return PLATFORM_ERR_BUSY;
    }

    return bsp_voice_synthesis_ret_map(vtx316_speak(
        &vtx316_device,
        vtx316_encoding,
        p_text,
        text_size,
        timeout_ms));
}

platform_err_t bsp_voice_synthesis_stop(uint32_t timeout_ms)
{
    if(PLATFORM_ERR_OK != bsp_voice_synthesis_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_voice_synthesis_ret_map(
        vtx316_stop(&vtx316_device, timeout_ms));
}

platform_err_t bsp_voice_synthesis_busy_get(uint8_t *p_is_busy)
{
    if(NULL == p_is_busy)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(PLATFORM_ERR_OK != bsp_voice_synthesis_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_voice_synthesis_ret_map(
        vtx316_busy_get(&vtx316_device, p_is_busy));
}

platform_err_t bsp_voice_synthesis_poll(
    bsp_voice_synthesis_event_t *p_event)
{
    uint8_t rx_data[BSP_VOICE_SYNTHESIS_RX_READ_SIZE];
    uint16_t available;
    uint16_t read_size;
    vtx316_event_t vtx316_event;
    vtx316_ret_t vtx_ret;
    platform_err_t ret;

    if(NULL == p_event)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(PLATFORM_ERR_OK != bsp_voice_synthesis_ready_check())
    {
        return PLATFORM_ERR_HW;
    }

    *p_event = BSP_VOICE_SYNTHESIS_EVENT_NONE;
    ret = plat_uart_get_rx_size(BOARD_UART_VTX316, &available);
    while((PLATFORM_ERR_OK == ret) && (available > 0U))
    {
        if(available > (uint16_t)sizeof(rx_data))
        {
            available = (uint16_t)sizeof(rx_data);
        }
        ret = plat_uart_read(BOARD_UART_VTX316,
                             rx_data,
                             available,
                             &read_size);
        if(PLATFORM_ERR_OK != ret)
        {
            break;
        }
        if(0U == read_size)
        {
            break;
        }

        vtx_ret = vtx316_response_process(&vtx316_device,
                                          rx_data,
                                          read_size,
                                          &vtx316_event);
        if(VTX316_RET_OK != vtx_ret)
        {
            return bsp_voice_synthesis_ret_map(vtx_ret);
        }
        *p_event = (bsp_voice_synthesis_event_t)(
            *p_event | bsp_voice_synthesis_event_map(vtx316_event));
        ret = plat_uart_get_rx_size(BOARD_UART_VTX316, &available);
    }
    return ret;
}

platform_err_t bsp_voice_synthesis_state_get(
    bsp_voice_synthesis_state_t *p_state)
{
    vtx316_state_t vtx316_state;
    vtx316_ret_t vtx_ret;

    if(NULL == p_state)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(PLATFORM_ERR_OK != bsp_voice_synthesis_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    vtx_ret = vtx316_state_get(&vtx316_device, &vtx316_state);
    if(VTX316_RET_OK == vtx_ret)
    {
        *p_state = bsp_voice_synthesis_state_map(vtx316_state);
    }
    return bsp_voice_synthesis_ret_map(vtx_ret);
}

/* end of file --------------------------------------------------------------*/
