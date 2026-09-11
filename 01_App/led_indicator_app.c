/**
 ******************************************************************************
 *@file               :   led_indicator_app.c
 *@brief              :   Implement non-blocking product LED patterns.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "bsp_led.h"
#include "led_indicator_app.h"

/* define -------------------------------------------------------------------*/
#define LED_INDICATOR_TIMER_PERIOD_MS       100U
#define LED_INDICATOR_RUN_TOGGLE_MS         500U
#define LED_INDICATOR_VOICE_RED_MS         5000U
#define LED_INDICATOR_VOICE_SEQUENCE_MS   10000U

/* variables ----------------------------------------------------------------*/
static StaticTimer_t led_indicator_timer_control;
static TimerHandle_t led_indicator_timer;
static uint32_t led_indicator_run_elapsed_ms;
static uint32_t led_indicator_voice_elapsed_ms;
static uint8_t led_indicator_run_is_on;
static uint8_t led_indicator_voice_is_active;
static uint8_t led_indicator_is_started;

/* private functions --------------------------------------------------------*/
static void led_indicator_run_process(void)
{
    led_indicator_run_elapsed_ms += LED_INDICATOR_TIMER_PERIOD_MS;
    if(led_indicator_run_elapsed_ms < LED_INDICATOR_RUN_TOGGLE_MS)
    {
        return;
    }

    led_indicator_run_elapsed_ms = 0U;
    led_indicator_run_is_on = (0U == led_indicator_run_is_on) ? 1U : 0U;
    (void)bsp_led_set(BSP_LED_RUN,
                      (0U != led_indicator_run_is_on) ?
                          BSP_LED_ON : BSP_LED_OFF);
}

static void led_indicator_voice_process(void)
{
    uint32_t elapsed_ms;

    taskENTER_CRITICAL();
    if(0U == led_indicator_voice_is_active)
    {
        taskEXIT_CRITICAL();
        return;
    }
    led_indicator_voice_elapsed_ms += LED_INDICATOR_TIMER_PERIOD_MS;
    elapsed_ms = led_indicator_voice_elapsed_ms;
    if(elapsed_ms >= LED_INDICATOR_VOICE_SEQUENCE_MS)
    {
        led_indicator_voice_is_active = 0U;
    }
    taskEXIT_CRITICAL();

    if(elapsed_ms == LED_INDICATOR_VOICE_RED_MS)
    {
        (void)bsp_led_set(BSP_LED_OUTPUT_RED, BSP_LED_OFF);
        (void)bsp_led_set(BSP_LED_OUTPUT_BLUE, BSP_LED_ON);
    }
    else if(elapsed_ms >= LED_INDICATOR_VOICE_SEQUENCE_MS)
    {
        (void)bsp_led_set(BSP_LED_OUTPUT_RED, BSP_LED_OFF);
        (void)bsp_led_set(BSP_LED_OUTPUT_BLUE, BSP_LED_OFF);
    }
}

static void led_indicator_timer_callback(TimerHandle_t timer)
{
    (void)timer;
    led_indicator_run_process();
    led_indicator_voice_process();
}

/* exported functions -------------------------------------------------------*/
platform_err_t led_indicator_app_init(void)
{
    platform_err_t ret;

    if(0U != led_indicator_is_started)
    {
        return PLATFORM_ERR_OK;
    }

    ret = bsp_led_init();
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }
    if(NULL == led_indicator_timer)
    {
        led_indicator_timer = xTimerCreateStatic(
            "led_indicator",
            pdMS_TO_TICKS(LED_INDICATOR_TIMER_PERIOD_MS),
            pdTRUE,
            NULL,
            led_indicator_timer_callback,
            &led_indicator_timer_control);
    }
    if(NULL == led_indicator_timer)
    {
        return PLATFORM_ERR_HW;
    }
    if(pdPASS != xTimerStart(led_indicator_timer, 0U))
    {
        return PLATFORM_ERR_BUSY;
    }
    led_indicator_is_started = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t led_indicator_app_voice_synthesis_started(void)
{
    platform_err_t ret;

    if(0U == led_indicator_is_started)
    {
        return PLATFORM_ERR_HW;
    }

    taskENTER_CRITICAL();
    led_indicator_voice_elapsed_ms = 0U;
    led_indicator_voice_is_active = 1U;
    taskEXIT_CRITICAL();

    ret = bsp_led_set(BSP_LED_OUTPUT_BLUE, BSP_LED_OFF);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_led_set(BSP_LED_OUTPUT_RED, BSP_LED_ON);
    }
    if(PLATFORM_ERR_OK != ret)
    {
        taskENTER_CRITICAL();
        led_indicator_voice_is_active = 0U;
        taskEXIT_CRITICAL();
        (void)bsp_led_set(BSP_LED_OUTPUT_RED, BSP_LED_OFF);
        (void)bsp_led_set(BSP_LED_OUTPUT_BLUE, BSP_LED_OFF);
    }
    return ret;
}

/* end of file --------------------------------------------------------------*/
