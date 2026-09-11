/**
 ******************************************************************************
 *@file               :   bsp_led.h
 *@brief              :   Provide board LED control with normalized polarity.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include "plat_error.h"

/* typedef ------------------------------------------------------------------*/
typedef enum BSP_LED_ID_T
{
    BSP_LED_REMOTE = 0U,
    BSP_LED_UPDATE,
    BSP_LED_BROADCAST,
    BSP_LED_RUN,
    BSP_LED_OUTPUT_BLUE,
    BSP_LED_OUTPUT_RED,
    BSP_LED_COUNT
} bsp_led_id_t;

typedef enum BSP_LED_STATE_T
{
    BSP_LED_OFF = 0U,
    BSP_LED_ON
} bsp_led_state_t;

/* functions ----------------------------------------------------------------*/
platform_err_t bsp_led_init(void);

platform_err_t bsp_led_set(bsp_led_id_t led_id, bsp_led_state_t state);

platform_err_t bsp_led_all_off(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */

