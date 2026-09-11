/**
 ******************************************************************************
 *@file               :   bsp_led.c
 *@brief              :   Bind logical LEDs to board GPIOs and polarities.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include "board_resources.h"
#include "bsp_led.h"
#include "plat_gpio.h"

/* typedef ------------------------------------------------------------------*/
typedef struct BSP_LED_CONFIG_T
{
    plat_gpio_id_t    gpio_id;
    plat_gpio_state_t active_level;
} bsp_led_config_t;

/* variables ----------------------------------------------------------------*/
static const bsp_led_config_t bsp_led_config[BSP_LED_COUNT] =
{
    [BSP_LED_REMOTE] =
        {BOARD_GPIO_LED1_REMOTE, PLAT_GPIO_RESET},
    [BSP_LED_UPDATE] =
        {BOARD_GPIO_LED2_UPDATA, PLAT_GPIO_RESET},
    [BSP_LED_BROADCAST] =
        {BOARD_GPIO_LED3_BRCAT, PLAT_GPIO_RESET},
    [BSP_LED_RUN] =
        {BOARD_GPIO_LED4_RUN, PLAT_GPIO_RESET},
    [BSP_LED_OUTPUT_BLUE] =
        {BOARD_GPIO_LED_OUT_B, PLAT_GPIO_SET},
    [BSP_LED_OUTPUT_RED] =
        {BOARD_GPIO_LED_OUT_R, PLAT_GPIO_SET}
};

/* exported functions -------------------------------------------------------*/
platform_err_t bsp_led_init(void)
{
    return bsp_led_all_off();
}

platform_err_t bsp_led_set(bsp_led_id_t led_id, bsp_led_state_t state)
{
    plat_gpio_state_t gpio_state;

    if(((uint32_t)led_id >= (uint32_t)BSP_LED_COUNT) ||
       ((BSP_LED_OFF != state) && (BSP_LED_ON != state)))
    {
        return PLATFORM_ERR_PARAM;
    }

    if(BSP_LED_ON == state)
    {
        gpio_state = bsp_led_config[led_id].active_level;
    }
    else
    {
        gpio_state = (PLAT_GPIO_SET == bsp_led_config[led_id].active_level) ?
                     PLAT_GPIO_RESET : PLAT_GPIO_SET;
    }
    return plat_gpio_write(bsp_led_config[led_id].gpio_id, gpio_state);
}

platform_err_t bsp_led_all_off(void)
{
    bsp_led_id_t led_id;
    platform_err_t ret;

    for(led_id = BSP_LED_REMOTE; led_id < BSP_LED_COUNT; led_id++)
    {
        ret = bsp_led_set(led_id, BSP_LED_OFF);
        if(PLATFORM_ERR_OK != ret)
        {
            return ret;
        }
    }
    return PLATFORM_ERR_OK;
}

/* end of file --------------------------------------------------------------*/

