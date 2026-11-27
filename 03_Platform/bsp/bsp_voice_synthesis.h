/**
 ******************************************************************************
 *@file               :   bsp_voice_synthesis.h
 *@brief              :   Provide the board voice synthesis capability.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BSP_VOICE_SYNTHESIS_H
#define BSP_VOICE_SYNTHESIS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"

/* typedef ------------------------------------------------------------------*/
typedef enum BSP_VOICE_SYNTHESIS_ENCODING_T
{
    BSP_VOICE_SYNTHESIS_ENCODING_GB2312 = 0U,
    BSP_VOICE_SYNTHESIS_ENCODING_GBK,
    BSP_VOICE_SYNTHESIS_ENCODING_UNICODE_LITTLE,
    BSP_VOICE_SYNTHESIS_ENCODING_UNICODE_BIG,
    BSP_VOICE_SYNTHESIS_ENCODING_UTF8
} bsp_voice_synthesis_encoding_t;

typedef enum BSP_VOICE_SYNTHESIS_EVENT_T
{
    BSP_VOICE_SYNTHESIS_EVENT_NONE             = 0U,
    BSP_VOICE_SYNTHESIS_EVENT_INITIALIZED      = (1U << 0),
    BSP_VOICE_SYNTHESIS_EVENT_COMMAND_ACCEPTED = (1U << 1),
    BSP_VOICE_SYNTHESIS_EVENT_COMMAND_REJECTED = (1U << 2),
    BSP_VOICE_SYNTHESIS_EVENT_SPEAKING         = (1U << 3),
    BSP_VOICE_SYNTHESIS_EVENT_IDLE             = (1U << 4),
    BSP_VOICE_SYNTHESIS_EVENT_SLEEPING         = (1U << 5)
} bsp_voice_synthesis_event_t;

typedef enum BSP_VOICE_SYNTHESIS_STATE_T
{
    BSP_VOICE_SYNTHESIS_STATE_UNKNOWN = 0U,
    BSP_VOICE_SYNTHESIS_STATE_IDLE,
    BSP_VOICE_SYNTHESIS_STATE_COMMAND_PENDING,
    BSP_VOICE_SYNTHESIS_STATE_SPEAKING,
    BSP_VOICE_SYNTHESIS_STATE_SLEEPING,
    BSP_VOICE_SYNTHESIS_STATE_ERROR
} bsp_voice_synthesis_state_t;

/* Supplied by the sole calling task; BSP does not own the wait primitive. */
typedef struct BSP_VOICE_SYNTHESIS_TX_SYNC_T {
    /* Consume a completion signal; 0 ms checks without waiting. */
    uint8_t (*pf_take)(uint32_t timeout_ms);
    /* Publish a completion from UART interrupt context. */
    void (*pf_signal_from_isr)(void);
} bsp_voice_synthesis_tx_sync_t;

/* functions ----------------------------------------------------------------*/
/* The hooks are copied. All voice operations must run on the same task. */
platform_err_t bsp_voice_synthesis_init(const bsp_voice_synthesis_tx_sync_t *p_tx_sync);

platform_err_t bsp_voice_synthesis_speak(
    bsp_voice_synthesis_encoding_t encoding,
    const uint8_t                 *p_text,
    uint16_t                       text_size,
    uint32_t                       timeout_ms);

platform_err_t bsp_voice_synthesis_stop(uint32_t timeout_ms);

/* Reply is read through bsp_voice_synthesis_poll(). */
platform_err_t bsp_voice_synthesis_status_request(uint32_t timeout_ms);

platform_err_t bsp_voice_synthesis_busy_get(uint8_t *p_is_busy);

/**
 * @brief Poll received voice synthesis bytes and report device events.
 * @param p_event Receives ORed BSP_VOICE_SYNTHESIS_EVENT_* flags.
 */
platform_err_t bsp_voice_synthesis_poll(
    bsp_voice_synthesis_event_t *p_event);

platform_err_t bsp_voice_synthesis_state_get(
    bsp_voice_synthesis_state_t *p_state);

#ifdef __cplusplus
}
#endif

#endif /* BSP_VOICE_SYNTHESIS_H */
