/**
 ******************************************************************************
 *@file               :   plat_gpio.h
 *@brief              :   MCU GPIO abstraction — write, read, toggle by
 *                        logical board GPIO ID.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef PLAT_GPIO_H
#define PLAT_GPIO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h" /* platform error code header file. */

typedef uint8_t plat_gpio_id_t;

typedef enum
{
    PLAT_GPIO_RESET = 0,
    PLAT_GPIO_SET,
    PLAT_GPIO_RESERVED = 0x7FFFFFFF
} plat_gpio_state_t;

platform_err_t plat_gpio_write(plat_gpio_id_t id, plat_gpio_state_t state);
platform_err_t plat_gpio_read(plat_gpio_id_t id,
                              plat_gpio_state_t *p_state);
platform_err_t plat_gpio_toggle(plat_gpio_id_t id);
#ifdef __cplusplus
}
#endif

#endif /* PLAT_GPIO_H */
