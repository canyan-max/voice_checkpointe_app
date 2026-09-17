/**
 ******************************************************************************
 *@file               :   bsp_hub_display.c
 *@brief              :   Drive the board HUB display interface.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_resources.h"
#include "bsp_hub_display.h"
#include "plat_gpio.h"

/* define -------------------------------------------------------------------*/
#define BSP_HUB_DISPLAY_POWER_ON     PLAT_GPIO_RESET
#define BSP_HUB_DISPLAY_POWER_OFF    PLAT_GPIO_SET

/* variables ----------------------------------------------------------------*/
static uint8_t bsp_hub_display_initialized;
static uint8_t bsp_hub_display_started;

/* private functions --------------------------------------------------------*/
static platform_err_t bsp_hub_display_gpio_write(plat_gpio_id_t gpio_id,
                                                 plat_gpio_state_t state)
{
    return plat_gpio_write(gpio_id, state);
}

static void bsp_hub_display_bus_disable(void)
{
    (void)bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE,
                                     PLAT_GPIO_RESET);
    (void)bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE245,
                                     PLAT_GPIO_SET);
}

static void bsp_hub_display_safe_off(void)
{
    bsp_hub_display_bus_disable();
    (void)bsp_hub_display_gpio_write(BOARD_GPIO_LED_POWER_CS,
                                     BSP_HUB_DISPLAY_POWER_OFF);
    bsp_hub_display_started = 0U;
}

static platform_err_t bsp_hub_display_row_set(uint8_t row)
{
    platform_err_t ret;

    ret = bsp_hub_display_gpio_write(
        BOARD_GPIO_HUB_A,
        (0U != (row & 0x01U)) ? PLAT_GPIO_SET : PLAT_GPIO_RESET);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    return bsp_hub_display_gpio_write(
        BOARD_GPIO_HUB_B,
        (0U != (row & 0x02U)) ? PLAT_GPIO_SET : PLAT_GPIO_RESET);
}

static platform_err_t bsp_hub_display_data_write(uint8_t bit_is_set)
{
    platform_err_t ret;
    plat_gpio_state_t data_state;

    /* The vendor HUB12 panel uses active-low serial data. */
    data_state = (0U != bit_is_set) ? PLAT_GPIO_RESET : PLAT_GPIO_SET;
    ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_R1, data_state);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    return bsp_hub_display_gpio_write(BOARD_GPIO_HUB_G1, data_state);
}

/* exported functions -------------------------------------------------------*/
platform_err_t bsp_hub_display_init(void)
{
    static const plat_gpio_id_t low_level_gpio[] =
    {
        BOARD_GPIO_HUB_OE,
        BOARD_GPIO_HUB_SCK,
        BOARD_GPIO_HUB_LAT,
        BOARD_GPIO_HUB_A,
        BOARD_GPIO_HUB_B,
        BOARD_GPIO_HUB_C,
        BOARD_GPIO_HUB_D,
        BOARD_GPIO_HUB_E,
        BOARD_GPIO_HUB_R1,
        BOARD_GPIO_HUB_G1,
        BOARD_GPIO_HUB_B1,
        BOARD_GPIO_HUB_R2,
        BOARD_GPIO_HUB_G2,
        BOARD_GPIO_HUB_B2
    };
    platform_err_t ret;
    uint32_t gpio_index;

    bsp_hub_display_initialized = 0U;
    bsp_hub_display_started = 0U;
    ret = bsp_hub_display_gpio_write(BOARD_GPIO_LED_POWER_CS,
                                     BSP_HUB_DISPLAY_POWER_OFF);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE245,
                                         PLAT_GPIO_SET);
    }
    if(PLATFORM_ERR_OK != ret)
    {
        bsp_hub_display_safe_off();
        return ret;
    }

    for(gpio_index = 0U;
        gpio_index < (sizeof(low_level_gpio) / sizeof(low_level_gpio[0]));
        gpio_index++)
    {
        ret = bsp_hub_display_gpio_write(low_level_gpio[gpio_index],
                                         PLAT_GPIO_RESET);
        if(PLATFORM_ERR_OK != ret)
        {
            bsp_hub_display_safe_off();
            return ret;
        }
    }

    bsp_hub_display_initialized = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t bsp_hub_display_start(void)
{
    platform_err_t ret;

    if(0U == bsp_hub_display_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if(0U != bsp_hub_display_started)
    {
        return PLATFORM_ERR_OK;
    }

    ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE,
                                     PLAT_GPIO_RESET);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_LED_POWER_CS,
                                         BSP_HUB_DISPLAY_POWER_ON);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE245,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK != ret)
    {
        bsp_hub_display_safe_off();
        return ret;
    }

    bsp_hub_display_started = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t bsp_hub_display_scan_row(const uint8_t *p_data,
                                        uint16_t data_length,
                                        uint8_t row)
{
    platform_err_t ret;
    uint16_t byte_index;
    uint8_t bit_index;
    uint8_t data_byte;

    if((NULL == p_data) ||
       (BSP_HUB_DISPLAY_SCAN_ROW_BYTES != data_length) ||
       (BSP_HUB_DISPLAY_SCAN_ROW_COUNT <= row))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == bsp_hub_display_started)
    {
        return PLATFORM_ERR_BUSY;
    }

    for(byte_index = 0U; byte_index < data_length; byte_index++)
    {
        data_byte = p_data[byte_index];
        for(bit_index = 0U; bit_index < 8U; bit_index++)
        {
            ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_SCK,
                                             PLAT_GPIO_RESET);
            if(PLATFORM_ERR_OK != ret)
            {
                bsp_hub_display_safe_off();
                return ret;
            }

            ret = bsp_hub_display_data_write(
                (0U != (data_byte & 0x80U)) ? 1U : 0U);
            if(PLATFORM_ERR_OK != ret)
            {
                bsp_hub_display_safe_off();
                return ret;
            }
            data_byte <<= 1U;

            ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_SCK,
                                             PLAT_GPIO_SET);
            if(PLATFORM_ERR_OK != ret)
            {
                bsp_hub_display_safe_off();
                return ret;
            }
        }
    }

    ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE,
                                     PLAT_GPIO_RESET);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_LAT,
                                         PLAT_GPIO_SET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_LAT,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_row_set(row);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE,
                                         PLAT_GPIO_SET);
    }
    if(PLATFORM_ERR_OK != ret)
    {
        bsp_hub_display_safe_off();
    }
    return ret;
}

platform_err_t bsp_hub_display_stop(void)
{
    platform_err_t ret;
    platform_err_t disable_ret;

    ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE,
                                     PLAT_GPIO_RESET);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_R1,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_G1,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_SCK,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_LAT,
                                         PLAT_GPIO_RESET);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_row_set(0U);
    }

    disable_ret = bsp_hub_display_gpio_write(BOARD_GPIO_HUB_OE245,
                                             PLAT_GPIO_SET);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = disable_ret;
    }
    disable_ret = bsp_hub_display_gpio_write(BOARD_GPIO_LED_POWER_CS,
                                             BSP_HUB_DISPLAY_POWER_OFF);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = disable_ret;
    }
    bsp_hub_display_started = 0U;
    return ret;
}

/* end of file --------------------------------------------------------------*/
