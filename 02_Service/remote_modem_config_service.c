/** Issue factory AT settings one at a time and verify terminal replies. */
#include <stddef.h>
#include <string.h>
#include "bsp_remote_modem.h"
#include "plat_log.h"
#include "remote_modem_config_service.h"

#define REMOTE_MODEM_CONFIG_SEND_TIMEOUT_MS   500U
#define REMOTE_MODEM_CONFIG_REPLY_TIMEOUT_MS  3000U
#define REMOTE_MODEM_ALL_REPLY_TIMEOUT_MS    10000U
#define REMOTE_MODEM_CONFIG_REBOOT_STEP          11U
#define REMOTE_MODEM_CONFIG_ID_STEP               5U
#define REMOTE_MODEM_DIAGNOSTIC_COMMAND_COUNT     8U
#define REMOTE_MODEM_DIAGNOSTIC_ALL_STEP           6U
#define REMOTE_MODEM_RUNTIME_COMMAND_COUNT         9U
#define REMOTE_MODEM_RUNTIME_IMEI_STEP             2U
#define REMOTE_MODEM_RUNTIME_ICCID_STEP            3U
#define REMOTE_MODEM_RUNTIME_IMSI_STEP             4U
#define REMOTE_MODEM_RUNTIME_DTUID_STEP            5U
#define REMOTE_MODEM_RUNTIME_CSQ_STEP              6U
#define REMOTE_MODEM_RUNTIME_CONNECT_STEP          7U
#define REMOTE_MODEM_CONFIG_ID_PREFIX  "AT+DTUID=1,0,0,\""
#define REMOTE_MODEM_CONFIG_ID_SUFFIX  "\",1\r\n"
#define REMOTE_MODEM_DTUID_REPLY_PREFIX "+DTUID: 1,0,0,\""

/* Step 5 is assembled from the validated external ID. Step 11 reboots. */
static const char *const remote_modem_commands[] =
{
    "+++",
    "AT+DTUMODE=1,1\r\n",
    "AT+TCPMODBUS=0,1\r\n",
    "AT+TCPHEX=0,1\r\n",
    "AT+KEEPALIVE=60,1,\"7777772E7573722E636E\",1\r\n",
    NULL, /* DTUID command is assembled from the validated input. */
    "AT+DSCADDR=1,\"tcp\",\"62195.cnsh.iot-tcp.com\",15000\r\n",
    "AT+SECSERVER=0,0\r\n",
    "AT+MQTTSSL=0,1\r\n",
    "AT+AUTOSTATUS=3,1\r\n",
    "AT&W\r\n",
    "AT+CFUN=1,1\r\n"
};

static const char *const remote_modem_diagnostic_commands[] =
{
    "+++",
    "AT+ASKNET?\r\n",
    "AT+ASKCONNECT?\r\n",
    "AT+CSQ\r\n",
    "AT+DSCADDR?\r\n",
    "AT+DTUMODE?\r\n",
    "AT+ALL?\r\n",
    "ATO\r\n"
};

static const char *const remote_modem_runtime_commands[] =
{
    "+++",
    "AT+CPIN?\r\n",
    "AT+GSN?\r\n",
    "AT+ICCID\r\n",
    "AT+IMSI\r\n",
    "AT+DTUID?\r\n",
    "AT+CSQ\r\n",
    "AT+ASKCONNECT?\r\n",
    "ATO\r\n"
};

_Static_assert((sizeof(remote_modem_commands) /
                sizeof(remote_modem_commands[0])) ==
               (REMOTE_MODEM_CONFIG_REBOOT_STEP + 1U),
               "Remote modem command table size mismatch");
_Static_assert((sizeof(remote_modem_diagnostic_commands) /
                sizeof(remote_modem_diagnostic_commands[0])) ==
               REMOTE_MODEM_DIAGNOSTIC_COMMAND_COUNT,
               "Remote modem diagnostic command table size mismatch");
_Static_assert((sizeof(remote_modem_runtime_commands) /
                sizeof(remote_modem_runtime_commands[0])) ==
               REMOTE_MODEM_RUNTIME_COMMAND_COUNT,
               "Remote modem runtime command table size mismatch");

static uint8_t remote_modem_id_is_valid(const uint8_t *p_id,
                                        uint16_t id_size)
{
    uint16_t index;

    if((NULL == p_id) || (REMOTE_MODEM_CONFIG_ID_SIZE != id_size))
    {
        return 0U;
    }
    for(index = 0U; index < id_size; index++)
    {
        if(!(((p_id[index] >= '0') && (p_id[index] <= '9')) ||
             ((p_id[index] >= 'A') && (p_id[index] <= 'Z')) ||
             ((p_id[index] >= 'a') && (p_id[index] <= 'z'))))
        {
            return 0U;
        }
    }
    return 1U;
}

static void remote_modem_identity_store(remote_modem_config_service_t *p_service,
                                        const char *p_digits,
                                        size_t min_size,
                                        size_t max_size,
                                        char *p_target,
                                        uint8_t valid_bit)
{
    size_t index;
    size_t size = strlen(p_digits);

    if((size < min_size) || (size > max_size))
    {
        return;
    }
    for(index = 0U; index < size; index++)
    {
        if((p_digits[index] < '0') || (p_digits[index] > '9'))
        {
            return;
        }
    }
    memcpy(p_target, p_digits, size + 1U);
    p_service->identity.valid_mask |= valid_bit;
}

static void remote_modem_identity_line_complete(
    remote_modem_config_service_t *p_service)
{
    const char *p_line = p_service->identity_line;
    const char *p_id;
    size_t index;
    unsigned int signal;

    if((0U == p_service->identity_line_length) ||
       (0U != p_service->identity_line_overflow))
    {
        return;
    }
    p_service->identity_line[p_service->identity_line_length] = '\0';
    if(2U == p_service->diagnostic_mode)
    {
        if(0 == strcmp(p_line, "+STATUS: 1, CONNECTED"))
        {
            p_service->channel_connected = 1U;
        }
        else if((0 == strcmp(p_line, "+STATUS: 1, CLOSED")) ||
                (0 == strcmp(p_line,
                             "+STATUS:NET STATE UNREGISTER")))
        {
            p_service->channel_connected = 0U;
        }
    }
    if((2U == p_service->diagnostic_mode) &&
       (REMOTE_MODEM_RUNTIME_IMEI_STEP == p_service->step))
    {
        remote_modem_identity_store(p_service, p_line,
                                    REMOTE_MODEM_IMEI_SIZE,
                                    REMOTE_MODEM_IMEI_SIZE,
                                    p_service->identity.imei,
                                    REMOTE_MODEM_IDENTITY_IMEI_VALID);
    }
    if(((1U == p_service->diagnostic_mode) &&
        (REMOTE_MODEM_DIAGNOSTIC_ALL_STEP == p_service->step)) &&
       (0 == strncmp(p_line, "+IMEI: ", 7U)))
    {
        remote_modem_identity_store(p_service, p_line + 7U,
                                    REMOTE_MODEM_IMEI_SIZE,
                                    REMOTE_MODEM_IMEI_SIZE,
                                    p_service->identity.imei,
                                    REMOTE_MODEM_IDENTITY_IMEI_VALID);
    }
    if((((2U == p_service->diagnostic_mode) &&
         (REMOTE_MODEM_RUNTIME_ICCID_STEP == p_service->step)) ||
        ((1U == p_service->diagnostic_mode) &&
         (REMOTE_MODEM_DIAGNOSTIC_ALL_STEP == p_service->step))) &&
       (0 == strncmp(p_line, "+ICCID: ", 8U)))
    {
        remote_modem_identity_store(p_service, p_line + 8U,
                                    19U, REMOTE_MODEM_ICCID_MAX_SIZE,
                                    p_service->identity.iccid,
                                    REMOTE_MODEM_IDENTITY_ICCID_VALID);
    }
    if((((2U == p_service->diagnostic_mode) &&
         (REMOTE_MODEM_RUNTIME_IMSI_STEP == p_service->step)) ||
        ((1U == p_service->diagnostic_mode) &&
         (REMOTE_MODEM_DIAGNOSTIC_ALL_STEP == p_service->step))) &&
       (0 == strncmp(p_line, "+IMSI: ", 7U)))
    {
        remote_modem_identity_store(p_service, p_line + 7U,
                                    REMOTE_MODEM_IMSI_SIZE,
                                    REMOTE_MODEM_IMSI_SIZE,
                                    p_service->identity.imsi,
                                    REMOTE_MODEM_IDENTITY_IMSI_VALID);
    }
    if((1U == p_service->diagnostic_mode) &&
       (2U == p_service->step) &&
       (0 == strcmp(p_line, "+ASKCONNECT: 1,0")))
    {
        p_service->channel_connected = 1U;
    }
    if(2U != p_service->diagnostic_mode)
    {
        return;
    }
    if((1U == p_service->step) &&
       (0 == strcmp(p_line, "+CPIN: READY")))
    {
        p_service->cpin_ready = 1U;
    }
    else if((REMOTE_MODEM_RUNTIME_DTUID_STEP == p_service->step) &&
            (p_service->identity_line_length ==
             (sizeof(REMOTE_MODEM_DTUID_REPLY_PREFIX) - 1U +
              REMOTE_MODEM_CONFIG_ID_SIZE + 3U)) &&
            (0 == strncmp(p_line, REMOTE_MODEM_DTUID_REPLY_PREFIX,
                          sizeof(REMOTE_MODEM_DTUID_REPLY_PREFIX) - 1U)))
    {
        p_id = p_line + sizeof(REMOTE_MODEM_DTUID_REPLY_PREFIX) - 1U;
        if((0U != remote_modem_id_is_valid((const uint8_t *)p_id,
                                            REMOTE_MODEM_CONFIG_ID_SIZE)) &&
           (p_id[REMOTE_MODEM_CONFIG_ID_SIZE] == '"') &&
           (0 == strcmp(p_id + REMOTE_MODEM_CONFIG_ID_SIZE + 1U, ",1")))
        {
            memcpy(p_service->identity.dtuid, p_id,
                   REMOTE_MODEM_CONFIG_ID_SIZE);
            p_service->identity.dtuid[REMOTE_MODEM_CONFIG_ID_SIZE] = '\0';
            p_service->identity.valid_mask |= REMOTE_MODEM_IDENTITY_DTUID_VALID;
        }
    }
    else if((REMOTE_MODEM_RUNTIME_CSQ_STEP == p_service->step) &&
            (0 == strncmp(p_line, "+CSQ: ", 6U)))
    {
        signal = 0U;
        index = 6U;
        if((p_line[index] < '0') || (p_line[index] > '9'))
        {
            return;
        }
        while((p_line[index] >= '0') && (p_line[index] <= '9') &&
              (signal < 100U))
        {
            signal = (signal * 10U) + (unsigned int)(p_line[index] - '0');
            index++;
        }
        if((',' == p_line[index]) && (signal >= 1U) && (signal <= 31U))
        {
            p_service->signal_value = (uint8_t)signal;
            p_service->signal_valid = 1U;
        }
    }
    else if((REMOTE_MODEM_RUNTIME_CONNECT_STEP == p_service->step) &&
            (0 == strcmp(p_line, "+ASKCONNECT: 1,0")))
    {
        p_service->channel_connected = 1U;
    }
}

static void remote_modem_identity_byte(remote_modem_config_service_t *p_service,
                                       uint8_t byte)
{
    if(('\r' == byte) || ('\n' == byte))
    {
        remote_modem_identity_line_complete(p_service);
        p_service->identity_line_length = 0U;
        p_service->identity_line_overflow = 0U;
        return;
    }
    if(p_service->identity_line_length <
       (sizeof(p_service->identity_line) - 1U))
    {
        p_service->identity_line[p_service->identity_line_length++] =
            (char)byte;
    }
    else
    {
        p_service->identity_line_overflow = 1U;
    }
}

static void remote_modem_identity_query_discard(
    remote_modem_config_service_t *p_service)
{
    if(2U == p_service->diagnostic_mode)
    {
        if(1U == p_service->step)
        {
            p_service->cpin_ready = 0U;
        }
        else if(REMOTE_MODEM_RUNTIME_CSQ_STEP == p_service->step)
        {
            p_service->signal_valid = 0U;
        }
        else if(REMOTE_MODEM_RUNTIME_CONNECT_STEP == p_service->step)
        {
            p_service->channel_connected = 0U;
        }
        else if(REMOTE_MODEM_RUNTIME_IMEI_STEP == p_service->step)
        {
            p_service->identity.imei[0] = '\0';
            p_service->identity.valid_mask &=
                (uint8_t)~REMOTE_MODEM_IDENTITY_IMEI_VALID;
        }
        else if(REMOTE_MODEM_RUNTIME_ICCID_STEP == p_service->step)
        {
            p_service->identity.iccid[0] = '\0';
            p_service->identity.valid_mask &=
                (uint8_t)~REMOTE_MODEM_IDENTITY_ICCID_VALID;
        }
        else if(REMOTE_MODEM_RUNTIME_IMSI_STEP == p_service->step)
        {
            p_service->identity.imsi[0] = '\0';
            p_service->identity.valid_mask &=
                (uint8_t)~REMOTE_MODEM_IDENTITY_IMSI_VALID;
        }
        else if(REMOTE_MODEM_RUNTIME_DTUID_STEP == p_service->step)
        {
            p_service->identity.dtuid[0] = '\0';
            p_service->identity.valid_mask &=
                (uint8_t)~REMOTE_MODEM_IDENTITY_DTUID_VALID;
        }
    }
}

platform_err_t remote_modem_config_service_monitor_start(void)
{
    return bsp_remote_modem_receive_start();
}

platform_err_t remote_modem_config_service_monitor_poll(uint8_t *p_data,
                                                        uint16_t capacity,
                                                        uint16_t *p_size)
{
    return bsp_remote_modem_poll(p_data, capacity, p_size);
}

static platform_err_t remote_modem_send_step(
    const remote_modem_config_service_t *p_service)
{
    uint8_t command[sizeof(REMOTE_MODEM_CONFIG_ID_PREFIX) - 1U +
                    REMOTE_MODEM_CONFIG_ID_SIZE +
                    sizeof(REMOTE_MODEM_CONFIG_ID_SUFFIX) - 1U];
    uint16_t size;
    const char *p_fixed;

    if((0U == p_service->diagnostic_mode) &&
       (REMOTE_MODEM_CONFIG_ID_STEP == p_service->step))
    {
        size = 0U;
        memcpy(&command[size],
               REMOTE_MODEM_CONFIG_ID_PREFIX,
               sizeof(REMOTE_MODEM_CONFIG_ID_PREFIX) - 1U);
        size += (uint16_t)(sizeof(REMOTE_MODEM_CONFIG_ID_PREFIX) - 1U);
        memcpy(&command[size],
               p_service->id,
               REMOTE_MODEM_CONFIG_ID_SIZE);
        size += REMOTE_MODEM_CONFIG_ID_SIZE;
        memcpy(&command[size],
               REMOTE_MODEM_CONFIG_ID_SUFFIX,
               sizeof(REMOTE_MODEM_CONFIG_ID_SUFFIX) - 1U);
        size += (uint16_t)(sizeof(REMOTE_MODEM_CONFIG_ID_SUFFIX) - 1U);
        plat_log_i("4G TX step=%u", (unsigned int)p_service->step);
        plat_log_hexdump("4G TX", 16U, command, size);
        return bsp_remote_modem_send(command,
                                     size,
                                     REMOTE_MODEM_CONFIG_SEND_TIMEOUT_MS);
    }
    if(2U == p_service->diagnostic_mode)
    {
        p_fixed = remote_modem_runtime_commands[p_service->step];
    }
    else if(1U == p_service->diagnostic_mode)
    {
        p_fixed = remote_modem_diagnostic_commands[p_service->step];
    }
    else
    {
        p_fixed = remote_modem_commands[p_service->step];
    }
    size = (uint16_t)strlen(p_fixed);
    plat_log_i("4G TX step=%u", (unsigned int)p_service->step);
    plat_log_hexdump("4G TX", 16U, p_fixed, size);
    return bsp_remote_modem_send((const uint8_t *)p_fixed,
                                 size,
                                 REMOTE_MODEM_CONFIG_SEND_TIMEOUT_MS);
}

/* Return 1 for a standalone OK line, 2 for ERROR, otherwise 0. */
static uint8_t remote_modem_response_byte(
    remote_modem_config_service_t *p_service,
    uint8_t byte)
{
    uint8_t result = 0U;

    if(('\r' == byte) || ('\n' == byte))
    {
        if((2U == p_service->line_length) &&
           ('O' == p_service->line_prefix[0]) &&
           ('K' == p_service->line_prefix[1]))
        {
            result = 1U;
        }
        else if(((5U == p_service->line_length) &&
                 (0 == memcmp(p_service->line_prefix, "ERROR", 5U))) ||
                ((p_service->line_length >= 10U) &&
                 ((0 == memcmp(p_service->line_prefix,
                              "+CME ERROR", 10U)) ||
                  (0 == memcmp(p_service->line_prefix,
                              "+CMS ERROR", 10U)))))
        {
            result = 2U;
        }
        p_service->line_length = 0U;
        return result;
    }
    if(p_service->line_length < sizeof(p_service->line_prefix))
    {
        p_service->line_prefix[p_service->line_length] = byte;
    }
    if(p_service->line_length <= sizeof(p_service->line_prefix))
    {
        p_service->line_length++;
    }
    return 0U;
}

platform_err_t remote_modem_config_service_start(
    remote_modem_config_service_t *p_service,
    const uint8_t *p_id,
    uint16_t id_size,
    uint32_t now_ms)
{
    platform_err_t ret;

    if((NULL == p_service) ||
       (0U == remote_modem_id_is_valid(p_id, id_size)) ||
       (REMOTE_MODEM_CONFIG_RUNNING == p_service->status))
    {
        return PLATFORM_ERR_PARAM;
    }
    ret = bsp_remote_modem_receive_start();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    memcpy(p_service->id, p_id, REMOTE_MODEM_CONFIG_ID_SIZE);
    p_service->step = 0U;
    p_service->diagnostic_mode = 0U;
    p_service->line_length = 0U;
    p_service->identity_line_length = 0U;
    p_service->identity_line_overflow = 0U;
    memset(&p_service->identity, 0, sizeof(p_service->identity));
    ret = remote_modem_send_step(p_service);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    p_service->step_start_ms = now_ms;
    p_service->status = REMOTE_MODEM_CONFIG_RUNNING;
    return PLATFORM_ERR_OK;
}

platform_err_t remote_modem_config_service_diagnostic_start(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms)
{
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }
    ret = bsp_remote_modem_receive_start();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    p_service->step = 0U;
    p_service->diagnostic_mode = 1U;
    p_service->line_length = 0U;
    p_service->identity_line_length = 0U;
    p_service->identity_line_overflow = 0U;
    p_service->channel_connected = 0U;
    ret = remote_modem_send_step(p_service);
    if(PLATFORM_ERR_OK != ret)
    {
        p_service->status = REMOTE_MODEM_CONFIG_FAILED;
        return ret;
    }
    p_service->step_start_ms = now_ms;
    p_service->status = REMOTE_MODEM_CONFIG_RUNNING;
    return PLATFORM_ERR_OK;
}

platform_err_t remote_modem_config_service_runtime_start(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms)
{
    platform_err_t ret;

    if((NULL == p_service) ||
       (REMOTE_MODEM_CONFIG_RUNNING == p_service->status))
    {
        return PLATFORM_ERR_PARAM;
    }
    ret = bsp_remote_modem_receive_start();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    p_service->step = 0U;
    p_service->diagnostic_mode = 2U;
    p_service->line_length = 0U;
    p_service->identity_line_length = 0U;
    p_service->identity_line_overflow = 0U;
    p_service->cpin_ready = 0U;
    p_service->signal_valid = 0U;
    p_service->channel_connected = 0U;
    p_service->signal_value = 0U;
    memset(&p_service->identity, 0, sizeof(p_service->identity));
    ret = remote_modem_send_step(p_service);
    if(PLATFORM_ERR_OK != ret)
    {
        p_service->status = REMOTE_MODEM_CONFIG_FAILED;
        return ret;
    }
    p_service->step_start_ms = now_ms;
    p_service->status = REMOTE_MODEM_CONFIG_RUNNING;
    return PLATFORM_ERR_OK;
}

platform_err_t remote_modem_config_service_poll(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms,
    uint8_t *p_data,
    uint16_t capacity,
    uint16_t *p_size,
    remote_modem_config_status_t *p_status,
    uint8_t *p_step)
{
    platform_err_t ret;
    uint16_t index;
    uint8_t terminal;

    if((NULL == p_service) || (NULL == p_data) || (0U == capacity) ||
       (NULL == p_size) || (NULL == p_status) || (NULL == p_step) ||
       (REMOTE_MODEM_CONFIG_RUNNING != p_service->status))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    *p_step = p_service->step;
    ret = bsp_remote_modem_poll(p_data, capacity, p_size);
    if(PLATFORM_ERR_OK != ret)
    {
        remote_modem_identity_query_discard(p_service);
        p_service->status = REMOTE_MODEM_CONFIG_FAILED;
        *p_status = p_service->status;
        return ret;
    }
    if(*p_size > 0U)
    {
        plat_log_i("4G RX step=%u", (unsigned int)p_service->step);
        plat_log_hexdump("4G RX", 16U, p_data, *p_size);
    }
    for(index = 0U; index < *p_size; index++)
    {
        remote_modem_identity_byte(p_service, p_data[index]);
        if(REMOTE_MODEM_CONFIG_RUNNING != p_service->status)
        {
            continue;
        }
        terminal = remote_modem_response_byte(p_service, p_data[index]);
        if(2U == terminal)
        {
            remote_modem_identity_query_discard(p_service);
            if((0U != p_service->diagnostic_mode) &&
               (0U != p_service->step) &&
               (p_service->step <
                ((1U == p_service->diagnostic_mode) ?
                 (REMOTE_MODEM_DIAGNOSTIC_COMMAND_COUNT - 1U) :
                 (REMOTE_MODEM_RUNTIME_COMMAND_COUNT - 1U))))
            {
                plat_log_w("4G query error at step=%u; continuing",
                           (unsigned int)p_service->step);
                p_service->step++;
                p_service->line_length = 0U;
                ret = remote_modem_send_step(p_service);
                if(PLATFORM_ERR_OK != ret)
                {
                    p_service->status = REMOTE_MODEM_CONFIG_FAILED;
                    *p_status = p_service->status;
                    return ret;
                }
                p_service->step_start_ms = now_ms;
                break;
            }
            p_service->status = REMOTE_MODEM_CONFIG_FAILED;
            *p_status = p_service->status;
            return PLATFORM_ERR_HW;
        }
        if(1U == terminal)
        {
            p_service->step++;
            p_service->line_length = 0U;
            if(((1U == p_service->diagnostic_mode) &&
                (REMOTE_MODEM_DIAGNOSTIC_COMMAND_COUNT == p_service->step)) ||
               ((2U == p_service->diagnostic_mode) &&
                (REMOTE_MODEM_RUNTIME_COMMAND_COUNT == p_service->step)))
            {
                p_service->status = REMOTE_MODEM_CONFIG_COMPLETE;
                if(2U == p_service->diagnostic_mode)
                {
                    /* A status URC may follow ATO's OK in the same DMA read. */
                    continue;
                }
                break;
            }
            ret = remote_modem_send_step(p_service);
            if(PLATFORM_ERR_OK != ret)
            {
                p_service->status = REMOTE_MODEM_CONFIG_FAILED;
                *p_status = p_service->status;
                return ret;
            }
            if((0U == p_service->diagnostic_mode) &&
               (REMOTE_MODEM_CONFIG_REBOOT_STEP == p_service->step))
            {
                /* The module may reset before replying to AT+CFUN. */
                p_service->status = REMOTE_MODEM_CONFIG_COMPLETE;
            }
            else
            {
                p_service->step_start_ms = now_ms;
            }
            break;
        }
    }
    if((REMOTE_MODEM_CONFIG_RUNNING == p_service->status) &&
       ((now_ms - p_service->step_start_ms) >=
         (((1U == p_service->diagnostic_mode) &&
          (REMOTE_MODEM_DIAGNOSTIC_ALL_STEP == p_service->step)) ?
         REMOTE_MODEM_ALL_REPLY_TIMEOUT_MS :
         REMOTE_MODEM_CONFIG_REPLY_TIMEOUT_MS)))
    {
        remote_modem_identity_query_discard(p_service);
        p_service->identity_line_length = 0U;
        p_service->identity_line_overflow = 0U;
        if((0U != p_service->diagnostic_mode) &&
           (0U != p_service->step) &&
           (p_service->step <
            ((1U == p_service->diagnostic_mode) ?
             (REMOTE_MODEM_DIAGNOSTIC_COMMAND_COUNT - 1U) :
             (REMOTE_MODEM_RUNTIME_COMMAND_COUNT - 1U))))
        {
            plat_log_w("4G query timeout at step=%u; continuing",
                       (unsigned int)p_service->step);
            p_service->step++;
            p_service->line_length = 0U;
            ret = remote_modem_send_step(p_service);
            if(PLATFORM_ERR_OK != ret)
            {
                p_service->status = REMOTE_MODEM_CONFIG_FAILED;
                *p_status = p_service->status;
                return ret;
            }
            p_service->step_start_ms = now_ms;
            *p_status = p_service->status;
            return PLATFORM_ERR_OK;
        }
        p_service->status = REMOTE_MODEM_CONFIG_FAILED;
        *p_status = p_service->status;
        return PLATFORM_ERR_TIMEOUT;
    }
    *p_status = p_service->status;
    return PLATFORM_ERR_OK;
}
