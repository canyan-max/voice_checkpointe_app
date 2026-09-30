/** Bind the board's single remote modem to E27-D and the logical UART. */
#include <stddef.h>
#include <string.h>
#include "board_resources.h"
#include "bsp_remote_modem.h"
#include "plat_uart.h"
#include "plat_log.h"
#include "e27_d.h"
static e27_d_device_t            remote_device;
static bsp_remote_modem_report_t remote_report;
static uint8_t                   remote_initialized;
static platform_err_t            remote_result_from_chip(e27_d_ret_t result)
{
    switch(result)
    {
        case E27_D_RET_OK:
            return PLATFORM_ERR_OK;
        case E27_D_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case E27_D_RET_BUSY:
            return PLATFORM_ERR_BUSY;
        case E27_D_RET_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;
        default:
            return PLATFORM_ERR_HW;
    }
}
static e27_d_ret_t remote_transmit(void          *p_context,
                                   const uint8_t *p_data,
                                   uint16_t       size,
                                   uint32_t       timeout_ms)
{
    platform_err_t result;
    (void)p_context;
    plat_log_i("4G AT TX (%u bytes): %.*s", (unsigned int)size, (int)size,
               (const char *)p_data);
    result = plat_uart_send(BOARD_UART_REMOTE_4G, p_data, size, timeout_ms);
    if(PLATFORM_ERR_OK != result)
    {
        plat_log_e("4G AT TX failed, ret=%d", (int32_t)result);
    }
    switch(result)
    {
        case PLATFORM_ERR_OK:
            return E27_D_RET_OK;
        case PLATFORM_ERR_PARAM:
            return E27_D_RET_PARAM;
        case PLATFORM_ERR_BUSY:
            return E27_D_RET_BUSY;
        case PLATFORM_ERR_TIMEOUT:
            return E27_D_RET_TIMEOUT;
        default:
            return E27_D_RET_IO;
    }
}
static void remote_text_copy(char                *p_target,
                             uint16_t             capacity,
                             const e27_d_event_t *p_event,
                             uint8_t              valid_bit)
{
    if((NULL != p_event->p_text) && (p_event->size < capacity))
    {
        memcpy(p_target, p_event->p_text, p_event->size);
        p_target[p_event->size] = '\0';
        remote_report.identity.valid_mask |= valid_bit;
    }
}
static void remote_event(void *p_context, const e27_d_event_t *p_event)
{
    (void)p_context;
    switch(p_event->kind)
    {
        case E27_D_EVENT_READY:
            remote_report.events |= BSP_REMOTE_MODEM_EVENT_READY;
            remote_report.connected = 0U;
            break;
        case E27_D_EVENT_COMPLETE:
            remote_report.events |= BSP_REMOTE_MODEM_EVENT_COMMAND_DONE;
            remote_report.command_result = remote_result_from_chip(
                p_event->result);
            break;
        case E27_D_EVENT_LINK:
            if(1U == p_event->channel)
            {
                remote_report.events |= BSP_REMOTE_MODEM_EVENT_LINK;
                remote_report.connected = (uint8_t)(0U != p_event->value);
            }
            break;
        case E27_D_EVENT_REGISTRATION:
            if(0U == p_event->value)
            {
                remote_report.events |= BSP_REMOTE_MODEM_EVENT_UNREGISTERED;
                remote_report.connected = 0U;
            }
            break;
        case E27_D_EVENT_IMEI:
            remote_text_copy(remote_report.identity.device_id,
                             sizeof(remote_report.identity.device_id), p_event,
                             BSP_REMOTE_MODEM_ID_DEVICE);
            break;
        case E27_D_EVENT_ICCID:
            remote_text_copy(remote_report.identity.sim_id,
                             sizeof(remote_report.identity.sim_id), p_event,
                             BSP_REMOTE_MODEM_ID_SIM);
            break;
        case E27_D_EVENT_IMSI:
            remote_text_copy(remote_report.identity.subscriber_id,
                             sizeof(remote_report.identity.subscriber_id),
                             p_event, BSP_REMOTE_MODEM_ID_SUBSCRIBER);
            break;
        case E27_D_EVENT_DTUID:
            /* Board's custom ASCII registration; chip fields stay private. */
            if((1U == p_event->channel) && (1U == p_event->value) &&
               (0U == p_event->auxiliary) && (0U == p_event->format))
            {
                remote_text_copy(remote_report.identity.registration,
                                 sizeof(remote_report.identity.registration),
                                 p_event, BSP_REMOTE_MODEM_ID_REGISTRATION);
            }
            break;
        case E27_D_EVENT_SIM_READY:
            remote_report.sim_ready = 1U;
            break;
        case E27_D_EVENT_CONNECTIONS:
            remote_report.connection_present = 1U;
            remote_report.connected          = (uint8_t)(0U != p_event->value);
            break;
        default:
            break;
    }
}
platform_err_t bsp_remote_modem_init(void)
{
    platform_err_t result;
    e27_d_io_t     io = {remote_transmit, remote_event, NULL};
    if(0U != remote_initialized)
    {
        return PLATFORM_ERR_OK;
    }
    result = plat_uart_receive_start(BOARD_UART_REMOTE_4G);
    if(PLATFORM_ERR_OK != result)
    {
        return result;
    }
    result = remote_result_from_chip(e27_d_init(&remote_device, &io));
    if(PLATFORM_ERR_OK == result)
    {
        remote_initialized = 1U;
    }
    return result;
}
platform_err_t
bsp_remote_modem_command_start(const bsp_remote_modem_request_t *p_request,
                               uint32_t                          now_ms,
                               uint32_t send_timeout_ms,
                               uint32_t reply_timeout_ms)
{
    e27_d_request_t request = {0};
    uint8_t         heartbeat_hex[256];
    platform_err_t  result;
    if((NULL == p_request) ||
       ((unsigned int)p_request->operation >= BSP_REMOTE_MODEM_OPERATION_COUNT))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == remote_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    switch(p_request->operation)
    {
        case BSP_REMOTE_MODEM_CONFIG_ENTER:
            request.command = E27_D_CMD_ENTER;
            break;
        case BSP_REMOTE_MODEM_CONFIG_EXIT:
            request.command = E27_D_CMD_EXIT;
            break;
        case BSP_REMOTE_MODEM_CONFIG_SAVE:
            request.command = E27_D_CMD_SAVE;
            break;
        case BSP_REMOTE_MODEM_RESTART:
            request.command = E27_D_CMD_RESTART;
            break;
        case BSP_REMOTE_MODEM_SIM_QUERY:
            request.command = E27_D_CMD_CPIN;
            break;
        case BSP_REMOTE_MODEM_DEVICE_ID_QUERY:
            request.command = E27_D_CMD_IMEI;
            break;
        case BSP_REMOTE_MODEM_SIM_ID_QUERY:
            request.command = E27_D_CMD_ICCID;
            break;
        case BSP_REMOTE_MODEM_SUBSCRIBER_ID_QUERY:
            request.command = E27_D_CMD_IMSI;
            break;
        case BSP_REMOTE_MODEM_REGISTRATION_QUERY:
            request.command = E27_D_CMD_DTUID_QUERY;
            break;
        case BSP_REMOTE_MODEM_CONNECTION_QUERY:
            request.command = E27_D_CMD_CONNECT_QUERY;
            break;
        case BSP_REMOTE_MODEM_TRANSPARENT_ENABLE:
            request.command              = E27_D_CMD_SET_MODE;
            request.args.channel.value   = 1U;
            request.args.channel.channel = 1U;
            break;
        case BSP_REMOTE_MODEM_FRAME_CONVERSION_DISABLE:
            request.command              = E27_D_CMD_SET_MODBUS;
            request.args.channel.channel = 1U;
            break;
        case BSP_REMOTE_MODEM_DATA_ENCODING_DISABLE:
            request.command              = E27_D_CMD_SET_HEX;
            request.args.channel.channel = 1U;
            break;
        case BSP_REMOTE_MODEM_HEARTBEAT_SET:
        {
            static const uint8_t hex_digits[] = "0123456789ABCDEF";
            uint16_t             index;
            if((p_request->size > (sizeof(heartbeat_hex) / 2U)) ||
               ((0U != p_request->size) && (NULL == p_request->p_text)))
            {
                return PLATFORM_ERR_PARAM;
            }
            for(index = 0U; index < p_request->size; ++index)
            {
                heartbeat_hex[index *
                              2U] = hex_digits[p_request->p_text[index] >> 4];
                heartbeat_hex[index * 2U +
                              1U] = hex_digits[p_request->p_text[index] &
                                               0x0FU];
            }
            request.command              = E27_D_CMD_SET_KEEPALIVE;
            request.args.packet.channel  = 1U;
            request.args.packet.format   = 1U;
            request.args.packet.interval = p_request->interval_seconds;
            request.args.packet.p_text   = heartbeat_hex;
            request.args.packet.size     = (uint16_t)(p_request->size * 2U);
            break;
        }
        case BSP_REMOTE_MODEM_REGISTRATION_SET:
            request.command             = E27_D_CMD_SET_DTUID;
            request.args.packet.mode    = 1U;
            request.args.packet.channel = 1U;
            request.args.packet.p_text  = p_request->p_text;
            request.args.packet.size    = p_request->size;
            break;
        case BSP_REMOTE_MODEM_SERVER_SET:
            request.command             = E27_D_CMD_SET_SERVER;
            request.args.server.channel = 1U;
            request.args.server.p_host  = p_request->p_text;
            request.args.server.size    = p_request->size;
            request.args.server.port    = p_request->port;
            break;
        case BSP_REMOTE_MODEM_SECONDARY_DISABLE:
            request.command = E27_D_CMD_SET_SECONDARY;
            break;
        case BSP_REMOTE_MODEM_SECURITY_DISABLE:
            request.command = E27_D_CMD_DISABLE_SECURITY;
            break;
        case BSP_REMOTE_MODEM_STATUS_ENABLE:
            request.command                     = E27_D_CMD_SET_AUTOSTATUS;
            request.args.autostatus.level       = 3U;
            request.args.autostatus.boot_report = 1U;
            break;
        default:
            return PLATFORM_ERR_PARAM;
    }
    result = remote_result_from_chip(
        e27_d_command_start(&remote_device, &request, now_ms, send_timeout_ms,
                            reply_timeout_ms));
    if(PLATFORM_ERR_OK == result)
    {
        uint8_t connected = remote_report.connected;
        memset(&remote_report, 0, sizeof(remote_report));
        remote_report.connected = connected;
    }
    return result;
}
platform_err_t bsp_remote_modem_poll(uint32_t                   now_ms,
                                     uint8_t                   *p_data,
                                     uint16_t                   capacity,
                                     uint16_t                  *p_size,
                                     bsp_remote_modem_report_t *p_report)
{
    platform_err_t result;
    uint8_t        trace_reply;
    if((NULL == p_data) || (0U == capacity) || (NULL == p_size) ||
       (NULL == p_report))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    if(0U == remote_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    trace_reply = (uint8_t)((0U != remote_device.pending) ||
                            (E27_D_MODE_TRANSPARENT != remote_device.mode));
    (void)e27_d_tick(&remote_device, now_ms);
    result = plat_uart_read(BOARD_UART_REMOTE_4G, p_data, capacity, p_size);
    if(PLATFORM_ERR_OK == result)
    {
        (void)e27_d_response_process(&remote_device, p_data, *p_size);
        if((0U != trace_reply) && (0U != *p_size))
        {
            /* Bound diagnostics so UART logging does not dump full streams. */
            plat_log_i("4G AT RX (%u bytes, first 160): %.*s",
                       (unsigned int)*p_size,
                       (int)((*p_size > 160U) ? 160U : *p_size),
                       (const char *)p_data);
        }
    }
    else
    {
        *p_size = 0U;
        (void)e27_d_transport_lost(&remote_device);
    }
    if(0U != (remote_report.events & BSP_REMOTE_MODEM_EVENT_COMMAND_DONE))
    {
        if(PLATFORM_ERR_OK == remote_report.command_result)
        {
            plat_log_i("4G AT reply OK");
        }
        else
        {
            plat_log_e("4G AT reply failed, ret=%d",
                       (int32_t)remote_report.command_result);
        }
    }
    if(0U != (remote_report.events & BSP_REMOTE_MODEM_EVENT_READY))
    {
        plat_log_i("4G modem READY");
    }
    *p_report            = remote_report;
    remote_report.events = 0U;
    return result;
}
platform_err_t bsp_remote_modem_data_send(const uint8_t *p_data,
                                          uint16_t       size,
                                          uint32_t       timeout_ms)
{
    if((NULL == p_data) || (0U == size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    if((0U == remote_initialized) ||
       (E27_D_MODE_TRANSPARENT != remote_device.mode) ||
       (0U != remote_device.pending))
    {
        return PLATFORM_ERR_BUSY;
    }
    return plat_uart_send(BOARD_UART_REMOTE_4G, p_data, size, timeout_ms);
}
platform_err_t bsp_remote_modem_session_reset(void)
{
    e27_d_io_t io;
    if(0U == remote_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    (void)e27_d_transport_lost(&remote_device);
    /* This is an explicit parser-session boundary. Transport loss alone
     * discards through the next delimiter, which could swallow a fresh READY.
     */
    io = remote_device.io;
    (void)e27_d_init(&remote_device, &io);
    memset(&remote_report, 0, sizeof(remote_report));
    return PLATFORM_ERR_OK;
}
