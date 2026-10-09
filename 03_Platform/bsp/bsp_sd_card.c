#include <stddef.h>
#include "bsp_sd_card.h"
#include "board_resources.h"
#include "plat_gpio.h"

platform_err_t bsp_sd_card_is_present(uint8_t *p_present)
{
    plat_gpio_state_t state;
    platform_err_t result;

    if(NULL == p_present)
    {
        return PLATFORM_ERR_PARAM;
    }
    result = plat_gpio_read(BOARD_GPIO_TF_CD, &state);
    if(PLATFORM_ERR_OK == result)
    {
        *p_present = (PLAT_GPIO_RESET == state) ? 1U : 0U;
    }
    return result;
}
