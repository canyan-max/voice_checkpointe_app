/** Task-owned remote communication and asynchronous configuration requests. */
#ifndef REMOTE_MODEM_APP_H
#define REMOTE_MODEM_APP_H
#include "remote_modem_service.h"
#define REMOTE_MODEM_APP_SEND_MAX_SIZE 256U
/* Runs in the modem task; borrowed bytes, no blocking or driver/Service calls.
 * No receive hook currently means bytes are counted and discarded, not
 * buffered. Future RS485 protocol handlers submit requests through the same App
 * API. */
typedef void (*remote_modem_receive_t)(void          *p_context,
                                       const uint8_t *p_data,
                                       uint16_t       size);
typedef struct
{
    remote_modem_receive_t receive;
    void                  *p_context;
    /* Temporary test: echo only newline-terminated test: lines, max 256 bytes.
     */
    uint8_t echo_test_enabled;
} remote_modem_app_io_t;
typedef struct
{
    remote_modem_status_t modem;
    uint32_t              received_bytes;
    uint32_t              stack_free_words;
    platform_err_t last_request_error; /* Dispatch/TX result; config flow result
                                          is above. */
    uint8_t busy;
} remote_modem_app_status_t;
/* Init from the system assembly after logging and LED initialization.
 * NULL io is valid until the business receive protocol is defined. */
platform_err_t remote_modem_app_init(const remote_modem_app_io_t *p_io);
/* Task-context only, nonblocking. OK means accepted, not configuration
 * complete. ID is copied into a fixed command slot; owner copies it into
 * Service state. Busy startup/config/recovery rejects new requests; a latched
 * fault permits an explicit configuration attempt, but never normal data
 * transmission. */
platform_err_t remote_modem_app_config_request(const uint8_t *p_id,
                                               uint16_t       size);
/* OK means accepted for owner transmission. No business TX queue during config.
 * The 256-byte API limit is a memory limit, not a business protocol frame size.
 */
platform_err_t remote_modem_app_send_request(const uint8_t *p_data,
                                             uint16_t       size);
platform_err_t remote_modem_app_status_get(remote_modem_app_status_t *p_status);
platform_err_t
remote_modem_app_identity_get(remote_modem_identity_t *p_identity);
#endif
