/** Board half-duplex RS485 capability. */
#ifndef BSP_RS485_H
#define BSP_RS485_H

#include <stdint.h>
#include "plat_error.h"

platform_err_t bsp_rs485_init(void);

/* Return one idle-delimited receive burst; size is zero until UART idle. */
platform_err_t bsp_rs485_poll(uint8_t *p_data,
                              uint16_t capacity,
                              uint16_t *p_size);

/* Switch to TX, send through UART completion, then restore RX even on error. */
platform_err_t bsp_rs485_send(const uint8_t *p_data,
                              uint16_t size,
                              uint32_t timeout_ms);

#endif /* BSP_RS485_H */
