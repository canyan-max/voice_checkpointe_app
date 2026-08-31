/**
 ******************************************************************************
 *@file               :   at32f435_gpio.c
 *@brief              :   Provide the HAL APIs of description.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "plat_gpio.h"
/* define   -----------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef struct AT32F435_GPIO_CFG_T
{
    gpio_type *p_port;
    uint16_t   pin;
} at32f435_gpio_cfg_t;

/* variables ----------------------------------------------------------------*/

static const at32f435_gpio_cfg_t gpio_cfg[BOARD_GPIO_RESOURCE_NUM] =
{
    [BOARD_GPIO_SW3] =
    {
        BOARD_GPIO_SW3_PORT,
        BOARD_GPIO_SW3_PIN
    },
    [BOARD_GPIO_SW4] =
    {
        BOARD_GPIO_SW4_PORT,
        BOARD_GPIO_SW4_PIN
    },
    [BOARD_GPIO_LED3] =
    {
        BOARD_GPIO_LED3_PORT,
        BOARD_GPIO_LED3_PIN
    },
    [BOARD_GPIO_LED4] =
    {
        BOARD_GPIO_LED4_PORT,
        BOARD_GPIO_LED4_PIN
    },
    [BOARD_GPIO_SD_CARD_DET] =
    {
        BOARD_GPIO_SD_CARD_DET_PORT,
        BOARD_GPIO_SD_CARD_DET_PIN
    }
};

_Static_assert((sizeof(gpio_cfg) / sizeof(gpio_cfg[0])) ==
                   BOARD_GPIO_RESOURCE_NUM,
               "GPIO resource table size mismatch");

/* private  functions  ------------------------------------------------------*/
static const at32f435_gpio_cfg_t *at32f435_gpio_get_cfg(plat_gpio_id_t id)
{
    if((uint32_t)id >= (uint32_t)BOARD_GPIO_RESOURCE_NUM)
    {
        return NULL;
    }

    return &gpio_cfg[id];
}
/* exported functions -------------------------------------------------------*/
platform_err_t plat_gpio_write(plat_gpio_id_t id, plat_gpio_state_t state)
{
    const at32f435_gpio_cfg_t *p_cfg = at32f435_gpio_get_cfg(id);
    confirm_state pin_state;

    if((NULL == p_cfg) ||
       ((PLAT_GPIO_RESET != state) && (PLAT_GPIO_SET != state)))
    {
        return PLATFORM_ERR_PARAM;
    }

    pin_state = (PLAT_GPIO_SET == state) ? TRUE : FALSE;
    gpio_bits_write(p_cfg->p_port, p_cfg->pin, pin_state);
    return PLATFORM_ERR_OK;
}

platform_err_t plat_gpio_toggle(plat_gpio_id_t id)
{
    const at32f435_gpio_cfg_t *p_cfg = at32f435_gpio_get_cfg(id);

    if(NULL == p_cfg)
    {
        return PLATFORM_ERR_PARAM;
    }

    gpio_bits_toggle(p_cfg->p_port, p_cfg->pin);
    return PLATFORM_ERR_OK;
}

platform_err_t plat_gpio_read(plat_gpio_id_t id, plat_gpio_state_t *p_state)
{
    const at32f435_gpio_cfg_t *p_cfg = at32f435_gpio_get_cfg(id);
    flag_status ret;

    if((NULL == p_cfg) || (NULL == p_state))
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = gpio_input_data_bit_read(p_cfg->p_port, p_cfg->pin);
    *p_state = (RESET == ret) ? PLAT_GPIO_RESET : PLAT_GPIO_SET;
    return PLATFORM_ERR_OK;
}



/* end of  file -------------------------------------------------------------*/
