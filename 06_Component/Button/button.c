/**
 ******************************************************************************
 *@file               :   button.c
 *@brief              :   Pure software button debounce and gesture detector.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "button.h"

/* define -------------------------------------------------------------------*/
#define BUTTON_NOT_INITIALIZED  (0U)
#define BUTTON_INITIALIZED      (1U)
#define BUTTON_RELEASED         (0U)
#define BUTTON_PRESSED          (1U)

/* typedef ------------------------------------------------------------------*/
typedef enum BUTTON_STATE_T
{
    BUTTON_STATE_IDLE = 0U,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_LONG_FIRED,
    BUTTON_STATE_WAIT_SECOND,
    BUTTON_STATE_SECOND_PRESSED,
} button_state_t;

/* Private functions --------------------------------------------------------*/
static uint8_t button_timing_is_valid(const button_timing_t *p_timing)
{
    return (uint8_t)((NULL != p_timing) && (0U != p_timing->debounce_ms) &&
                     (0U != p_timing->long_press_ms) &&
                     (0U != p_timing->double_click_ms) &&
                     (UINT32_MAX != p_timing->debounce_ms) &&
                     (UINT32_MAX != p_timing->long_press_ms) &&
                     (UINT32_MAX != p_timing->double_click_ms));
}

static uint32_t button_elapsed_ms(uint32_t from_ms, uint32_t now_ms)
{
    return now_ms - from_ms;
}

static button_event_t button_on_press(button_t *p_button, uint32_t now_ms)
{
    switch((button_state_t)p_button->state)
    {
        case BUTTON_STATE_IDLE:
            p_button->state         = (uint8_t)BUTTON_STATE_PRESSED;
            p_button->press_time_ms = now_ms;
            return BUTTON_EVENT_PRESS;

        case BUTTON_STATE_WAIT_SECOND:
            p_button->press_time_ms = now_ms;
            if(button_elapsed_ms(p_button->release_time_ms, now_ms) <=
               p_button->timing.double_click_ms)
            {
                p_button->state = (uint8_t)BUTTON_STATE_SECOND_PRESSED;
                return BUTTON_EVENT_PRESS;
            }

            p_button->state = (uint8_t)BUTTON_STATE_PRESSED;
            return BUTTON_EVENT_CLICK;

        case BUTTON_STATE_PRESSED:
        case BUTTON_STATE_LONG_FIRED:
        case BUTTON_STATE_SECOND_PRESSED:
        default:
            return BUTTON_EVENT_NONE;
    }
}

static button_event_t button_on_release(button_t *p_button, uint32_t now_ms)
{
    switch((button_state_t)p_button->state)
    {
        case BUTTON_STATE_PRESSED:
            p_button->state           = (uint8_t)BUTTON_STATE_WAIT_SECOND;
            p_button->release_time_ms = now_ms;
            return BUTTON_EVENT_RELEASE;

        case BUTTON_STATE_SECOND_PRESSED:
            p_button->state = (uint8_t)BUTTON_STATE_IDLE;
            return BUTTON_EVENT_DOUBLE_CLICK;

        case BUTTON_STATE_LONG_FIRED:
            p_button->state = (uint8_t)BUTTON_STATE_IDLE;
            return BUTTON_EVENT_RELEASE;

        case BUTTON_STATE_IDLE:
        case BUTTON_STATE_WAIT_SECOND:
        default:
            return BUTTON_EVENT_NONE;
    }
}

static button_event_t button_accept_stable_change(button_t *p_button,
                                                   uint32_t  now_ms)
{
    if(BUTTON_PRESSED == p_button->stable_pressed)
    {
        return button_on_press(p_button, now_ms);
    }
    return button_on_release(p_button, now_ms);
}

static button_event_t button_check_timed_event(button_t *p_button,
                                                uint32_t  now_ms)
{
    switch((button_state_t)p_button->state)
    {
        case BUTTON_STATE_PRESSED:
        case BUTTON_STATE_SECOND_PRESSED:
            if(button_elapsed_ms(p_button->press_time_ms, now_ms) >=
               p_button->timing.long_press_ms)
            {
                p_button->state = (uint8_t)BUTTON_STATE_LONG_FIRED;
                return BUTTON_EVENT_LONG_PRESS;
            }
            break;

        case BUTTON_STATE_WAIT_SECOND:
            if(button_elapsed_ms(p_button->release_time_ms, now_ms) >=
               p_button->timing.double_click_ms)
            {
                p_button->state = (uint8_t)BUTTON_STATE_IDLE;
                return BUTTON_EVENT_CLICK;
            }
            break;

        case BUTTON_STATE_IDLE:
        case BUTTON_STATE_LONG_FIRED:
        default:
            break;
    }
    return BUTTON_EVENT_NONE;
}

/* Exported functions -------------------------------------------------------*/
button_status_t button_init(button_t              *p_button,
                            const button_timing_t *p_timing,
                            uint8_t                initial_pressed,
                            uint32_t               now_ms)
{
    if(NULL == p_button)
    {
        return BUTTON_STATUS_PARAM;
    }

    p_button->initialized = BUTTON_NOT_INITIALIZED;
    if((0U == button_timing_is_valid(p_timing)) ||
       (initial_pressed > BUTTON_PRESSED))
    {
        return BUTTON_STATUS_PARAM;
    }

    p_button->timing         = *p_timing;
    p_button->edge_time_ms   = now_ms;
    p_button->press_time_ms  = now_ms;
    p_button->release_time_ms = now_ms;
    p_button->raw_pressed    = initial_pressed;
    p_button->stable_pressed = initial_pressed;
    p_button->state          = (uint8_t)BUTTON_STATE_IDLE;
    p_button->initialized    = BUTTON_INITIALIZED;
    return BUTTON_STATUS_OK;
}

button_status_t button_update(button_t       *p_button,
                              uint8_t         pressed,
                              uint32_t        now_ms,
                              button_event_t *p_event)
{
    if((NULL == p_button) || (NULL == p_event) ||
       (pressed > BUTTON_PRESSED))
    {
        return BUTTON_STATUS_PARAM;
    }
    *p_event = BUTTON_EVENT_NONE;
    if(BUTTON_INITIALIZED != p_button->initialized)
    {
        return BUTTON_STATUS_NOT_INITIALIZED;
    }

    if(pressed != p_button->raw_pressed)
    {
        p_button->raw_pressed = pressed;
        p_button->edge_time_ms = now_ms;
    }

    if((pressed != p_button->stable_pressed) &&
       (button_elapsed_ms(p_button->edge_time_ms, now_ms) >=
        p_button->timing.debounce_ms))
    {
        p_button->stable_pressed = pressed;
        *p_event = button_accept_stable_change(p_button, now_ms);
    }

    if(BUTTON_EVENT_NONE == *p_event)
    {
        *p_event = button_check_timed_event(p_button, now_ms);
    }
    return BUTTON_STATUS_OK;
}

/* end of file --------------------------------------------------------------*/
