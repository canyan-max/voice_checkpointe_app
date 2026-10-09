#ifndef SD_CARD_PRESENCE_SERVICE_H
#define SD_CARD_PRESENCE_SERVICE_H

#include <stdint.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SD_CARD_PRESENCE_EVENT_NONE = 0,
    SD_CARD_PRESENCE_EVENT_INITIAL,
    SD_CARD_PRESENCE_EVENT_CHANGED
} sd_card_presence_event_t;

/* Caller owns this context; all calls must use one serialized owner. */
typedef struct
{
    uint32_t candidate_since;
    uint32_t last_sample;
    uint8_t candidate;
    uint8_t confirmed;
    uint8_t have_candidate;
    uint8_t have_confirmed;
    uint8_t initialized;
} sd_card_presence_service_t;

platform_err_t sd_card_presence_service_init(sd_card_presence_service_t *p_service);
/* Supply a monotonic millisecond timestamp (uint32_t wrap is supported).
 * Confirm after 50 ms; gaps over 100 ms or read errors restart debounce.
 * No event is emitted on failure. Only use p_present when an event is emitted. */
platform_err_t sd_card_presence_service_poll(sd_card_presence_service_t *p_service,
                                            uint32_t now_ms,
                                            sd_card_presence_event_t *p_event,
                                            uint8_t *p_present);
/* Returns BUSY until initial debounce completes. Returns last confirmed
 * state, not current raw hardware state; errors do not erase confirmation. */
platform_err_t sd_card_presence_service_get(const sd_card_presence_service_t *p_service,
                                           uint8_t *p_present);

#ifdef __cplusplus
}
#endif
#endif
