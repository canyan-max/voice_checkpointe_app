/** Board remote communication capability; no chip protocol types escape. */
#ifndef BSP_REMOTE_MODEM_H
#define BSP_REMOTE_MODEM_H
#include <stdint.h>
#include "plat_error.h"
#define BSP_REMOTE_MODEM_DEVICE_ID_SIZE 15U
#define BSP_REMOTE_MODEM_SIM_ID_SIZE 20U
#define BSP_REMOTE_MODEM_SUBSCRIBER_ID_SIZE 15U
#define BSP_REMOTE_MODEM_REGISTRATION_MAX_SIZE 128U
#define BSP_REMOTE_MODEM_ID_DEVICE       (1U << 0)
#define BSP_REMOTE_MODEM_ID_SIM          (1U << 1)
#define BSP_REMOTE_MODEM_ID_SUBSCRIBER   (1U << 2)
#define BSP_REMOTE_MODEM_ID_REGISTRATION (1U << 3)
typedef enum
{
    BSP_REMOTE_MODEM_CONFIG_ENTER = 0,
    BSP_REMOTE_MODEM_CONFIG_EXIT,
    BSP_REMOTE_MODEM_CONFIG_SAVE,
    BSP_REMOTE_MODEM_RESTART,
    BSP_REMOTE_MODEM_SIM_QUERY,
    BSP_REMOTE_MODEM_DEVICE_ID_QUERY,
    BSP_REMOTE_MODEM_SIM_ID_QUERY,
    BSP_REMOTE_MODEM_SUBSCRIBER_ID_QUERY,
    BSP_REMOTE_MODEM_REGISTRATION_QUERY,
    BSP_REMOTE_MODEM_CONNECTION_QUERY,
    BSP_REMOTE_MODEM_TRANSPARENT_ENABLE,
    BSP_REMOTE_MODEM_FRAME_CONVERSION_DISABLE,
    BSP_REMOTE_MODEM_DATA_ENCODING_DISABLE,
    BSP_REMOTE_MODEM_HEARTBEAT_SET,
    BSP_REMOTE_MODEM_REGISTRATION_SET,
    BSP_REMOTE_MODEM_SERVER_SET,
    BSP_REMOTE_MODEM_SECONDARY_DISABLE,
    BSP_REMOTE_MODEM_SECURITY_DISABLE,
    BSP_REMOTE_MODEM_STATUS_ENABLE,
    BSP_REMOTE_MODEM_OPERATION_COUNT
} bsp_remote_modem_operation_t;
/* Text is borrowed only for command_start. Channel 1 is the board's link. */
typedef struct
{
    bsp_remote_modem_operation_t operation;
    const uint8_t               *p_text;
    uint16_t                     size;
    uint16_t                     interval_seconds;
    uint16_t                     port;
} bsp_remote_modem_request_t;
typedef struct
{
    char    device_id[BSP_REMOTE_MODEM_DEVICE_ID_SIZE + 1U];
    char    sim_id[BSP_REMOTE_MODEM_SIM_ID_SIZE + 1U];
    char    subscriber_id[BSP_REMOTE_MODEM_SUBSCRIBER_ID_SIZE + 1U];
    char    registration[BSP_REMOTE_MODEM_REGISTRATION_MAX_SIZE + 1U];
    uint8_t valid_mask;
} bsp_remote_modem_identity_t;
#define BSP_REMOTE_MODEM_EVENT_READY        (1U << 0)
#define BSP_REMOTE_MODEM_EVENT_COMMAND_DONE (1U << 1)
#define BSP_REMOTE_MODEM_EVENT_LINK         (1U << 2)
#define BSP_REMOTE_MODEM_EVENT_UNREGISTERED (1U << 3)
typedef struct
{
    uint8_t                     events;
    platform_err_t              command_result;
    bsp_remote_modem_identity_t identity; /* Staged; commit only after OK. */
    uint8_t                     sim_ready;
    uint8_t                     connection_present;
    uint8_t                     connected;
} bsp_remote_modem_report_t;
/* DMA only; generated GPIO code already enables power. No reset/reload guesses.
 */
platform_err_t bsp_remote_modem_init(void);
platform_err_t
bsp_remote_modem_command_start(const bsp_remote_modem_request_t *p_request,
                               uint32_t                          now_ms,
                               uint32_t send_timeout_ms,
                               uint32_t reply_timeout_ms);
/* Single owner. Raw bytes remain intact; events include the whole RX chunk. */
platform_err_t bsp_remote_modem_poll(uint32_t                   now_ms,
                                     uint8_t                   *p_data,
                                     uint16_t                   capacity,
                                     uint16_t                  *p_size,
                                     bsp_remote_modem_report_t *p_report);
platform_err_t bsp_remote_modem_data_send(const uint8_t *p_data,
                                          uint16_t       size,
                                          uint32_t       timeout_ms);
/* Forget parser session after error; does not reset hardware or UART. */
platform_err_t bsp_remote_modem_session_reset(void);
#endif
