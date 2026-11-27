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
#include "plat_resource.h"
/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
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

typedef void (*plat_uart_tx_done_cb_t)(plat_uart_id_t id);

/* Exported types -----------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
/**
  * @brief            : Send data through a logical UART in blocking mode.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM, PLATFORM_ERR_BUSY
  *                     or PLATFORM_ERR_TIMEOUT.
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
 * @brief Start an interrupt-driven byte transfer after UART initialization.
 * @retval PLATFORM_ERR_OK if accepted, PLATFORM_ERR_PARAM for invalid arguments,
 *         or PLATFORM_ERR_BUSY if this UART is already transmitting.
 * @note Call from task context. The driver borrows p_data without copying it.
 *       Keep it valid and unchanged until the callback or a successful abort.
 *       The required callback runs once in the UART ISR after the last stop bit.
 *       The caller owns the timeout policy and serializes start/abort for its transfer.
 */
platform_err_t plat_uart_send_it(plat_uart_id_t         id,
                                 const uint8_t         *p_data,
                                 uint16_t               size,
                                 plat_uart_tx_done_cb_t pf_done);

/**
 * @brief Send two consecutive buffers as one interrupt-driven transfer.
 * @note Same contract as plat_uart_send_it(); both buffers are borrowed.
 *       The second buffer is optional when second_size is zero.
 */
platform_err_t plat_uart_send_parts_it(plat_uart_id_t         id,
                                       const uint8_t         *p_first,
                                       uint16_t               first_size,
                                       const uint8_t         *p_second,
                                       uint16_t               second_size,
                                       plat_uart_tx_done_cb_t pf_done);

/**
 * @brief Stop an interrupt-driven transfer and release its buffer.
 * @retval PLATFORM_ERR_OK if stopped or idle, PLATFORM_ERR_PARAM for an invalid
 *         ID, or PLATFORM_ERR_BUSY if a blocking send owns the UART.
 * @note Call from the owning task. No callback is issued for an aborted transfer.
 *       Bytes already loaded into the UART may still go out on the wire.
 */
platform_err_t plat_uart_send_it_abort(plat_uart_id_t id);

/**
  * @brief            : Start continuous reception into driver-owned storage.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_BUSY.
  * @param[in]        : id Logical UART ID.
  * @note             : The MCU implementation owns the circular DMA buffer.
  *                     Call once from task context after UART initialization.
  */
platform_err_t plat_uart_receive_start(plat_uart_id_t id);

/**
  * @brief            : Read available bytes from the UART receive FIFO.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_HW if DMA overflowed during the read.
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

/* Snapshot an opaque RX progress token, including bytes not yet readable.
 * The token can wrap; compare successive samples for equality only. */
platform_err_t plat_uart_get_rx_progress(plat_uart_id_t id, uint16_t *p_progress);

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

/* Call only from the matching UART ISR. */
void plat_uart_tx_irq_handler(plat_uart_id_t id);

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
