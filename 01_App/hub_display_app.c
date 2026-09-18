/**
 ******************************************************************************
 *@file               :   hub_display_app.c
 *@brief              :   Own the product HUB display lifecycle.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "hub_display_app.h"
#include "hub_display_service.h"

/* define -------------------------------------------------------------------*/
#define HUB_DISPLAY_APP_ROW_PERIOD_MS       (5U)
#define HUB_DISPLAY_APP_FINISH_STEP_FRAMES  (3U)
#define HUB_DISPLAY_APP_FONT_READ_TIMEOUT_MS (100U)
#define HUB_DISPLAY_APP_TEXT_MAX_BYTES       (512U)

/* variables ----------------------------------------------------------------*/
static StaticTimer_t hub_display_app_timer_control;
static TimerHandle_t hub_display_app_timer;
static StaticSemaphore_t hub_display_app_mutex_control;
static SemaphoreHandle_t hub_display_app_mutex;
static hub_display_service_t hub_display_app_service;
static uint8_t hub_display_app_text[HUB_DISPLAY_APP_TEXT_MAX_BYTES];
static volatile platform_err_t hub_display_app_last_error = PLATFORM_ERR_HW;
static uint8_t hub_display_app_initialized;
static volatile uint8_t hub_display_app_visible;
static volatile uint8_t hub_display_app_finish_pending;
static uint8_t hub_display_app_finish_start_cycle;
static hub_display_app_finish_callback_t hub_display_app_finish_callback;
static void *hub_display_app_finish_context;

/* private functions --------------------------------------------------------*/
static void hub_display_app_finish_notify(void)
{
    hub_display_app_finish_callback_t callback;
    void *p_context;

    callback = hub_display_app_finish_callback;
    p_context = hub_display_app_finish_context;
    hub_display_app_finish_callback = NULL;
    hub_display_app_finish_context = NULL;

    if(NULL != callback)
    {
        callback(p_context);
    }
}

static void hub_display_app_timer_callback(TimerHandle_t timer)
{
    platform_err_t ret;
    uint8_t scroll_cycle_count;
    uint8_t finish_notify;

    (void)timer;
    if(0U == hub_display_app_visible)
    {
        return;
    }
    if(pdPASS != xSemaphoreTake(hub_display_app_mutex, 0U))
    {
        return;
    }

    finish_notify = 0U;
    ret = hub_display_service_refresh(&hub_display_app_service);
    if(PLATFORM_ERR_OK != ret)
    {
        hub_display_app_visible = 0U;
        hub_display_app_finish_pending = 0U;
        hub_display_app_last_error = ret;
        (void)hub_display_service_stop(&hub_display_app_service);
        finish_notify = 1U;
    }
    else if(0U != hub_display_app_finish_pending)
    {
        ret = hub_display_service_scroll_cycle_count_get(
                  &hub_display_app_service,
                  &scroll_cycle_count);
        if((PLATFORM_ERR_OK == ret) &&
           (hub_display_app_finish_start_cycle != scroll_cycle_count))
        {
            hub_display_app_visible = 0U;
            hub_display_app_finish_pending = 0U;
            hub_display_app_last_error = hub_display_service_stop(
                                             &hub_display_app_service);
            finish_notify = 1U;
        }
    }
    (void)xSemaphoreGive(hub_display_app_mutex);

    if(0U != finish_notify)
    {
        (void)xTimerStop(hub_display_app_timer, 0U);
        hub_display_app_finish_notify();
    }
}

/* exported functions -------------------------------------------------------*/
platform_err_t hub_display_app_init(void)
{
    platform_err_t ret;

    if(0U != hub_display_app_initialized)
    {
        return hub_display_app_last_error;
    }

    hub_display_app_mutex = xSemaphoreCreateMutexStatic(
                                &hub_display_app_mutex_control);
    if(NULL == hub_display_app_mutex)
    {
        hub_display_app_last_error = PLATFORM_ERR_HW;
        return PLATFORM_ERR_HW;
    }

    ret = hub_display_service_init(&hub_display_app_service);
    if(PLATFORM_ERR_OK != ret)
    {
        hub_display_app_last_error = ret;
        return ret;
    }

    hub_display_app_timer = xTimerCreateStatic(
        "hub_display",
        pdMS_TO_TICKS(HUB_DISPLAY_APP_ROW_PERIOD_MS),
        pdTRUE,
        NULL,
        hub_display_app_timer_callback,
        &hub_display_app_timer_control);
    if(NULL == hub_display_app_timer)
    {
        (void)hub_display_service_stop(&hub_display_app_service);
        hub_display_app_last_error = PLATFORM_ERR_HW;
        return PLATFORM_ERR_HW;
    }

    hub_display_app_visible = 0U;
    hub_display_app_finish_pending = 0U;
    hub_display_app_finish_start_cycle = 0U;
    hub_display_app_finish_callback = NULL;
    hub_display_app_finish_context = NULL;
    hub_display_app_initialized = 1U;
    hub_display_app_last_error = PLATFORM_ERR_OK;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_app_show_gbk(
    const uint8_t         *p_text,
    uint16_t               text_size,
    hub_display_app_mode_t mode)
{
    platform_err_t ret;
    uint8_t scroll_enabled;

    if((NULL == p_text) || (0U == text_size) ||
       (HUB_DISPLAY_APP_TEXT_MAX_BYTES < text_size) ||
       (0U == hub_display_app_initialized) ||
       (HUB_DISPLAY_APP_MODE_COUNT <= mode))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != hub_display_app_visible)
    {
        return PLATFORM_ERR_BUSY;
    }

    memcpy(hub_display_app_text, p_text, text_size);
    ret = hub_display_service_gbk_text_set(
              &hub_display_app_service,
              hub_display_app_text,
              text_size,
              HUB_DISPLAY_APP_FONT_READ_TIMEOUT_MS);
    if(PLATFORM_ERR_OK != ret)
    {
        hub_display_app_last_error = ret;
        return ret;
    }

    scroll_enabled = (HUB_DISPLAY_APP_MODE_SCROLL == mode) ? 1U : 0U;
    ret = hub_display_service_start(&hub_display_app_service,
                                    scroll_enabled);
    if(PLATFORM_ERR_OK != ret)
    {
        hub_display_app_last_error = ret;
        return ret;
    }

    hub_display_app_visible = 1U;
    hub_display_app_finish_pending = 0U;
    hub_display_app_finish_start_cycle = 0U;
    hub_display_app_finish_callback = NULL;
    hub_display_app_finish_context = NULL;
    if(pdPASS != xTimerStart(hub_display_app_timer, 0U))
    {
        hub_display_app_visible = 0U;
        (void)hub_display_service_stop(&hub_display_app_service);
        hub_display_app_last_error = PLATFORM_ERR_BUSY;
        return PLATFORM_ERR_BUSY;
    }

    hub_display_app_last_error = PLATFORM_ERR_OK;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_app_finish_scroll(
    hub_display_app_finish_callback_t callback,
    void                             *p_context)
{
    if(0U == hub_display_app_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if(0U == hub_display_app_visible)
    {
        if(NULL != callback)
        {
            callback(p_context);
        }
        return PLATFORM_ERR_OK;
    }

    if(pdPASS != xSemaphoreTake(hub_display_app_mutex, portMAX_DELAY))
    {
        return PLATFORM_ERR_HW;
    }
    if(PLATFORM_ERR_OK != hub_display_service_scroll_cycle_count_get(
            &hub_display_app_service,
            &hub_display_app_finish_start_cycle))
    {
        (void)xSemaphoreGive(hub_display_app_mutex);
        return PLATFORM_ERR_HW;
    }
    if(PLATFORM_ERR_OK != hub_display_service_scroll_step_frames_set(
            &hub_display_app_service,
            HUB_DISPLAY_APP_FINISH_STEP_FRAMES))
    {
        (void)xSemaphoreGive(hub_display_app_mutex);
        return PLATFORM_ERR_HW;
    }
    (void)xSemaphoreGive(hub_display_app_mutex);

    hub_display_app_finish_callback = callback;
    hub_display_app_finish_context = p_context;
    hub_display_app_finish_pending = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_app_process(void)
{
    platform_err_t ret;

    if(0U == hub_display_app_initialized)
    {
        return PLATFORM_ERR_HW;
    }
    if(0U == hub_display_app_visible)
    {
        return PLATFORM_ERR_OK;
    }
    if(pdPASS != xSemaphoreTake(hub_display_app_mutex, portMAX_DELAY))
    {
        return PLATFORM_ERR_BUSY;
    }
    ret = hub_display_service_stream_process(
              &hub_display_app_service,
              HUB_DISPLAY_APP_FONT_READ_TIMEOUT_MS);
    (void)xSemaphoreGive(hub_display_app_mutex);

    if(PLATFORM_ERR_OK != ret)
    {
        hub_display_app_last_error = ret;
    }
    return ret;
}

platform_err_t hub_display_app_hide(void)
{
    platform_err_t ret;
    platform_err_t stop_ret;

    if(0U == hub_display_app_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    hub_display_app_visible = 0U;
    hub_display_app_finish_pending = 0U;
    hub_display_app_finish_start_cycle = 0U;
    hub_display_app_finish_callback = NULL;
    hub_display_app_finish_context = NULL;
    ret = (pdPASS == xTimerStop(hub_display_app_timer, 0U)) ?
          PLATFORM_ERR_OK : PLATFORM_ERR_BUSY;
    if(pdPASS == xSemaphoreTake(hub_display_app_mutex, portMAX_DELAY))
    {
        stop_ret = hub_display_service_stop(&hub_display_app_service);
        (void)xSemaphoreGive(hub_display_app_mutex);
    }
    else
    {
        stop_ret = PLATFORM_ERR_BUSY;
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = stop_ret;
    }

    hub_display_app_last_error = ret;
    return ret;
}

platform_err_t hub_display_app_status_get(void)
{
    return (platform_err_t)hub_display_app_last_error;
}

uint8_t hub_display_app_is_visible(void)
{
    return hub_display_app_visible;
}

/* end of file --------------------------------------------------------------*/
