/** Bounded factory configuration and diagnostic of the remote modem. */
#ifndef REMOTE_MODEM_CONFIG_SERVICE_H
#define REMOTE_MODEM_CONFIG_SERVICE_H

#include <stdint.h>
#include "plat_error.h"

#define REMOTE_MODEM_CONFIG_ID_SIZE  16U
#define REMOTE_MODEM_IMEI_SIZE       15U
#define REMOTE_MODEM_ICCID_MAX_SIZE  20U
#define REMOTE_MODEM_IMSI_SIZE       15U

#define REMOTE_MODEM_IDENTITY_IMEI_VALID   (1U << 0)
#define REMOTE_MODEM_IDENTITY_ICCID_VALID  (1U << 1)
#define REMOTE_MODEM_IDENTITY_IMSI_VALID   (1U << 2)
#define REMOTE_MODEM_IDENTITY_DTUID_VALID  (1U << 3)

typedef struct
{
    char imei[REMOTE_MODEM_IMEI_SIZE + 1U];
    char iccid[REMOTE_MODEM_ICCID_MAX_SIZE + 1U];
    char imsi[REMOTE_MODEM_IMSI_SIZE + 1U];
    char dtuid[REMOTE_MODEM_CONFIG_ID_SIZE + 1U];
    uint8_t valid_mask;
} remote_modem_identity_t;

typedef enum
{
    REMOTE_MODEM_CONFIG_IDLE = 0U,
    REMOTE_MODEM_CONFIG_RUNNING,
    REMOTE_MODEM_CONFIG_COMPLETE,
    REMOTE_MODEM_CONFIG_FAILED
} remote_modem_config_status_t;

typedef struct
{
    uint32_t step_start_ms;
    uint8_t id[REMOTE_MODEM_CONFIG_ID_SIZE];
    uint8_t line_prefix[10];
    uint8_t line_length;
    char identity_line[40];
    uint8_t identity_line_length;
    uint8_t identity_line_overflow;
    uint8_t step;
    uint8_t diagnostic_mode;
    uint8_t cpin_ready;
    uint8_t signal_valid;
    uint8_t channel_connected;
    uint8_t signal_value;
    remote_modem_identity_t identity;
    remote_modem_config_status_t status;
} remote_modem_config_service_t;

platform_err_t remote_modem_config_service_start(
    remote_modem_config_service_t *p_service,
    const uint8_t *p_id,
    uint16_t id_size,
    uint32_t now_ms);
platform_err_t remote_modem_config_service_diagnostic_start(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms);
platform_err_t remote_modem_config_service_runtime_start(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms);

platform_err_t remote_modem_config_service_monitor_start(void);
platform_err_t remote_modem_config_service_monitor_poll(uint8_t *p_data,
                                                        uint16_t capacity,
                                                        uint16_t *p_size);

platform_err_t remote_modem_config_service_poll(
    remote_modem_config_service_t *p_service,
    uint32_t now_ms,
    uint8_t *p_data,
    uint16_t capacity,
    uint16_t *p_size,
    remote_modem_config_status_t *p_status,
    uint8_t *p_step);

#endif /* REMOTE_MODEM_CONFIG_SERVICE_H */
