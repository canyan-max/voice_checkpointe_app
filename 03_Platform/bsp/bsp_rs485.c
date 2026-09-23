/** Bind the board RS485 transceiver direction and USART1 receive path. */
#include <stddef.h>
#include "board_resources.h"
#include "bsp_rs485.h"
#include "plat_gpio.h"
#include "plat_uart.h"

static volatile uint32_t rs485_idle_sequence;
static volatile uint32_t rs485_error_sequence;
static uint32_t rs485_idle_handled;
static uint32_t rs485_error_handled;
static uint8_t rs485_initialized;

static void bsp_rs485_rx_callback(plat_uart_id_t id,
                                  plat_uart_rx_event_t event,
                                  uint16_t size)
{
    (void)size;
    if(BOARD_UART_RS485 != id)
    {
        return;
    }
    if(0U != ((uint32_t)event & (uint32_t)PLAT_UART_RX_EVENT_IDLE))
    {
        rs485_idle_sequence++;
    }
    if(0U != ((uint32_t)event &
              ((uint32_t)PLAT_UART_RX_EVENT_OVERFLOW |
               (uint32_t)PLAT_UART_RX_EVENT_ERROR)))
    {
        rs485_error_sequence++;
    }
}

platform_err_t bsp_rs485_init(void)
{
    platform_err_t ret;

    if(0U != rs485_initialized)
    {
        return PLATFORM_ERR_OK;
    }
    ret = plat_gpio_write(BOARD_GPIO_RS485_DIR, PLAT_GPIO_RESET);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    ret = plat_uart_set_rx_callback(BOARD_UART_RS485,
                                    bsp_rs485_rx_callback);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    ret = plat_uart_receive_start(BOARD_UART_RS485);
    if(PLATFORM_ERR_OK != ret)
    {
        (void)plat_uart_set_rx_callback(BOARD_UART_RS485, NULL);
        return ret;
    }
    rs485_idle_handled = rs485_idle_sequence;
    rs485_error_handled = rs485_error_sequence;
    rs485_initialized = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t bsp_rs485_poll(uint8_t *p_data,
                              uint16_t capacity,
                              uint16_t *p_size)
{
    platform_err_t ret;
    uint16_t available;
    uint16_t discarded;

    if((NULL == p_data) || (0U == capacity) || (NULL == p_size))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    if(0U == rs485_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if((rs485_idle_handled == rs485_idle_sequence) &&
       (rs485_error_handled == rs485_error_sequence))
    {
        return PLATFORM_ERR_OK;
    }
    rs485_idle_handled = rs485_idle_sequence;
    ret = plat_uart_get_rx_size(BOARD_UART_RS485, &available);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    if((rs485_error_handled != rs485_error_sequence) ||
       (available > capacity))
    {
        rs485_error_handled = rs485_error_sequence;
        while(available > 0U)
        {
            uint16_t chunk = (available > capacity) ? capacity : available;
            ret = plat_uart_read(BOARD_UART_RS485,
                                 p_data,
                                 chunk,
                                 &discarded);
            if((PLATFORM_ERR_OK != ret) || (0U == discarded))
            {
                return PLATFORM_ERR_HW;
            }
            available = (uint16_t)(available - discarded);
        }
        return PLATFORM_ERR_HW;
    }
    if(0U == available)
    {
        return PLATFORM_ERR_OK;
    }
    ret = plat_uart_read(BOARD_UART_RS485, p_data, available, p_size);
    if(rs485_error_handled != rs485_error_sequence)
    {
        rs485_error_handled = rs485_error_sequence;
        *p_size = 0U;
        return PLATFORM_ERR_HW;
    }
    return ret;
}

platform_err_t bsp_rs485_send(const uint8_t *p_data,
                              uint16_t size,
                              uint32_t timeout_ms)
{
    platform_err_t ret;
    platform_err_t direction_ret;

    if((NULL == p_data) || (0U == size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == rs485_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    ret = plat_gpio_write(BOARD_GPIO_RS485_DIR, PLAT_GPIO_SET);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    ret = plat_uart_send(BOARD_UART_RS485, p_data, size, timeout_ms);
    direction_ret = plat_gpio_write(BOARD_GPIO_RS485_DIR, PLAT_GPIO_RESET);
    return (PLATFORM_ERR_OK == direction_ret) ? ret : direction_ret;
}
