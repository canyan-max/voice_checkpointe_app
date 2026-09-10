/**
 ******************************************************************************
 *@file               :   button.h
 *@brief              :   Pure software button debounce and gesture detector.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* typedef ------------------------------------------------------------------*/
typedef enum BUTTON_STATUS_T
{
    BUTTON_STATUS_OK = 0U,
    BUTTON_STATUS_PARAM,
    BUTTON_STATUS_NOT_INITIALIZED,
} button_status_t;

typedef enum BUTTON_EVENT_T
{
    BUTTON_EVENT_NONE = 0U,
    BUTTON_EVENT_PRESS,
    BUTTON_EVENT_RELEASE,
    BUTTON_EVENT_CLICK,
    BUTTON_EVENT_DOUBLE_CLICK,
    BUTTON_EVENT_LONG_PRESS,
} button_event_t;

typedef struct BUTTON_TIMING_T
{
    uint32_t debounce_ms;
    uint32_t long_press_ms;
    uint32_t double_click_ms;
} button_timing_t;

typedef struct BUTTON_T
{
    button_timing_t timing;
    uint32_t        edge_time_ms;
    uint32_t        press_time_ms;
    uint32_t        release_time_ms;
    uint8_t         raw_pressed;
    uint8_t         stable_pressed;
    uint8_t         state;
    uint8_t         initialized;
} button_t;

/* functions ----------------------------------------------------------------*/
/**
 * @brief Initialize one button state machine from the current sampled state.
 * @param p_button Instance storage owned by the caller.
 * @param p_timing Immutable timing values copied into the instance.
 * @param initial_pressed Initial normalized state: 1 pressed, 0 released.
 * @param now_ms Current monotonic millisecond timestamp.
 * @retval BUTTON_STATUS_OK on success; BUTTON_STATUS_PARAM otherwise.
 */
button_status_t button_init(button_t              *p_button,
                            const button_timing_t *p_timing,
                            uint8_t                initial_pressed,
                            uint32_t               now_ms);

/**
 * @brief Feed one normalized sample into the non-blocking button detector.
 * @param p_button Initialized instance.
 * @param pressed Current normalized state: 1 pressed, 0 released.
 * @param now_ms Current monotonic millisecond timestamp.
 * @param p_event Receives at most one event; NONE means no event this update.
 * @retval BUTTON_STATUS_OK on success; otherwise a Button status.
 * @note Unsigned timestamp subtraction supports one uint32_t wraparound as
 *       long as elapsed intervals remain shorter than the counter period.
 */
button_status_t button_update(button_t       *p_button,
                              uint8_t         pressed,
                              uint32_t        now_ms,
                              button_event_t *p_event);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
