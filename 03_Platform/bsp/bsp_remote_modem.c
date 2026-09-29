/** Bind the board remote modem to its UART receive path. */
#include <stddef.h>
#include "board_resources.h"
#include "bsp_remote_modem.h"
#include "plat_gpio.h"
#include "plat_uart.h"

static uint8_t remote_modem_rx_started;

platform_err_t bsp_remote_modem_receive_start(void)
{
    platform_err_t ret;

    if(0U != remote_modem_rx_started)
    {
        return PLATFORM_ERR_OK;
    }
    ret = plat_uart_receive_start(BOARD_UART_REMOTE_4G);
    if(PLATFORM_ERR_OK == ret)
    {
        remote_modem_rx_started = 1U;
    }
    return ret;
}

platform_err_t bsp_remote_modem_power_enable(void)
{
    if(0U == remote_modem_rx_started)
    {
        return PLATFORM_ERR_HW;
    }
    return plat_gpio_write(BOARD_GPIO_E27_POWEREN, PLAT_GPIO_SET);
}

platform_err_t bsp_remote_modem_send(const uint8_t *p_data,
                                     uint16_t size,
                                     uint32_t timeout_ms)
{
    if((0U == remote_modem_rx_started) || (NULL == p_data) ||
       (0U == size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    return plat_uart_send(BOARD_UART_REMOTE_4G,
                          p_data,
                          size,
                          timeout_ms);
}

platform_err_t bsp_remote_modem_poll(uint8_t *p_data,
                                     uint16_t capacity,
                                     uint16_t *p_size)
{
    platform_err_t ret;
    uint16_t available;

    if((NULL == p_data) || (0U == capacity) || (NULL == p_size))
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_size = 0U;
    if(0U == remote_modem_rx_started)
    {
        return PLATFORM_ERR_HW;
    }
    ret = plat_uart_get_rx_size(BOARD_UART_REMOTE_4G, &available);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    if(available > capacity)
    {
        available = capacity;
    }
    if(0U == available)
    {
        return PLATFORM_ERR_OK;
    }
    return plat_uart_read(BOARD_UART_REMOTE_4G,
                          p_data,
                          available,
                          p_size);
}
