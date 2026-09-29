/** Board UART capability for the remote modem. */
#ifndef BSP_REMOTE_MODEM_H
#define BSP_REMOTE_MODEM_H

#include <stdint.h>
#include "plat_error.h"

platform_err_t bsp_remote_modem_receive_start(void);
platform_err_t bsp_remote_modem_power_enable(void);
platform_err_t bsp_remote_modem_send(const uint8_t *p_data,
                                     uint16_t size,
                                     uint32_t timeout_ms);
platform_err_t bsp_remote_modem_poll(uint8_t *p_data,
                                     uint16_t capacity,
                                     uint16_t *p_size);

#endif /* BSP_REMOTE_MODEM_H */
