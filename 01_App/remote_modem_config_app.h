/** Temporary board entry for one remote modem factory configuration. */
#ifndef REMOTE_MODEM_CONFIG_APP_H
#define REMOTE_MODEM_CONFIG_APP_H

#include <stdint.h>
#include "remote_modem_config_service.h"

platform_err_t remote_modem_config_app_request(uint32_t now_ms);
platform_err_t remote_modem_config_app_diagnostic_request(uint32_t now_ms);
void remote_modem_config_app_poll(uint32_t now_ms);
platform_err_t remote_modem_config_app_monitor_start(void);
void remote_modem_config_app_monitor_poll(uint32_t now_ms);
/* Call from the modem owner task; inspect valid_mask before using fields. */
platform_err_t remote_modem_config_app_identity_get(
    remote_modem_identity_t *p_identity);

#endif /* REMOTE_MODEM_CONFIG_APP_H */
