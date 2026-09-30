/** One low-priority task owns the complete modem transport and Service. */
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "plat_sys.h"
#include "plat_log.h"
#include "led_indicator_app.h"
#include "remote_modem_app.h"
#define REMOTE_TASK_STACK_WORDS 1024U
#define REMOTE_TASK_POLL_MS 10U
#define REMOTE_TASK_REPORT_MS 10000U
#define REMOTE_RX_CHUNK_SIZE 256U
#define REMOTE_RX_TRACE_LIMIT 20U
#define REMOTE_RX_TRACE_BYTES 64U
#define REMOTE_COMMAND_CONFIG 0U
#define REMOTE_COMMAND_SEND 1U
typedef struct
{
    uint8_t  type;
    uint16_t size;
    uint8_t  data[REMOTE_MODEM_APP_SEND_MAX_SIZE];
} remote_command_t;
/* One fixed slot rejects competing requests. No accumulated configuration
 * queue. Stack: 4 KiB initial budget for nested parser, framing and logging
 * calls; validate free-word high-water logs under configuration and RX load on
 * board. */
static StaticTask_t              remote_task_control;
static StackType_t               remote_task_stack[REMOTE_TASK_STACK_WORDS];
static TaskHandle_t              remote_task_handle;
static StaticQueue_t             remote_queue_control;
static uint8_t                   remote_queue_storage[sizeof(remote_command_t)];
static QueueHandle_t             remote_queue;
static remote_modem_service_t    remote_service;
static remote_modem_app_io_t     remote_io;
static remote_modem_app_status_t remote_snapshot;
static uint8_t                   remote_request_reserved;
static uint8_t                   remote_backend_ready;
static uint8_t                   remote_rx_data[REMOTE_RX_CHUNK_SIZE];
static remote_command_t
                      remote_active_command; /* owner-only dequeue/TX storage */
static uint8_t        remote_led_connected = 0xFFU;
static uint32_t       remote_received_bytes;
static uint8_t        remote_rx_trace_count;
static uint8_t        remote_test_line[REMOTE_MODEM_APP_SEND_MAX_SIZE];
static uint16_t       remote_test_line_size;
static uint8_t        remote_test_discard;
static uint32_t       remote_test_last_rx_ms;
static platform_err_t remote_last_request_error;

static void remote_publish(uint8_t release_request)
{
    remote_modem_app_status_t snapshot;
    snapshot.modem              = remote_service.status;
    snapshot.received_bytes     = remote_received_bytes;
    snapshot.last_request_error = remote_last_request_error;
    snapshot.stack_free_words   = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    snapshot.busy               = (uint8_t)((REMOTE_MODEM_STAGE_NORMAL !=
                                             snapshot.modem.stage) &&
                                            (REMOTE_MODEM_STAGE_FAULT !=
                                             snapshot.modem.stage));
    taskENTER_CRITICAL();
    if(0U != release_request)
    {
        remote_request_reserved = 0U;
    }
    snapshot.busy |= remote_request_reserved;
    remote_snapshot      = snapshot;
    remote_backend_ready = remote_service.initialized;
    taskEXIT_CRITICAL();
}
static void remote_link_update(void)
{
    uint8_t connected = (uint8_t)((REMOTE_MODEM_MODE_NORMAL ==
                                   remote_service.status.mode) &&
                                  (0U != remote_service.status.connected));
    if(connected != remote_led_connected)
    {
        platform_err_t result = led_indicator_app_remote_connected_set(
            connected);
        if(PLATFORM_ERR_OK == result)
        {
            remote_led_connected = connected;
            plat_log_i("4G platform link %s",
                       (0U != connected) ? "connected" : "disconnected");
        }
    }
}
static void remote_command_handle(uint32_t now_ms)
{
    platform_err_t result;
    if(pdPASS != xQueueReceive(remote_queue, &remote_active_command, 0U))
    {
        return;
    }
    if(REMOTE_COMMAND_CONFIG == remote_active_command.type)
    {
        result = remote_modem_service_config_start(&remote_service,
                                                   remote_active_command.data,
                                                   remote_active_command.size,
                                                   now_ms);
        if(PLATFORM_ERR_OK == result)
        {
            plat_log_i("4G configuration execution started, ID=%.*s",
                       (int)remote_active_command.size,
                       (const char *)remote_active_command.data);
        }
        else
        {
            plat_log_e("4G accepted configuration could not start, ret=%d",
                       (int32_t)result);
        }
    }
    else if(REMOTE_COMMAND_SEND == remote_active_command.type)
    {
        result = remote_modem_service_data_send(&remote_service,
                                                remote_active_command.data,
                                                remote_active_command.size,
                                                now_ms);
        if(PLATFORM_ERR_OK != result)
        {
            plat_log_e("4G accepted data send failed, ret=%d", (int32_t)result);
        }
    }
    else
    {
        result = PLATFORM_ERR_PARAM;
        plat_log_e("4G invalid App command");
    }
    remote_last_request_error = result;
    remote_publish(1U);
}
/* Temporary newline-framed test protocol; never treat UART chunks as frames. */
static void
remote_test_receive(const uint8_t *bytes, uint16_t size, uint32_t now)
{
    uint16_t index;
    if((0U != remote_test_line_size || 0U != remote_test_discard) &&
       ((now - remote_test_last_rx_ms) >= 2000U))
    {
        remote_test_line_size = 0U;
        remote_test_discard   = 0U;
    }
    if(0U != size)
        remote_test_last_rx_ms = now;
    for(index = 0U; index < size; ++index)
    {
        uint8_t byte = bytes[index];
        if(0U == remote_test_discard)
        {
            if(remote_test_line_size < sizeof(remote_test_line))
            {
                remote_test_line[remote_test_line_size++] = byte;
            }
            else
            {
                remote_test_discard = 1U;
                plat_log_w("4G test line too long; discarded through newline");
            }
        }
        if('\n' == byte)
        {
            if((0U == remote_test_discard) && (remote_test_line_size >= 6U) &&
               (0 == memcmp(remote_test_line, "test:", 5U)))
            {
                platform_err_t
                    ret = remote_modem_service_data_send(&remote_service,
                                                         remote_test_line,
                                                         remote_test_line_size,
                                                         plat_tick_get_ms());
                remote_last_request_error = ret;
                if(PLATFORM_ERR_OK == ret)
                {
                    plat_log_i("4G test echo TX=%u bytes",
                               (unsigned int)remote_test_line_size);
                }
                else
                {
                    plat_log_e("4G test echo failed, ret=%d", (int32_t)ret);
                }
                if(REMOTE_MODEM_STAGE_NORMAL != remote_service.status.stage)
                {
                    remote_test_line_size = 0U;
                    remote_test_discard   = 0U;
                    return;
                }
            }
            remote_test_line_size = 0U;
            remote_test_discard   = 0U;
        }
    }
}
static void remote_receive_handle(uint32_t now_ms)
{
    uint16_t       size   = 0U;
    platform_err_t result = remote_modem_service_poll(&remote_service, now_ms,
                                                      remote_rx_data,
                                                      sizeof(remote_rx_data),
                                                      &size);
    if(PLATFORM_ERR_OK != result)
    {
        /* Fault latch suppresses repeated UART errors; log only a transition.
         */
        if(REMOTE_MODEM_STAGE_FAULT != remote_snapshot.modem.stage)
        {
            plat_log_e("4G flow failed, stage=%u step=%u ret=%d",
                       (unsigned int)remote_service.status.failed_stage,
                       (unsigned int)remote_service.status.failed_step,
                       (int32_t)result);
        }
    }
    remote_received_bytes += size;
    if((0U != size) && (remote_rx_trace_count < REMOTE_RX_TRACE_LIMIT))
    {
        ++remote_rx_trace_count;
        plat_log_i("4G normal RX sample %u/%u: size=%u, showing first %u bytes",
                   (unsigned int)remote_rx_trace_count,
                   (unsigned int)REMOTE_RX_TRACE_LIMIT, (unsigned int)size,
                   (unsigned int)((size > REMOTE_RX_TRACE_BYTES)
                                      ? REMOTE_RX_TRACE_BYTES
                                      : size));
        plat_log_hexdump("4G normal RX", 16U, remote_rx_data,
                         (uint16_t)((size > REMOTE_RX_TRACE_BYTES)
                                        ? REMOTE_RX_TRACE_BYTES
                                        : size));
        if(REMOTE_RX_TRACE_LIMIT == remote_rx_trace_count)
        {
            plat_log_i("4G RX samples finished; echo enabled=%u",
                       (unsigned int)remote_io.echo_test_enabled);
        }
    }
    if((0U != remote_io.echo_test_enabled) &&
       (REMOTE_MODEM_STAGE_NORMAL == remote_service.status.stage))
    {
        remote_test_receive(remote_rx_data, size, plat_tick_get_ms());
    }
    else
    {
        remote_test_line_size = 0U;
        remote_test_discard   = 0U;
    }

    /* Publish mode/busy before a hook can request new work. Configuration bytes
     * are already filtered by Service and never enter this callback. */
    remote_publish(0U);
    if((0U != size) && (NULL != remote_io.receive) &&
       (REMOTE_MODEM_STAGE_NORMAL == remote_service.status.stage))
    {
        remote_io.receive(remote_io.p_context, remote_rx_data, size);
    }
}
static void
remote_progress_report(remote_modem_stage_t         *p_stage,
                       remote_modem_config_result_t *p_config_result,
                       uint32_t                     *p_last_report_ms,
                       uint32_t                      now_ms)
{
    if(*p_stage != remote_service.status.stage)
    {
        *p_stage = remote_service.status.stage;
        plat_log_i("4G stage=%u, mode=%u, step=%u", (unsigned int)*p_stage,
                   (unsigned int)remote_service.status.mode,
                   (unsigned int)remote_service.status.step);
        if(REMOTE_MODEM_STAGE_NORMAL == *p_stage)
        {
            const remote_modem_identity_t *identity = &remote_service.status
                                                           .identity;
            plat_log_i("4G startup read complete; normal communication, "
                       "identity=0x%02X",
                       (unsigned int)identity->valid_mask);
            if(0U != (identity->valid_mask & REMOTE_MODEM_IDENTITY_IMEI))
            {
                plat_log_i("4G RAM IMEI=%s", identity->imei);
            }
            if(0U != (identity->valid_mask & REMOTE_MODEM_IDENTITY_ICCID))
            {
                plat_log_i("4G RAM ICCID=%s", identity->iccid);
            }
            if(0U != (identity->valid_mask & REMOTE_MODEM_IDENTITY_IMSI))
            {
                plat_log_i("4G RAM IMSI=%s", identity->imsi);
            }
            if(0U != (identity->valid_mask & REMOTE_MODEM_IDENTITY_ID))
            {
                plat_log_i("4G RAM ID=%s", identity->id);
            }
        }
    }
    if(*p_config_result != remote_service.status.config_result)
    {
        *p_config_result = remote_service.status.config_result;
        if(REMOTE_MODEM_CONFIG_SUCCEEDED == *p_config_result)
        {
            plat_log_i("4G configuration complete; ID verified and normal mode "
                       "restored");
        }
        else if(REMOTE_MODEM_CONFIG_FAILED == *p_config_result)
        {
            plat_log_e("4G configuration failed, ret=%d",
                       (int32_t)remote_service.status.last_error);
        }
    }
    if((now_ms - *p_last_report_ms) >= REMOTE_TASK_REPORT_MS)
    {
        *p_last_report_ms = now_ms;
        plat_log_i("4G task stack free=%lu words, normal RX=%lu bytes",
                   (unsigned long)uxTaskGetStackHighWaterMark(NULL),
                   (unsigned long)remote_received_bytes);
    }
}
static void remote_task(void *p_parameter)
{
    uint32_t                     now_ms          = plat_tick_get_ms();
    uint32_t                     last_report_ms  = now_ms;
    TickType_t                   last_wake       = xTaskGetTickCount();
    remote_modem_stage_t         previous_stage  = REMOTE_MODEM_STAGE_FAULT;
    remote_modem_config_result_t previous_config = REMOTE_MODEM_CONFIG_NONE;
    platform_err_t               result;
    (void)p_parameter;
    result = remote_modem_service_init(&remote_service, now_ms);
    plat_log_i("4G owner task started, service_init=%d", (int32_t)result);
    remote_publish(0U);
    for(;;)
    {
        now_ms = plat_tick_get_ms();
        remote_command_handle(now_ms);
        remote_receive_handle(plat_tick_get_ms());
        remote_link_update();
        remote_progress_report(&previous_stage, &previous_config,
                               &last_report_ms, plat_tick_get_ms());
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(REMOTE_TASK_POLL_MS));
    }
}
platform_err_t remote_modem_app_init(const remote_modem_app_io_t *p_io)
{
    if(NULL != remote_task_handle)
    {
        return PLATFORM_ERR_OK;
    }
    if(NULL != p_io)
    {
        remote_io = *p_io;
    }
    remote_snapshot.modem.mode  = REMOTE_MODEM_MODE_CONFIGURATION;
    remote_snapshot.modem.stage = REMOTE_MODEM_STAGE_BOOT_WAIT;
    remote_snapshot.busy        = 1U;
    remote_queue = xQueueCreateStatic(1U, sizeof(remote_command_t),
                                      remote_queue_storage,
                                      &remote_queue_control);
    if(NULL == remote_queue)
    {
        return PLATFORM_ERR_HW;
    }
    /* Same low priority as the existing test task: timeout TX spins cannot
     * starve its button polling; audio and timer tasks remain higher priority.
     */
    remote_task_handle = xTaskCreateStatic(remote_task, "remote_modem",
                                           REMOTE_TASK_STACK_WORDS, NULL,
                                           tskIDLE_PRIORITY, remote_task_stack,
                                           &remote_task_control);
    return (NULL != remote_task_handle) ? PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}
static platform_err_t remote_request_submit(const remote_command_t *p_command)
{
    platform_err_t result = PLATFORM_ERR_OK;
    taskENTER_CRITICAL();
    if(0U == remote_backend_ready)
    {
        result = PLATFORM_ERR_HW;
    }
    else if((0U != remote_request_reserved) ||
            ((REMOTE_MODEM_STAGE_NORMAL != remote_snapshot.modem.stage) &&
             ((REMOTE_COMMAND_CONFIG != p_command->type) ||
              (REMOTE_MODEM_STAGE_FAULT != remote_snapshot.modem.stage))))
    {
        result = PLATFORM_ERR_BUSY;
    }
    else
    {
        remote_request_reserved = 1U;
        remote_snapshot.busy    = 1U;
    }
    taskEXIT_CRITICAL();
    if(PLATFORM_ERR_OK != result)
    {
        return result;
    }
    if(pdPASS != xQueueSend(remote_queue, p_command, 0U))
    {
        taskENTER_CRITICAL();
        remote_request_reserved = 0U;
        remote_snapshot.busy    = (uint8_t)((REMOTE_MODEM_STAGE_NORMAL !=
                                             remote_snapshot.modem.stage) &&
                                            (REMOTE_MODEM_STAGE_FAULT !=
                                             remote_snapshot.modem.stage));
        taskEXIT_CRITICAL();
        return PLATFORM_ERR_BUSY;
    }
    return PLATFORM_ERR_OK;
}
platform_err_t remote_modem_app_config_request(const uint8_t *p_id,
                                               uint16_t       size)
{
    remote_command_t command = {0};
    if(0U == remote_modem_service_id_is_valid(p_id, size))
    {
        return PLATFORM_ERR_PARAM;
    }
    command.type = REMOTE_COMMAND_CONFIG;
    command.size = size;
    memcpy(command.data, p_id, size);
    return remote_request_submit(&command);
}
platform_err_t remote_modem_app_send_request(const uint8_t *p_data,
                                             uint16_t       size)
{
    remote_command_t command = {0};
    if((NULL == p_data) || (0U == size) || (size > sizeof(command.data)))
    {
        return PLATFORM_ERR_PARAM;
    }
    command.type = REMOTE_COMMAND_SEND;
    command.size = size;
    memcpy(command.data, p_data, size);
    return remote_request_submit(&command);
}
platform_err_t remote_modem_app_status_get(remote_modem_app_status_t *p_status)
{
    if(NULL == p_status)
    {
        return PLATFORM_ERR_PARAM;
    }
    taskENTER_CRITICAL();
    *p_status = remote_snapshot;
    taskEXIT_CRITICAL();
    return (NULL != remote_task_handle) ? PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}
platform_err_t
remote_modem_app_identity_get(remote_modem_identity_t *p_identity)
{
    if(NULL == p_identity)
    {
        return PLATFORM_ERR_PARAM;
    }
    taskENTER_CRITICAL();
    *p_identity = remote_snapshot.modem.identity;
    taskEXIT_CRITICAL();
    return (NULL != remote_task_handle) ? PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}
