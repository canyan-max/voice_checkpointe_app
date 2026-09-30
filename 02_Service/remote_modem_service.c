/** One bounded state machine for startup reads, configuration and recovery. */
#include <stddef.h>
#include <string.h>
#include "remote_modem_service.h"
#include "bsp_remote_modem.h"
#define REMOTE_SEND_TIMEOUT_MS 500U
#define REMOTE_REPLY_TIMEOUT_MS 3000U
#define REMOTE_BOOT_SETTLE_MS 2000U
#define REMOTE_RESTART_TIMEOUT_MS 30000U
#define REMOTE_QUIET_MS 100U
#define REMOTE_RECOVERY_QUIET_MS 3000U
#define REMOTE_QUIET_TIMEOUT_MS 6000U
#define REMOTE_RECOVERY_TIMEOUT_MS 30000U
#define REMOTE_FLOW_TIMEOUT_MS 120000U
#define REMOTE_QUIET_READ 0U
#define REMOTE_QUIET_WRITE 1U
#define REMOTE_QUIET_RECOVER 2U
static const uint8_t remote_heartbeat[] = "www.usr.cn";
static const uint8_t remote_server[]    = "62195.cnsh.iot-tcp.com";
/* Workflow order belongs here; chip command strings remain below BSP. */
static const bsp_remote_modem_operation_t remote_modem_commands[] =
    {BSP_REMOTE_MODEM_CONFIG_ENTER,
     BSP_REMOTE_MODEM_TRANSPARENT_ENABLE,
     BSP_REMOTE_MODEM_FRAME_CONVERSION_DISABLE,
     BSP_REMOTE_MODEM_DATA_ENCODING_DISABLE,
     BSP_REMOTE_MODEM_HEARTBEAT_SET,
     BSP_REMOTE_MODEM_REGISTRATION_SET,
     BSP_REMOTE_MODEM_SERVER_SET,
     BSP_REMOTE_MODEM_SECONDARY_DISABLE,
     BSP_REMOTE_MODEM_SECURITY_DISABLE,
     BSP_REMOTE_MODEM_STATUS_ENABLE,
     BSP_REMOTE_MODEM_CONFIG_SAVE,
     BSP_REMOTE_MODEM_RESTART};
static const bsp_remote_modem_operation_t remote_modem_runtime_commands[] =
    {BSP_REMOTE_MODEM_CONFIG_ENTER,        BSP_REMOTE_MODEM_SIM_QUERY,
     BSP_REMOTE_MODEM_DEVICE_ID_QUERY,     BSP_REMOTE_MODEM_SIM_ID_QUERY,
     BSP_REMOTE_MODEM_SUBSCRIBER_ID_QUERY, BSP_REMOTE_MODEM_REGISTRATION_QUERY,
     BSP_REMOTE_MODEM_CONNECTION_QUERY,    BSP_REMOTE_MODEM_CONFIG_EXIT};
#define REMOTE_CONFIG_COUNT (sizeof(remote_modem_commands) / sizeof(remote_modem_commands[0]))
#define REMOTE_READ_COUNT (sizeof(remote_modem_runtime_commands) / sizeof(remote_modem_runtime_commands[0]))
_Static_assert(REMOTE_CONFIG_COUNT <= 255U && REMOTE_READ_COUNT <= 255U,
               "Modem step counters must hold command counts");

uint8_t remote_modem_service_id_is_valid(const uint8_t *p_id, uint16_t size)
{
    uint16_t index;
    if((NULL == p_id) || (REMOTE_MODEM_ID_SIZE != size))
    {
        return 0U;
    }
    for(index = 0U; index < size; ++index)
    {
        uint8_t c = p_id[index];
        if(!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) ||
             ((c >= 'A') && (c <= 'Z'))))
        {
            return 0U;
        }
    }
    return 1U;
}
static void
remote_quiet_start(remote_modem_service_t *s, uint8_t target, uint32_t now_ms)
{
    s->status.mode      = REMOTE_MODEM_MODE_CONFIGURATION;
    s->status.stage     = REMOTE_MODEM_STAGE_QUIET;
    s->quiet_target     = target;
    s->stage_start_ms   = now_ms;
    s->last_rx_ms       = now_ms;
    s->pending          = 0U;
    s->status.connected = 0U;
    (void)bsp_remote_modem_session_reset();
}
static void
remote_fail(remote_modem_service_t *s, platform_err_t error, uint32_t now_ms)
{
    s->status.last_error   = error;
    s->status.failed_stage = s->status.stage;
    s->status.failed_step  = s->status.step;
    s->status.mode         = REMOTE_MODEM_MODE_CONFIGURATION;
    s->status.connected    = 0U;
    s->pending             = 0U;
    memset(&s->status.identity, 0, sizeof(s->status.identity));
    if(REMOTE_MODEM_CONFIG_RUNNING == s->status.config_result)
    {
        s->status.config_result = REMOTE_MODEM_CONFIG_FAILED;
    }
    if(0U == s->recovery_attempted)
    {
        s->recovery_attempted = 1U;
        s->flow_start_ms      = now_ms;
        remote_quiet_start(s, REMOTE_QUIET_RECOVER, now_ms);
    }
    else
    {
        s->status.stage = REMOTE_MODEM_STAGE_FAULT;
        (void)bsp_remote_modem_session_reset();
    }
}
static platform_err_t remote_step_send(remote_modem_service_t *s,
                                       uint32_t                now_ms)
{
    bsp_remote_modem_request_t request = {0};
    platform_err_t             result;
    if(REMOTE_MODEM_STAGE_WRITE_CONFIG == s->status.stage)
    {
        if(s->status.step >= REMOTE_CONFIG_COUNT)
        {
            return PLATFORM_ERR_PARAM;
        }
        request.operation = remote_modem_commands[s->status.step];
    }
    else if(REMOTE_MODEM_STAGE_STARTUP_READ == s->status.stage)
    {
        if(s->status.step >= REMOTE_READ_COUNT)
        {
            return PLATFORM_ERR_PARAM;
        }
        request.operation = remote_modem_runtime_commands[s->status.step];
    }
    else if(REMOTE_MODEM_STAGE_RECOVER_ENTER == s->status.stage)
    {
        request.operation = BSP_REMOTE_MODEM_CONFIG_ENTER;
    }
    else if(REMOTE_MODEM_STAGE_RECOVER_EXIT == s->status.stage)
    {
        request.operation = BSP_REMOTE_MODEM_CONFIG_EXIT;
    }
    else
    {
        return PLATFORM_ERR_PARAM;
    }
    switch(request.operation)
    {
        case BSP_REMOTE_MODEM_HEARTBEAT_SET:
            request.p_text           = remote_heartbeat;
            request.size             = sizeof(remote_heartbeat) - 1U;
            request.interval_seconds = 60U;
            break;
        case BSP_REMOTE_MODEM_REGISTRATION_SET:
            request.p_text = s->requested_id;
            request.size   = REMOTE_MODEM_ID_SIZE;
            break;
        case BSP_REMOTE_MODEM_SERVER_SET:
            request.p_text = remote_server;
            request.size   = sizeof(remote_server) - 1U;
            request.port   = 15000U;
            break;
        default:
            break;
    }
    result = bsp_remote_modem_command_start(&request, now_ms,
                                            REMOTE_SEND_TIMEOUT_MS,
                                            REMOTE_REPLY_TIMEOUT_MS);
    if(PLATFORM_ERR_OK == result)
    {
        s->pending = 1U;
    }
    return result;
}
static platform_err_t remote_query_commit(remote_modem_service_t          *s,
                                          const bsp_remote_modem_report_t *r)
{
    remote_modem_identity_t     *identity = &s->status.identity;
    bsp_remote_modem_operation_t operation;
    if(s->status.step >= REMOTE_READ_COUNT)
    {
        return PLATFORM_ERR_PARAM;
    }
    operation = remote_modem_runtime_commands[s->status.step];
    switch(operation)
    {
        case BSP_REMOTE_MODEM_SIM_QUERY:
            if(0U == r->sim_ready)
            {
                return PLATFORM_ERR_HW;
            }
            break;
        case BSP_REMOTE_MODEM_DEVICE_ID_QUERY:
            if(0U == (r->identity.valid_mask & BSP_REMOTE_MODEM_ID_DEVICE))
            {
                return PLATFORM_ERR_HW;
            }
            memcpy(identity->imei, r->identity.device_id,
                   sizeof(identity->imei));
            identity->valid_mask |= REMOTE_MODEM_IDENTITY_IMEI;
            break;
        case BSP_REMOTE_MODEM_SIM_ID_QUERY:
            if(0U == (r->identity.valid_mask & BSP_REMOTE_MODEM_ID_SIM))
            {
                return PLATFORM_ERR_HW;
            }
            memcpy(identity->iccid, r->identity.sim_id,
                   sizeof(identity->iccid));
            identity->valid_mask |= REMOTE_MODEM_IDENTITY_ICCID;
            break;
        case BSP_REMOTE_MODEM_SUBSCRIBER_ID_QUERY:
            if(0U == (r->identity.valid_mask & BSP_REMOTE_MODEM_ID_SUBSCRIBER))
            {
                return PLATFORM_ERR_HW;
            }
            memcpy(identity->imsi, r->identity.subscriber_id,
                   sizeof(identity->imsi));
            identity->valid_mask |= REMOTE_MODEM_IDENTITY_IMSI;
            break;
        case BSP_REMOTE_MODEM_REGISTRATION_QUERY:
            /* Unconfigured factory registration is allowed during startup.
             * A successful configuration must read back exactly our new ID. */
            if((0U !=
                (r->identity.valid_mask & BSP_REMOTE_MODEM_ID_REGISTRATION)) &&
               (strlen(r->identity.registration) == REMOTE_MODEM_ID_SIZE) &&
               (0U !=
                remote_modem_service_id_is_valid((const uint8_t *)
                                                     r->identity.registration,
                                                 REMOTE_MODEM_ID_SIZE)))
            {
                memcpy(identity->id, r->identity.registration,
                       sizeof(identity->id));
                identity->valid_mask |= REMOTE_MODEM_IDENTITY_ID;
            }
            if((REMOTE_MODEM_CONFIG_RUNNING == s->status.config_result) &&
               ((0U == (identity->valid_mask & REMOTE_MODEM_IDENTITY_ID)) ||
                (0 !=
                 memcmp(identity->id, s->requested_id, REMOTE_MODEM_ID_SIZE))))
            {
                return PLATFORM_ERR_HW;
            }
            break;
        case BSP_REMOTE_MODEM_CONNECTION_QUERY:
            if(0U == r->connection_present)
            {
                return PLATFORM_ERR_HW;
            }
            s->status.connected = r->connected;
            break;
        default:
            break;
    }
    return PLATFORM_ERR_OK;
}
static void remote_boot_wait(remote_modem_service_t *s,
                             uint32_t                now_ms,
                             uint8_t                 confirmed_ready)
{
    s->status.mode      = REMOTE_MODEM_MODE_CONFIGURATION;
    s->status.stage     = REMOTE_MODEM_STAGE_BOOT_WAIT;
    s->boot_ready       = confirmed_ready;
    s->stage_start_ms   = now_ms;
    s->status.step      = 0U;
    s->status.connected = 0U;
    s->pending          = 0U;
    memset(&s->status.identity, 0, sizeof(s->status.identity));
    (void)bsp_remote_modem_session_reset();
}
static platform_err_t remote_completion(remote_modem_service_t          *s,
                                        const bsp_remote_modem_report_t *r,
                                        uint32_t                         now_ms)
{
    platform_err_t result = r->command_result;
    s->pending            = 0U;
    if(PLATFORM_ERR_OK != result)
    {
        return result;
    }
    switch(s->status.stage)
    {
        case REMOTE_MODEM_STAGE_STARTUP_READ:
            result = remote_query_commit(s, r);
            if(PLATFORM_ERR_OK != result)
            {
                return result;
            }
            ++s->status.step;
            if(s->status.step == REMOTE_READ_COUNT)
            {
                /* Last command is EXIT; its OK is mandatory for normal mode. */
                s->status.mode        = REMOTE_MODEM_MODE_NORMAL;
                s->status.stage       = REMOTE_MODEM_STAGE_NORMAL;
                s->recovery_attempted = 0U;
                if(REMOTE_MODEM_CONFIG_RUNNING == s->status.config_result)
                {
                    s->status.config_result = REMOTE_MODEM_CONFIG_SUCCEEDED;
                }
            }
            break;
        case REMOTE_MODEM_STAGE_WRITE_CONFIG:
            ++s->status.step;
            if(s->status.step == REMOTE_CONFIG_COUNT)
            {
                s->status.stage   = REMOTE_MODEM_STAGE_RESTART_WAIT;
                s->stage_start_ms = now_ms;
                if(0U != (r->events & BSP_REMOTE_MODEM_EVENT_READY))
                {
                    remote_boot_wait(s, now_ms, 1U);
                }
            }
            break;
        case REMOTE_MODEM_STAGE_RECOVER_ENTER:
            s->status.stage = REMOTE_MODEM_STAGE_RECOVER_EXIT;
            break;
        case REMOTE_MODEM_STAGE_RECOVER_EXIT:
            /* Recovery never changes a failed configuration into success. */
            memset(&s->status.identity, 0, sizeof(s->status.identity));
            remote_quiet_start(s, REMOTE_QUIET_READ, now_ms);
            break;
        default:
            return PLATFORM_ERR_HW;
    }
    return PLATFORM_ERR_OK;
}
platform_err_t remote_modem_service_init(remote_modem_service_t *s,
                                         uint32_t                now_ms)
{
    platform_err_t result;
    if(NULL == s)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != s->initialized)
    {
        return PLATFORM_ERR_BUSY;
    }
    memset(s, 0, sizeof(*s));
    result = bsp_remote_modem_init();
    if(PLATFORM_ERR_OK != result)
    {
        s->status.stage      = REMOTE_MODEM_STAGE_FAULT;
        s->status.last_error = result;
        return result;
    }
    s->initialized   = 1U;
    s->flow_start_ms = now_ms;
    remote_boot_wait(s, now_ms, 0U);
    return PLATFORM_ERR_OK;
}
platform_err_t remote_modem_service_config_start(remote_modem_service_t *s,
                                                 const uint8_t          *p_id,
                                                 uint16_t                size,
                                                 uint32_t                now_ms)
{
    if((NULL == s) || (0U == remote_modem_service_id_is_valid(p_id, size)))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == s->initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if((REMOTE_MODEM_STAGE_NORMAL != s->status.stage) &&
       (REMOTE_MODEM_STAGE_FAULT != s->status.stage))
    {
        return PLATFORM_ERR_BUSY;
    }
    memcpy(s->requested_id, p_id, REMOTE_MODEM_ID_SIZE);
    s->status.config_result = REMOTE_MODEM_CONFIG_RUNNING;
    s->status.last_error    = PLATFORM_ERR_OK;
    s->status.step          = 0U;
    s->recovery_attempted   = 0U;
    s->flow_start_ms        = now_ms;
    remote_quiet_start(s, REMOTE_QUIET_WRITE, now_ms);
    return PLATFORM_ERR_OK;
}
platform_err_t remote_modem_service_poll(remote_modem_service_t *s,
                                         uint32_t                now_ms,
                                         uint8_t                *p_data,
                                         uint16_t                capacity,
                                         uint16_t               *p_size)
{
    bsp_remote_modem_report_t report   = {0};
    uint16_t                  received = 0U;
    uint8_t                   was_normal;
    platform_err_t            result;
    if((NULL == s) || (NULL == p_data) || (0U == capacity) || (NULL == p_size))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    if(0U == s->initialized)
    {
        return PLATFORM_ERR_HW;
    }
    was_normal = (uint8_t)(REMOTE_MODEM_STAGE_NORMAL == s->status.stage);
    result     = bsp_remote_modem_poll(now_ms, p_data, capacity, &received,
                                       &report);
    if(0U != received)
    {
        s->last_rx_ms = now_ms;
    }
    if(PLATFORM_ERR_OK != result)
    {
        if(REMOTE_MODEM_STAGE_FAULT != s->status.stage)
        {
            remote_fail(s, result, now_ms);
        }
        return result;
    }
    if((0U != s->pending) &&
       (0U != (report.events & BSP_REMOTE_MODEM_EVENT_COMMAND_DONE)))
    {
        result = remote_completion(s, &report, now_ms);
        if(PLATFORM_ERR_OK != result)
        {
            remote_fail(s, result, now_ms);
            /* A fresh boot is a known synchronization boundary. Never lose
             * READY just because it also invalidated the pending command. */
            if(0U != (report.events & BSP_REMOTE_MODEM_EVENT_READY))
            {
                remote_boot_wait(s, now_ms, 1U);
            }
            return result;
        }
    }
    if(0U != (report.events & BSP_REMOTE_MODEM_EVENT_READY))
    {
        if((REMOTE_MODEM_STAGE_NORMAL == s->status.stage) ||
           (REMOTE_MODEM_STAGE_FAULT == s->status.stage))
        {
            s->flow_start_ms      = now_ms;
            s->recovery_attempted = 0U;
            remote_boot_wait(s, now_ms, 1U);
        }
        else if((REMOTE_MODEM_STAGE_RESTART_WAIT == s->status.stage) ||
                (REMOTE_MODEM_STAGE_BOOT_WAIT == s->status.stage))
        {
            remote_boot_wait(s, now_ms, 1U);
        }
        else if((REMOTE_MODEM_STAGE_QUIET == s->status.stage) &&
                (REMOTE_QUIET_READ == s->quiet_target))
        {
            remote_boot_wait(s, now_ms, 1U);
        }
        else if((REMOTE_MODEM_STAGE_QUIET == s->status.stage) &&
                (REMOTE_QUIET_WRITE == s->quiet_target))
        {
            remote_fail(s, PLATFORM_ERR_HW, now_ms);
            return PLATFORM_ERR_HW;
        }
        else if((REMOTE_MODEM_STAGE_QUIET != s->status.stage) &&
                (REMOTE_MODEM_STAGE_RECOVER_ENTER != s->status.stage) &&
                (REMOTE_MODEM_STAGE_RECOVER_EXIT != s->status.stage))
        {
            remote_fail(s, PLATFORM_ERR_HW, now_ms);
            return PLATFORM_ERR_HW;
        }
    }
    if(0U != (report.events & (BSP_REMOTE_MODEM_EVENT_LINK |
                               BSP_REMOTE_MODEM_EVENT_UNREGISTERED)))
    {
        s->status.connected = report.connected;
    }
    if(REMOTE_MODEM_STAGE_NORMAL == s->status.stage)
    {
        if(0U != was_normal)
        {
            *p_size = received;
        }
        return PLATFORM_ERR_OK;
    }
    if(REMOTE_MODEM_STAGE_FAULT == s->status.stage)
    {
        return PLATFORM_ERR_OK;
    }
    if((now_ms - s->flow_start_ms) >= REMOTE_FLOW_TIMEOUT_MS)
    {
        remote_fail(s, PLATFORM_ERR_TIMEOUT, now_ms);
        return PLATFORM_ERR_TIMEOUT;
    }
    if(REMOTE_MODEM_STAGE_BOOT_WAIT == s->status.stage)
    {
        if(((0U != s->boot_ready) &&
            ((now_ms - s->stage_start_ms) >= REMOTE_BOOT_SETTLE_MS)) ||
           ((0U == s->boot_ready) &&
            ((now_ms - s->stage_start_ms) >= REMOTE_RESTART_TIMEOUT_MS)))
        {
            remote_quiet_start(s, REMOTE_QUIET_READ, now_ms);
        }
    }
    else if(REMOTE_MODEM_STAGE_QUIET == s->status.stage)
    {
        uint32_t quiet_ms = (REMOTE_QUIET_RECOVER == s->quiet_target)
                                ? REMOTE_RECOVERY_QUIET_MS
                                : REMOTE_QUIET_MS;
        /* Normal business RX must not postpone an accepted configuration.

         * * Only recovery of an ambiguous old AT reply needs RX silence. */
        if(((REMOTE_QUIET_RECOVER == s->quiet_target) &&
            ((now_ms - s->last_rx_ms) >= quiet_ms)) ||
           ((REMOTE_QUIET_RECOVER != s->quiet_target) &&
            ((now_ms - s->stage_start_ms) >= quiet_ms)))
        {
            /* All stale bytes observed during quieting have been consumed. */
            (void)bsp_remote_modem_session_reset();
            s->status.step  = 0U;
            s->status.stage = (REMOTE_QUIET_WRITE == s->quiet_target)
                                  ? REMOTE_MODEM_STAGE_WRITE_CONFIG
                                  : ((REMOTE_QUIET_READ == s->quiet_target)
                                         ? REMOTE_MODEM_STAGE_STARTUP_READ
                                         : REMOTE_MODEM_STAGE_RECOVER_ENTER);
        }
        else if((now_ms - s->stage_start_ms) >=
                ((REMOTE_QUIET_RECOVER == s->quiet_target)
                     ? REMOTE_RECOVERY_TIMEOUT_MS
                     : REMOTE_QUIET_TIMEOUT_MS))
        {
            remote_fail(s, PLATFORM_ERR_TIMEOUT, now_ms);
            return PLATFORM_ERR_TIMEOUT;
        }
    }
    else if(REMOTE_MODEM_STAGE_RESTART_WAIT == s->status.stage)
    {
        if((now_ms - s->stage_start_ms) >= REMOTE_RESTART_TIMEOUT_MS)
        {
            remote_fail(s, PLATFORM_ERR_TIMEOUT, now_ms);
            return PLATFORM_ERR_TIMEOUT;
        }
    }
    if((0U == s->pending) &&
       ((REMOTE_MODEM_STAGE_STARTUP_READ == s->status.stage) ||
        (REMOTE_MODEM_STAGE_WRITE_CONFIG == s->status.stage) ||
        (REMOTE_MODEM_STAGE_RECOVER_ENTER == s->status.stage) ||
        (REMOTE_MODEM_STAGE_RECOVER_EXIT == s->status.stage)))
    {
        result = remote_step_send(s, now_ms);
        if(PLATFORM_ERR_OK != result)
        {
            remote_fail(s, result, now_ms);
        }
    }
    return result;
}
platform_err_t remote_modem_service_data_send(remote_modem_service_t *s,
                                              const uint8_t          *p_data,
                                              uint16_t                size,
                                              uint32_t                now_ms)
{
    platform_err_t result;
    if((NULL == s) || (NULL == p_data) || (0U == size))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == s->initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if(REMOTE_MODEM_STAGE_NORMAL != s->status.stage)
    {
        return PLATFORM_ERR_BUSY;
    }
    result = bsp_remote_modem_data_send(p_data, size, REMOTE_SEND_TIMEOUT_MS);
    if(PLATFORM_ERR_OK != result)
    {
        remote_fail(s, result, now_ms);
    }
    return result;
}
