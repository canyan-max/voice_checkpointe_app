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

#include "plat_error.h" /* platform error code header file. */
#include "plat_resource.h"

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

/* Called in ISR context for each serviced hardware edge. Edges occurring
 * before a pending flag is cleared may coalesce. Do not block or log here. */
typedef void (*plat_gpio_irq_cb_t)(plat_gpio_id_t id);

/* Only interrupt-capable GPIO IDs are accepted. Call after generated EXINT
 * initialization, from a single serialized task/startup owner.
 * Disable the line before changing its callback (otherwise ERR_BUSY).
 * NULL unregisters it. Callback storage is owned by the implementation. */
platform_err_t plat_gpio_set_irq_callback(plat_gpio_id_t id,
                                         plat_gpio_irq_cb_t cb);
/* Enabling requires a registered callback. Both operations discard pending
 * edges; reception starts at enable. These APIs do not change trigger edges. */
platform_err_t plat_gpio_irq_enable(plat_gpio_id_t id);
platform_err_t plat_gpio_irq_disable(plat_gpio_id_t id);
/* Call only from the matching GPIO ISR. Clears pending before dispatch. */
void plat_gpio_irq_handler(plat_gpio_id_t id);
#ifdef __cplusplus
}
#endif

#endif /* PLAT_GPIO_H */
