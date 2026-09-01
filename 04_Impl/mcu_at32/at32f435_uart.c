/**
 ******************************************************************************
 *@file               :   at32f435_uart.c
 *@brief              :   Provide the AT32F435 UART platform implementation.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "kfifo.h"
#include "plat_sys.h"
#include "plat_uart.h"
#include "wk_dma.h"
#include "wk_usart.h"

/* define   -----------------------------------------------------------------*/
#define AT32F435_UART_RX_DMA_SIZE    512U

/* typedef ------------------------------------------------------------------*/
typedef struct AT32F435_UART_RX_T
{
    kfifo_t              fifo;
    plat_uart_rx_cb_t    callback;
    uint16_t             dma_last_pos;
    uint8_t              is_started;
} at32f435_uart_rx_t;

typedef struct AT32F435_UART_CFG_T
{
    usart_type       *p_uart;
    dma_channel_type *p_rx_dma;
    uint32_t          rx_dma_global_flag;
    uint32_t          rx_dma_half_flag;
    uint32_t          rx_dma_full_flag;
    uint32_t          rx_dma_error_flag;
} at32f435_uart_cfg_t;

/* variables ----------------------------------------------------------------*/
static const at32f435_uart_cfg_t uart_cfg[BOARD_UART_RESOURCE_NUM] =
{
    [BOARD_UART_PROTOCOL] =
    {
        BOARD_UART_PROTOCOL_INSTANCE,
        BOARD_UART_PROTOCOL_RX_DMA,
        BOARD_UART_PROTOCOL_RX_DMA_GLOBAL,
        BOARD_UART_PROTOCOL_RX_DMA_HALF,
        BOARD_UART_PROTOCOL_RX_DMA_FULL,
        BOARD_UART_PROTOCOL_RX_DMA_ERROR
    }
};

static uint32_t uart_rx_dma_storage[AT32F435_UART_RX_DMA_SIZE / sizeof(uint32_t)];
static at32f435_uart_rx_t uart_rx_table[BOARD_UART_RESOURCE_NUM];

_Static_assert((sizeof(uart_cfg) / sizeof(uart_cfg[0])) ==
                   BOARD_UART_RESOURCE_NUM,
               "UART resource table size mismatch");

/* private  functions  ------------------------------------------------------*/
/**
  * @brief            : Wait until a UART status flag is set.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_TIMEOUT.
  * @param[in]        : p_uart AT32 UART instance.
  * @param[in]        : flag UART status flag.
  * @param[in]        : start_tick Send start tick in milliseconds.
  * @param[in]        : timeout_ms Total send timeout in milliseconds.
  */
static platform_err_t at32f435_uart_wait_flag(usart_type *p_uart,
                                              uint32_t    flag,
                                              uint32_t    start_tick,
                                              uint32_t    timeout_ms)
{
    while(RESET == usart_flag_get(p_uart, flag))
    {
        if((plat_tick_get_ms() - start_tick) >= timeout_ms)
        {
            return PLATFORM_ERR_TIMEOUT;
        }
    }

    return PLATFORM_ERR_OK;
}

/**
  * @brief            : Notify the registered UART callback from an ISR.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : event Receive event flags.
  * @param[in]        : size Number of bytes added to the FIFO.
  */
static void at32f435_uart_rx_notify(plat_uart_id_t       id,
                                    plat_uart_rx_event_t event,
                                    uint16_t             size)
{
    if(NULL != uart_rx_table[id].callback)
    {
        uart_rx_table[id].callback(id, event, size);
    }
}

/**
  * @brief            : Copy newly received circular DMA data into the FIFO.
  * @retval           : Receive event flags produced by the synchronization.
  * @param[in]        : id Logical UART ID.
  * @param[out]       : p_write_size Number of bytes added to the FIFO.
  */
static plat_uart_rx_event_t
at32f435_uart_rx_sync(plat_uart_id_t id, uint16_t *p_write_size)
{
    const at32f435_uart_cfg_t *p_cfg = &uart_cfg[id];
    at32f435_uart_rx_t *p_rx = &uart_rx_table[id];
    uint8_t *p_dma_buf = (uint8_t *)uart_rx_dma_storage;
    uint16_t dma_pos;
    uint16_t first_size;
    uint16_t second_size;
    uint32_t write_size;
    plat_uart_rx_event_t event = PLAT_UART_RX_EVENT_NONE;

    dma_pos = (uint16_t)(AT32F435_UART_RX_DMA_SIZE -
                         dma_data_number_get(p_cfg->p_rx_dma));
    if(dma_pos >= AT32F435_UART_RX_DMA_SIZE)
    {
        dma_pos = 0U;
    }

    if(dma_pos >= p_rx->dma_last_pos)
    {
        first_size = (uint16_t)(dma_pos - p_rx->dma_last_pos);
        second_size = 0U;
    }
    else
    {
        first_size = (uint16_t)(AT32F435_UART_RX_DMA_SIZE -
                                p_rx->dma_last_pos);
        second_size = dma_pos;
    }

    write_size = kfifo_put(&p_rx->fifo,
                           &p_dma_buf[p_rx->dma_last_pos],
                           first_size);
    if(write_size < first_size)
    {
        event = (plat_uart_rx_event_t)(event |
                                       PLAT_UART_RX_EVENT_OVERFLOW);
    }
    *p_write_size = (uint16_t)write_size;

    if(second_size > 0U)
    {
        write_size = kfifo_put(&p_rx->fifo, p_dma_buf, second_size);
        if(write_size < second_size)
        {
            event = (plat_uart_rx_event_t)(event |
                                           PLAT_UART_RX_EVENT_OVERFLOW);
        }
        *p_write_size = (uint16_t)(*p_write_size + (uint16_t)write_size);
    }

    if(*p_write_size > 0U)
    {
        event = (plat_uart_rx_event_t)(event | PLAT_UART_RX_EVENT_DATA);
    }
    p_rx->dma_last_pos = dma_pos;
    return event;
}

/* exported functions -------------------------------------------------------*/
/**
  * @brief            : Send data through a logical UART in blocking mode.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_TIMEOUT.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : p_data Data buffer to send.
  * @param[in]        : size Number of bytes to send.
  * @param[in]        : timeout_ms Total send timeout in milliseconds.
  */
platform_err_t plat_uart_send(plat_uart_id_t id,
                              const uint8_t *p_data,
                              uint16_t       size,
                              uint32_t       timeout_ms)
{
    const at32f435_uart_cfg_t *p_cfg;
    usart_type *p_uart;
    uint32_t    start_tick;
    uint16_t    index;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (NULL == p_data) ||
       (0U == size) ||
       (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_cfg = &uart_cfg[id];
    p_uart = p_cfg->p_uart;
    start_tick = plat_tick_get_ms();

    for(index = 0U; index < size; index++)
    {
        if(PLATFORM_ERR_OK != at32f435_uart_wait_flag(p_uart,
                                                      USART_TDBE_FLAG,
                                                      start_tick,
                                                      timeout_ms))
        {
            return PLATFORM_ERR_TIMEOUT;
        }

        usart_data_transmit(p_uart, (uint16_t)p_data[index]);
    }

    return at32f435_uart_wait_flag(p_uart,
                                   USART_TDC_FLAG,
                                   start_tick,
                                   timeout_ms);
}

/**
  * @brief            : Start continuous DMA reception into a software FIFO.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or
  *                     PLATFORM_ERR_BUSY.
  * @param[in]        : id Logical UART ID.
  * @param[in]        : p_buf Caller-owned FIFO storage buffer.
  * @param[in]        : buf_size FIFO size in bytes; must be a power of two.
  */
platform_err_t plat_uart_receive_start(plat_uart_id_t id,
                                       uint8_t       *p_buf,
                                       uint16_t       buf_size)
{
    const at32f435_uart_cfg_t *p_cfg;
    at32f435_uart_rx_t *p_rx;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (NULL == p_buf) ||
       (0U == buf_size))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_cfg = &uart_cfg[id];
    p_rx = &uart_rx_table[id];
    if(0U != p_rx->is_started)
    {
        return PLATFORM_ERR_BUSY;
    }
    if(0U != kfifo_init(&p_rx->fifo, p_buf, buf_size))
    {
        return PLATFORM_ERR_PARAM;
    }

    dma_channel_enable(p_cfg->p_rx_dma, FALSE);
    dma_flag_clear(p_cfg->rx_dma_global_flag);
    wk_dma_channel_config(p_cfg->p_rx_dma,
                          (uint32_t)&p_cfg->p_uart->dt,
                          (uint32_t)uart_rx_dma_storage,
                          AT32F435_UART_RX_DMA_SIZE);
    p_rx->dma_last_pos = 0U;
    p_rx->is_started = 1U;
    dma_channel_enable(p_cfg->p_rx_dma, TRUE);
    return PLATFORM_ERR_OK;
}

/**
  * @brief            : Read available bytes from the UART receive FIFO.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  */
platform_err_t plat_uart_read(plat_uart_id_t id,
                              uint8_t       *p_data,
                              uint16_t       size,
                              uint16_t      *p_read_size)
{
    uint32_t read_size;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (NULL == p_data) ||
       (0U == size) ||
       (NULL == p_read_size) ||
       (0U == uart_rx_table[id].is_started))
    {
        return PLATFORM_ERR_PARAM;
    }

    read_size = kfifo_get(&uart_rx_table[id].fifo, p_data, size);
    *p_read_size = (uint16_t)read_size;
    return PLATFORM_ERR_OK;
}

/**
  * @brief            : Get the number of bytes in the UART receive FIFO.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  */
platform_err_t plat_uart_get_rx_size(plat_uart_id_t id, uint16_t *p_size)
{
    uint32_t rx_size;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (NULL == p_size) ||
       (0U == uart_rx_table[id].is_started))
    {
        return PLATFORM_ERR_PARAM;
    }

    rx_size = kfifo_len(&uart_rx_table[id].fifo);
    *p_size = (uint16_t)rx_size;
    return PLATFORM_ERR_OK;
}

/**
  * @brief            : Register the UART receive event callback.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_PARAM.
  */
platform_err_t plat_uart_set_rx_callback(plat_uart_id_t    id,
                                         plat_uart_rx_cb_t cb)
{
    if((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM)
    {
        return PLATFORM_ERR_PARAM;
    }

    uart_rx_table[id].callback = cb;
    return PLATFORM_ERR_OK;
}

/**
  * @brief            : Handle UART receive and error interrupts.
  * @param[in]        : id Logical UART ID.
  */
void plat_uart_rx_irq_handler(plat_uart_id_t id)
{
    const at32f435_uart_cfg_t *p_cfg;
    plat_uart_rx_event_t event;
    uint16_t write_size = 0U;
    flag_status error_status;
    flag_status idle_status;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (0U == uart_rx_table[id].is_started))
    {
        return;
    }

    p_cfg = &uart_cfg[id];
    error_status = ((SET == usart_interrupt_flag_get(p_cfg->p_uart,
                                                      USART_NERR_FLAG)) ||
                    (SET == usart_interrupt_flag_get(p_cfg->p_uart,
                                                      USART_ROERR_FLAG)) ||
                    (SET == usart_interrupt_flag_get(p_cfg->p_uart,
                                                      USART_FERR_FLAG))) ?
                   SET : RESET;
    idle_status = usart_interrupt_flag_get(p_cfg->p_uart, USART_IDLEF_FLAG);

    if((SET == error_status) || (SET == idle_status))
    {
        usart_flag_clear(p_cfg->p_uart,
                         USART_NERR_FLAG |
                         USART_ROERR_FLAG |
                         USART_FERR_FLAG |
                         USART_IDLEF_FLAG);
        event = at32f435_uart_rx_sync(id, &write_size);
        if(SET == error_status)
        {
            event = (plat_uart_rx_event_t)(event |
                                           PLAT_UART_RX_EVENT_ERROR);
        }
        if(SET == idle_status)
        {
            event = (plat_uart_rx_event_t)(event |
                                           PLAT_UART_RX_EVENT_IDLE);
        }
        at32f435_uart_rx_notify(id, event, write_size);
    }
}

/**
  * @brief            : Handle UART RX DMA interrupts.
  * @param[in]        : id Logical UART ID.
  */
void plat_uart_rx_dma_irq_handler(plat_uart_id_t id)
{
    const at32f435_uart_cfg_t *p_cfg;
    plat_uart_rx_event_t event;
    uint16_t write_size = 0U;
    flag_status data_status;
    flag_status error_status;

    if(((uint32_t)id >= (uint32_t)BOARD_UART_RESOURCE_NUM) ||
       (0U == uart_rx_table[id].is_started))
    {
        if((uint32_t)id < (uint32_t)BOARD_UART_RESOURCE_NUM)
        {
            dma_flag_clear(uart_cfg[id].rx_dma_global_flag);
        }
        return;
    }

    p_cfg = &uart_cfg[id];
    data_status = ((SET == dma_interrupt_flag_get(p_cfg->rx_dma_half_flag)) ||
                   (SET == dma_interrupt_flag_get(p_cfg->rx_dma_full_flag))) ?
                  SET : RESET;
    error_status = dma_interrupt_flag_get(p_cfg->rx_dma_error_flag);

    if((SET == data_status) || (SET == error_status))
    {
        event = at32f435_uart_rx_sync(id, &write_size);
        if(SET == error_status)
        {
            event = (plat_uart_rx_event_t)(event |
                                           PLAT_UART_RX_EVENT_ERROR);
        }
        dma_flag_clear(p_cfg->rx_dma_global_flag);
        at32f435_uart_rx_notify(id, event, write_size);
    }
}

/* end of  file -------------------------------------------------------------*/
