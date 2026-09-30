/** Remote modem business flows, independent of RTOS and chip protocol. */
#ifndef REMOTE_MODEM_SERVICE_H
#define REMOTE_MODEM_SERVICE_H
#include <stdint.h>
#include "plat_error.h"
#define REMOTE_MODEM_ID_SIZE 16U
#define REMOTE_MODEM_IMEI_SIZE 15U
#define REMOTE_MODEM_ICCID_SIZE 20U
#define REMOTE_MODEM_IMSI_SIZE 15U
#define REMOTE_MODEM_IDENTITY_IMEI  (1U << 0)
#define REMOTE_MODEM_IDENTITY_ICCID (1U << 1)
#define REMOTE_MODEM_IDENTITY_IMSI  (1U << 2)
#define REMOTE_MODEM_IDENTITY_ID    (1U << 3)
typedef struct
{
    char    imei[REMOTE_MODEM_IMEI_SIZE + 1U];
    char    iccid[REMOTE_MODEM_ICCID_SIZE + 1U];
    char    imsi[REMOTE_MODEM_IMSI_SIZE + 1U];
    char    id[REMOTE_MODEM_ID_SIZE + 1U];
    uint8_t valid_mask;
} remote_modem_identity_t;
typedef enum
{
    REMOTE_MODEM_MODE_CONFIGURATION = 0,
    REMOTE_MODEM_MODE_NORMAL
} remote_modem_mode_t;
typedef enum
{
    REMOTE_MODEM_STAGE_BOOT_WAIT = 0,
    REMOTE_MODEM_STAGE_QUIET,
    REMOTE_MODEM_STAGE_STARTUP_READ,
    REMOTE_MODEM_STAGE_WRITE_CONFIG,
    REMOTE_MODEM_STAGE_RESTART_WAIT,
    REMOTE_MODEM_STAGE_RECOVER_ENTER,
    REMOTE_MODEM_STAGE_RECOVER_EXIT,
    REMOTE_MODEM_STAGE_NORMAL,
    REMOTE_MODEM_STAGE_FAULT
} remote_modem_stage_t;
typedef enum
{
    REMOTE_MODEM_CONFIG_NONE = 0,
    REMOTE_MODEM_CONFIG_RUNNING,
    REMOTE_MODEM_CONFIG_SUCCEEDED,
    REMOTE_MODEM_CONFIG_FAILED
} remote_modem_config_result_t;
typedef struct
{
    remote_modem_mode_t          mode;
    remote_modem_stage_t         stage;
    remote_modem_config_result_t config_result;
    remote_modem_identity_t      identity;
    platform_err_t               last_error;
    remote_modem_stage_t         failed_stage;
    uint8_t                      failed_step;
    uint8_t                      step;
    uint8_t                      connected;
} remote_modem_status_t;
/* Caller owns this context and calls every function from the modem owner.
 * Identity is cached in RAM after each query OK; no persistent storage implied.
 */
typedef struct
{
    remote_modem_status_t status;
    uint8_t               requested_id[REMOTE_MODEM_ID_SIZE];
    uint32_t              stage_start_ms;
    uint32_t              last_rx_ms;
    uint32_t              flow_start_ms;
    uint8_t               initialized;
    uint8_t               pending;
    uint8_t               recovery_attempted;
    uint8_t               quiet_target;
    uint8_t               boot_ready;
} remote_modem_service_t;
uint8_t remote_modem_service_id_is_valid(const uint8_t *p_id, uint16_t size);
platform_err_t remote_modem_service_init(remote_modem_service_t *p_service,
                                         uint32_t                now_ms);
platform_err_t
remote_modem_service_config_start(remote_modem_service_t *p_service,
                                  const uint8_t          *p_id,
                                  uint16_t                size,
                                  uint32_t                now_ms);
/* p_size >0 only for normal-mode raw RX; configuration RX is discarded after
 * parsing. Snapshot getters are the App's responsibility for other tasks. */
platform_err_t remote_modem_service_poll(remote_modem_service_t *p_service,
                                         uint32_t                now_ms,
                                         uint8_t                *p_data,
                                         uint16_t                capacity,
                                         uint16_t               *p_size);
platform_err_t remote_modem_service_data_send(remote_modem_service_t *p_service,
                                              const uint8_t          *p_data,
                                              uint16_t                size,
                                              uint32_t                now_ms);
#endif
