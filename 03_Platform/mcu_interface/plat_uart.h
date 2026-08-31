/**
 ******************************************************************************
 *@file               :   plat_uart.h
 *@brief              :   MCU UART abstraction — send / start DMA+IDLE receive /
 *                        register RX event callback, addressed by logical id.
 *                        Board wiring (id -> HAL handle) is resolved inside
 *                        the implementation.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PLAT_UART_H
#define PLAT_UART_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>     /* stdint lib header file. */
#include "plat_error.h" /* platform error code header file. */
/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef uint8_t plat_uart_id_t;

typedef enum
{
    PLAT_UART_RX_EVENT_NONE     = 0U,
    PLAT_UART_RX_EVENT_DATA     = (1U << 0),
    PLAT_UART_RX_EVENT_IDLE     = (1U << 1),
    PLAT_UART_RX_EVENT_OVERFLOW = (1U << 2),
    PLAT_UART_RX_EVENT_ERROR    = (1U << 3)
} plat_uart_rx_event_t;

typedef void (*plat_uart_rx_cb_t)(plat_uart_id_t       id,
                                  plat_uart_rx_event_t event,
                                  uint16_t             size);

/* Exported types -----------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
/**
  * @brief            : Send data through a logical UART in blocking mode.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_TIMEOUT.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : p_data Data buffer to send.
  * @param[in]        : size Number of bytes to send.
  * @param[in]        : timeout_ms Total send timeout in milliseconds; must be
  *                     greater than 0U.
  * @note             : Call from task context after UART initialization.
  *                     Concurrent callers must serialize access.
  */
platform_err_t plat_uart_send(plat_uart_id_t id,
                              const uint8_t *p_data,
                              uint16_t       size,
                              uint32_t       timeout_ms);

/**
  * @brief            : Start continuous DMA reception into a software FIFO.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_BUSY.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : p_buf Caller-owned FIFO storage buffer.
  * @param[in]        : buf_size FIFO size in bytes; must be a power of two.
  * @note             : The buffer remains owned by the driver after success.
  *                     Call once from task context after UART initialization.
  */
platform_err_t plat_uart_receive_start(plat_uart_id_t id,
                                       uint8_t       *p_buf,
                                       uint16_t       buf_size);

/**
  * @brief            : Read available bytes from the UART receive FIFO.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  * @param[in]        : id Logical UART ID.
  * @param[out]       : p_data Destination buffer.
  * @param[in]        : size Maximum number of bytes to read.
  * @param[out]       : p_read_size Actual number of bytes read.
  * @note             : A single task must be the only FIFO consumer.
  */
platform_err_t plat_uart_read(plat_uart_id_t id,
                              uint8_t       *p_data,
                              uint16_t       size,
                              uint16_t      *p_read_size);

/**
  * @brief            : Get the number of bytes currently in the receive FIFO.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  * @param[in]        : id Logical UART ID.
  * @param[out]       : p_size Number of available bytes.
  */
platform_err_t plat_uart_get_rx_size(plat_uart_id_t id, uint16_t *p_size);

/**
  * @brief            : Register the UART receive event callback.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : cb Callback invoked in interrupt context; NULL removes
  *                     the current callback.
  */
platform_err_t plat_uart_set_rx_callback(plat_uart_id_t    id,
                                         plat_uart_rx_cb_t cb);

/**
  * @brief            : Handle UART receive and error interrupts.
  * @param[in]        : id Logical UART ID.
  * @note             : Call only from the matching UART ISR.
  */
void plat_uart_rx_irq_handler(plat_uart_id_t id);

/**
  * @brief            : Handle UART RX DMA interrupts.
  * @param[in]        : id Logical UART ID.
  * @note             : Call only from the matching DMA ISR.
  */
void plat_uart_rx_dma_irq_handler(plat_uart_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_UART_H */
