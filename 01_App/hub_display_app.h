/**
 ******************************************************************************
 *@file               :   hub_display_app.h
 *@brief              :   Own the product HUB display lifecycle.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef HUB_DISPLAY_APP_H
#define HUB_DISPLAY_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include "plat_error.h"

/* typedef ------------------------------------------------------------------*/
typedef void (*hub_display_app_finish_callback_t)(void *p_context);

typedef enum HUB_DISPLAY_APP_MODE_T
{
    HUB_DISPLAY_APP_MODE_STATIC = 0U,
    HUB_DISPLAY_APP_MODE_SCROLL,
    HUB_DISPLAY_APP_MODE_COUNT
} hub_display_app_mode_t;

/* functions ----------------------------------------------------------------*/
platform_err_t hub_display_app_init(void);

platform_err_t hub_display_app_show_gbk(
    const uint8_t         *p_text,
    uint16_t               text_size,
    hub_display_app_mode_t mode);

platform_err_t hub_display_app_finish_scroll(
    hub_display_app_finish_callback_t callback,
    void                             *p_context);

platform_err_t hub_display_app_hide(void);

platform_err_t hub_display_app_status_get(void);

#ifdef __cplusplus
}
#endif

#endif /* HUB_DISPLAY_APP_H */
