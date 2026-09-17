/**
 ******************************************************************************
 *@file               :   voice_presentation_app.c
 *@brief              :   Coordinate voice captions and LED indication.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hub_display_app.h"
#include "led_indicator_app.h"
#include "voice_presentation_app.h"

/* typedef ------------------------------------------------------------------*/
typedef enum VOICE_PRESENTATION_STATE_T
{
    VOICE_PRESENTATION_STATE_IDLE = 0U,
    VOICE_PRESENTATION_STATE_ACTIVE,
    VOICE_PRESENTATION_STATE_FINISH_PENDING
} voice_presentation_state_t;

/* variables ----------------------------------------------------------------*/
static voice_presentation_state_t voice_presentation_state =
    VOICE_PRESENTATION_STATE_IDLE;
static uint8_t voice_presentation_initialized;

/* private functions --------------------------------------------------------*/
static voice_presentation_state_t voice_presentation_state_get(void)
{
    voice_presentation_state_t state;

    taskENTER_CRITICAL();
    state = voice_presentation_state;
    taskEXIT_CRITICAL();
    return state;
}

static void voice_presentation_scroll_finished(void *p_context)
{
    uint8_t stop_led;

    (void)p_context;
    stop_led = 0U;
    taskENTER_CRITICAL();
    if(VOICE_PRESENTATION_STATE_FINISH_PENDING == voice_presentation_state)
    {
        voice_presentation_state = VOICE_PRESENTATION_STATE_IDLE;
        stop_led = 1U;
    }
    taskEXIT_CRITICAL();

    if(0U != stop_led)
    {
        (void)led_indicator_app_voice_stop();
    }
}

/* exported functions -------------------------------------------------------*/
platform_err_t voice_presentation_app_init(void)
{
    platform_err_t ret;

    if(0U != voice_presentation_initialized)
    {
        return PLATFORM_ERR_OK;
    }

    ret = led_indicator_app_init();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    ret = hub_display_app_init();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    voice_presentation_state = VOICE_PRESENTATION_STATE_IDLE;
    voice_presentation_initialized = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t voice_presentation_app_start(void)
{
    platform_err_t ret;

    if(0U == voice_presentation_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    if(VOICE_PRESENTATION_STATE_IDLE != voice_presentation_state_get())
    {
        ret = voice_presentation_app_abort();
        if(PLATFORM_ERR_OK != ret)
        {
            return ret;
        }
    }

    ret = hub_display_app_show(HUB_DISPLAY_APP_MODE_SCROLL);
    if(PLATFORM_ERR_BUSY == ret)
    {
        ret = hub_display_app_hide();
        if(PLATFORM_ERR_OK == ret)
        {
            ret = hub_display_app_show(HUB_DISPLAY_APP_MODE_SCROLL);
        }
    }
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    ret = led_indicator_app_voice_start();
    if(PLATFORM_ERR_OK != ret)
    {
        (void)hub_display_app_hide();
        return ret;
    }

    taskENTER_CRITICAL();
    voice_presentation_state = VOICE_PRESENTATION_STATE_ACTIVE;
    taskEXIT_CRITICAL();
    return PLATFORM_ERR_OK;
}

platform_err_t voice_presentation_app_audio_finished(void)
{
    platform_err_t ret;

    if(0U == voice_presentation_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    taskENTER_CRITICAL();
    if(VOICE_PRESENTATION_STATE_ACTIVE != voice_presentation_state)
    {
        taskEXIT_CRITICAL();
        return PLATFORM_ERR_OK;
    }
    voice_presentation_state = VOICE_PRESENTATION_STATE_FINISH_PENDING;
    taskEXIT_CRITICAL();

    ret = hub_display_app_finish_scroll(
              voice_presentation_scroll_finished,
              NULL);
    if(PLATFORM_ERR_OK != ret)
    {
        taskENTER_CRITICAL();
        voice_presentation_state = VOICE_PRESENTATION_STATE_IDLE;
        taskEXIT_CRITICAL();
        (void)hub_display_app_hide();
        (void)led_indicator_app_voice_stop();
    }
    return ret;
}

platform_err_t voice_presentation_app_abort(void)
{
    platform_err_t ret;
    platform_err_t led_ret;

    if(0U == voice_presentation_initialized)
    {
        return PLATFORM_ERR_HW;
    }

    taskENTER_CRITICAL();
    voice_presentation_state = VOICE_PRESENTATION_STATE_IDLE;
    taskEXIT_CRITICAL();

    ret = hub_display_app_hide();
    led_ret = led_indicator_app_voice_stop();
    if(PLATFORM_ERR_OK == ret)
    {
        ret = led_ret;
    }
    return ret;
}

/* end of file --------------------------------------------------------------*/
