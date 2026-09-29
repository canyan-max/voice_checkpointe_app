/** Temporary 4G board diagnostics and factory configuration entry. */
#include <stddef.h>
#include <string.h>
#include "plat_log.h"
#include "led_indicator_app.h"
#include "remote_modem_config_app.h"
#include "remote_modem_config_service.h"

#define REMOTE_MODEM_CONFIG_RX_CHUNK_SIZE  32U
#define REMOTE_MODEM_MONITOR_RX_CHUNK_SIZE  64U
#define REMOTE_MODEM_BOOT_SETTLE_MS       2000U
#define REMOTE_MODEM_MONITOR_LINE_SIZE      48U

/* Temporary board-test input; production will obtain this from RS485. */
static const uint8_t remote_modem_test_id[] = "kvt9dxr84qrryr1z";
remote_modem_config_service_t remote_modem_config;
static uint8_t remote_modem_config_requested;
static uint8_t remote_modem_config_finished;
static uint8_t remote_modem_diagnostic_active;
static uint8_t remote_modem_monitor_ready;
static uint8_t remote_modem_monitor_error_logged;
static uint8_t remote_modem_runtime_active;
static uint8_t remote_modem_runtime_due;
static uint8_t remote_modem_runtime_verified;
static uint8_t remote_modem_link_connected;
static uint32_t remote_modem_runtime_due_ms;
static char remote_modem_monitor_line[REMOTE_MODEM_MONITOR_LINE_SIZE];
static uint8_t remote_modem_monitor_line_size;
static uint8_t remote_modem_monitor_line_overflow;

_Static_assert(sizeof(remote_modem_test_id) ==
               (REMOTE_MODEM_CONFIG_ID_SIZE + 1U),
               "Remote modem test ID length mismatch");

static void remote_modem_app_link_set(uint8_t connected)
{
    platform_err_t ret;

    if(connected != remote_modem_link_connected)
    {
        remote_modem_link_connected = connected;
        ret = led_indicator_app_remote_connected_set(connected);
        if(PLATFORM_ERR_OK != ret)
        {
            plat_log_e("4G LED1 update failed, ret=%d", (int32_t)ret);
        }
        plat_log_i("4G platform link %s", (0U != connected) ?
                   "connected" : "disconnected");
    }
}

static void remote_modem_app_monitor_line(uint32_t now_ms)
{
    if((0U == remote_modem_monitor_line_size) ||
       (0U != remote_modem_monitor_line_overflow))
    {
        return;
    }
    remote_modem_monitor_line[remote_modem_monitor_line_size] = '\0';
    if(0 == strcmp(remote_modem_monitor_line, "AT Ready"))
    {
        if(0U != remote_modem_runtime_due)
        {
            return;
        }
        remote_modem_runtime_verified = 0U;
        remote_modem_app_link_set(0U);
        memset(&remote_modem_config.identity, 0,
               sizeof(remote_modem_config.identity));
        remote_modem_runtime_due_ms = now_ms + REMOTE_MODEM_BOOT_SETTLE_MS;
        remote_modem_runtime_due = 1U;
        plat_log_i("4G AT Ready; runtime check in 2 s");
    }
    else if((0 == strcmp(remote_modem_monitor_line,
                         "+STATUS: 1, CLOSED")) ||
            (0 == strcmp(remote_modem_monitor_line,
                         "+STATUS:NET STATE UNREGISTER")))
    {
        remote_modem_app_link_set(0U);
    }
    else if(0 == strcmp(remote_modem_monitor_line,
                        "+STATUS: 1, CONNECTED"))
    {
        if(0U != remote_modem_runtime_verified)
        {
            remote_modem_app_link_set(1U);
        }
    }
}

static void remote_modem_app_monitor_byte(uint8_t byte, uint32_t now_ms)
{
    if(('\r' == byte) || ('\n' == byte))
    {
        remote_modem_app_monitor_line(now_ms);
        remote_modem_monitor_line_size = 0U;
        remote_modem_monitor_line_overflow = 0U;
    }
    else if((byte >= 0x20U) && (byte <= 0x7EU) &&
            (remote_modem_monitor_line_size <
             (sizeof(remote_modem_monitor_line) - 1U)))
    {
        remote_modem_monitor_line[remote_modem_monitor_line_size++] =
            (char)byte;
    }
    else
    {
        remote_modem_monitor_line_overflow = 1U;
    }
}

platform_err_t remote_modem_config_app_monitor_start(void)
{
    platform_err_t ret = remote_modem_config_service_monitor_start();

    if(PLATFORM_ERR_OK == ret)
    {
        remote_modem_monitor_ready = 1U;
    }
    return ret;
}

void remote_modem_config_app_monitor_poll(uint32_t now_ms)
{
    uint8_t rx_data[REMOTE_MODEM_MONITOR_RX_CHUNK_SIZE];
    uint16_t rx_size;
    platform_err_t ret;

    if((0U == remote_modem_monitor_ready) ||
       ((0U != remote_modem_config_requested) &&
        (0U == remote_modem_config_finished)) ||
       (0U != remote_modem_diagnostic_active) ||
       (0U != remote_modem_runtime_active))
    {
        return;
    }
    ret = remote_modem_config_service_monitor_poll(rx_data,
                                                   (uint16_t)sizeof(rx_data),
                                                   &rx_size);
    if(PLATFORM_ERR_OK != ret)
    {
        remote_modem_runtime_verified = 0U;
        remote_modem_app_link_set(0U);
        if(0U == remote_modem_monitor_error_logged)
        {
            plat_log_e("4G passive RX failed, ret=%d", (int32_t)ret);
            remote_modem_monitor_error_logged = 1U;
        }
        return;
    }
    remote_modem_monitor_error_logged = 0U;
    if(0U != rx_size)
    {
        uint16_t index;

        plat_log_hexdump("4G RX", 16U, rx_data, rx_size);
        for(index = 0U; index < rx_size; index++)
        {
            remote_modem_app_monitor_byte(rx_data[index], now_ms);
        }
    }
}

platform_err_t remote_modem_config_app_request(uint32_t now_ms)
{
    platform_err_t ret;

    if((0U != remote_modem_diagnostic_active) ||
       (0U != remote_modem_runtime_active) ||
       (0U != remote_modem_runtime_due) ||
       ((0U != remote_modem_config_requested) &&
        (0U == remote_modem_config_finished)))
    {
        return PLATFORM_ERR_BUSY;
    }
    ret = remote_modem_config_service_start(
        &remote_modem_config,
        remote_modem_test_id,
        REMOTE_MODEM_CONFIG_ID_SIZE,
        now_ms);
    if(PLATFORM_ERR_OK == ret)
    {
        remote_modem_runtime_due = 0U;
        remote_modem_runtime_verified = 0U;
        remote_modem_app_link_set(0U);
        remote_modem_config_requested = 1U;
        remote_modem_config_finished = 0U;
        plat_log_i("4G factory config started, DTUID=%s",
                   (const char *)remote_modem_test_id);
    }
    return ret;
}

platform_err_t remote_modem_config_app_diagnostic_request(uint32_t now_ms)
{
    platform_err_t ret;

    if((0U != remote_modem_diagnostic_active) ||
       (0U != remote_modem_runtime_active) ||
       (0U != remote_modem_runtime_due) ||
       ((0U != remote_modem_config_requested) &&
        (0U == remote_modem_config_finished)))
    {
        return PLATFORM_ERR_BUSY;
    }
    ret = remote_modem_config_service_diagnostic_start(
        &remote_modem_config, now_ms);
    if(PLATFORM_ERR_OK == ret)
    {
        remote_modem_runtime_due = 0U;
        remote_modem_diagnostic_active = 1U;
        plat_log_i("4G read-only network diagnostic started");
    }
    return ret;
}

void remote_modem_config_app_poll(uint32_t now_ms)
{
    uint8_t rx_data[REMOTE_MODEM_CONFIG_RX_CHUNK_SIZE];
    uint16_t rx_size;
    uint8_t step;
    remote_modem_config_status_t status;
    platform_err_t ret;
    const uint8_t required_identity =
        REMOTE_MODEM_IDENTITY_IMEI_VALID |
        REMOTE_MODEM_IDENTITY_ICCID_VALID |
        REMOTE_MODEM_IDENTITY_DTUID_VALID;

    if((0U != remote_modem_runtime_due) &&
       (0U == remote_modem_runtime_active) &&
       (0U == remote_modem_diagnostic_active) &&
       ((0U == remote_modem_config_requested) ||
        (0U != remote_modem_config_finished)) &&
       ((int32_t)(now_ms - remote_modem_runtime_due_ms) >= 0))
    {
        remote_modem_runtime_due = 0U;
        ret = remote_modem_config_service_runtime_start(
            &remote_modem_config, now_ms);
        if(PLATFORM_ERR_OK == ret)
        {
            remote_modem_runtime_active = 1U;
            plat_log_i("4G normal startup check started");
        }
        else
        {
            plat_log_e("4G normal startup check start failed, ret=%d",
                       (int32_t)ret);
        }
    }

    if((0U == remote_modem_runtime_active) &&
       (0U == remote_modem_diagnostic_active) &&
       ((0U == remote_modem_config_requested) ||
        (0U != remote_modem_config_finished)))
    {
        return;
    }
    ret = remote_modem_config_service_poll(
        &remote_modem_config,
        now_ms,
        rx_data,
        (uint16_t)sizeof(rx_data),
        &rx_size,
        &status,
        &step);
    if(PLATFORM_ERR_OK != ret)
    {
        plat_log_e("4G %s failed at step=%u, ret=%d",
                   (0U != remote_modem_runtime_active) ? "runtime" :
                   ((0U != remote_modem_diagnostic_active) ?
                    "diagnostic" : "config"),
                   (unsigned int)step,
                   (int32_t)ret);
        if(0U != remote_modem_runtime_active)
        {
            remote_modem_runtime_active = 0U;
            remote_modem_runtime_verified = 0U;
            remote_modem_app_link_set(0U);
        }
        else if(0U != remote_modem_diagnostic_active)
        {
            remote_modem_diagnostic_active = 0U;
            remote_modem_app_link_set(0U);
        }
        else
        {
            remote_modem_config_finished = 1U;
        }
    }
    else if(REMOTE_MODEM_CONFIG_COMPLETE == status)
    {
        if(0U != remote_modem_runtime_active)
        {
            remote_modem_runtime_active = 0U;
            remote_modem_runtime_verified =
                (uint8_t)((0U != remote_modem_config.cpin_ready) &&
                (0U != remote_modem_config.signal_valid) &&
                ((remote_modem_config.identity.valid_mask &
                  required_identity) == required_identity));
            plat_log_i("4G startup CPIN=%u, CSQ=%u, identity mask=0x%02X, channel1=%u",
                       (unsigned int)remote_modem_config.cpin_ready,
                       (unsigned int)remote_modem_config.signal_value,
                       (unsigned int)remote_modem_config.identity.valid_mask,
                       (unsigned int)remote_modem_config.channel_connected);
            if(0U != (remote_modem_config.identity.valid_mask &
                      REMOTE_MODEM_IDENTITY_DTUID_VALID))
            {
                plat_log_i("4G RAM DTUID=%s",
                           remote_modem_config.identity.dtuid);
            }
            if(0U != remote_modem_runtime_verified)
            {
                remote_modem_app_link_set(
                    remote_modem_config.channel_connected);
            }
            else
            {
                remote_modem_app_link_set(0U);
            }
        }
        else if(0U != remote_modem_diagnostic_active)
        {
            plat_log_i("4G identity cached mask=0x%02X",
                       (unsigned int)remote_modem_config.identity.valid_mask);
            plat_log_i("4G network diagnostic complete; transparent mode restored");
            remote_modem_diagnostic_active = 0U;
            if(0U != remote_modem_runtime_verified)
            {
                remote_modem_app_link_set(
                    remote_modem_config.channel_connected);
            }
        }
        else
        {
            plat_log_i("4G config saved; restart sent");
            remote_modem_config_finished = 1U;
        }
    }
}

platform_err_t remote_modem_config_app_identity_get(
    remote_modem_identity_t *p_identity)
{
    if(NULL == p_identity)
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_identity = remote_modem_config.identity;
    return PLATFORM_ERR_OK;
}
