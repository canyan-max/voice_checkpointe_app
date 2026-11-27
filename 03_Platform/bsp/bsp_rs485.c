/** Bind the board RS485 transceiver direction and USART1 receive path. */
#include <stddef.h>
#include "board_resources.h"
#include "bsp_rs485.h"
#include "plat_gpio.h"
#include "plat_uart.h"

static volatile uint32_t rs485_error_sequence;
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
    rs485_error_handled = rs485_error_sequence;
    rs485_initialized = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t bsp_rs485_poll(uint8_t *p_data,
                              uint16_t capacity,
                              uint16_t *p_size)
{
    if((NULL == p_data) || (0U == capacity) || (NULL == p_size))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    if(0U == rs485_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    uint32_t error_sequence = rs485_error_sequence;
    if (rs485_error_handled != error_sequence) {
        uint16_t available = 0U;
        platform_err_t ret = plat_uart_get_rx_size(BOARD_UART_RS485, &available);
        if (PLATFORM_ERR_OK != ret) {
            return ret;
        }
        while (0U < available) {
            uint16_t chunk = (available > capacity) ? capacity : available;
            uint16_t discarded = 0U;
            ret = plat_uart_read(BOARD_UART_RS485,
                                 p_data,
                                 chunk,
                                 &discarded);
            if ((PLATFORM_ERR_OK != ret) || (0U == discarded)) {
                return PLATFORM_ERR_HW;
            }
            available = (uint16_t)(available - discarded);
        }
        rs485_error_handled = error_sequence;
        return PLATFORM_ERR_HW;
    }
    platform_err_t ret = plat_uart_read(BOARD_UART_RS485, p_data, capacity, p_size);
    if (error_sequence != rs485_error_sequence) {
        *p_size = 0U;
        return PLATFORM_ERR_HW;
    }
    return ret;
}

platform_err_t bsp_rs485_get_rx_progress(uint16_t *p_progress) {
    return plat_uart_get_rx_progress(BOARD_UART_RS485, p_progress);
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
