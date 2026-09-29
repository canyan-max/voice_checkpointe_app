/**
 ******************************************************************************
 *@file               :   led_indicator_app.h
 *@brief              :   Drive product LED indication patterns.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef LED_INDICATOR_APP_H
#define LED_INDICATOR_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"

/* functions ----------------------------------------------------------------*/
platform_err_t led_indicator_app_init(void);

platform_err_t led_indicator_app_voice_start(void);

platform_err_t led_indicator_app_voice_stop(void);
platform_err_t led_indicator_app_remote_connected_set(uint8_t connected);

#ifdef __cplusplus
}
#endif

#endif /* LED_INDICATOR_APP_H */
