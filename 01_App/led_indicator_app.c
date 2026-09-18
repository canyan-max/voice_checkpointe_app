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
#define LED_INDICATOR_TIMER_PERIOD_MS       (100U)
#define LED_INDICATOR_RUN_TOGGLE_MS         (500U)
#define LED_INDICATOR_VOICE_TOGGLE_MS      (2000U)

/* variables ----------------------------------------------------------------*/
static StaticTimer_t led_indicator_timer_control;
static TimerHandle_t led_indicator_timer;
static uint32_t led_indicator_run_elapsed_ms;
static uint32_t led_indicator_voice_elapsed_ms;
static uint8_t led_indicator_run_is_on;
static uint8_t led_indicator_voice_red_is_on;
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
    uint8_t indicator_toggle;
    uint8_t red_is_on;

    taskENTER_CRITICAL();
    if(0U == led_indicator_voice_is_active)
    {
        taskEXIT_CRITICAL();
        return;
    }

    indicator_toggle = 0U;
    led_indicator_voice_elapsed_ms += LED_INDICATOR_TIMER_PERIOD_MS;
    if(led_indicator_voice_elapsed_ms >= LED_INDICATOR_VOICE_TOGGLE_MS)
    {
        led_indicator_voice_elapsed_ms = 0U;
        led_indicator_voice_red_is_on =
            (0U == led_indicator_voice_red_is_on) ? 1U : 0U;
        indicator_toggle = 1U;
    }
    red_is_on = led_indicator_voice_red_is_on;
    taskEXIT_CRITICAL();

    if(0U != indicator_toggle)
    {
        (void)bsp_led_set(BSP_LED_OUTPUT_RED,
                          (0U != red_is_on) ? BSP_LED_ON : BSP_LED_OFF);
        (void)bsp_led_set(BSP_LED_OUTPUT_BLUE,
                          (0U != red_is_on) ? BSP_LED_OFF : BSP_LED_ON);
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

platform_err_t led_indicator_app_voice_start(void)
{
    platform_err_t ret;

    if(0U == led_indicator_is_started)
    {
        return PLATFORM_ERR_HW;
    }

    taskENTER_CRITICAL();
    led_indicator_voice_elapsed_ms = 0U;
    led_indicator_voice_red_is_on = 1U;
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

platform_err_t led_indicator_app_voice_stop(void)
{
    platform_err_t ret;
    platform_err_t led_ret;

    if(0U == led_indicator_is_started)
    {
        return PLATFORM_ERR_HW;
    }

    taskENTER_CRITICAL();
    led_indicator_voice_is_active = 0U;
    taskEXIT_CRITICAL();

    ret = bsp_led_set(BSP_LED_OUTPUT_RED, BSP_LED_OFF);
    led_ret = bsp_led_set(BSP_LED_OUTPUT_BLUE, BSP_LED_OFF);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = led_ret;
    }
    return ret;
}

/* end of file --------------------------------------------------------------*/
