#ifndef SD_CARD_PRESENCE_APP_H
#define SD_CARD_PRESENCE_APP_H

#include <stdint.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One task owns init/poll; no task or queue is created by this module.
 * Initialize before audio startup; do not reinitialize during active sessions. */
platform_err_t sd_card_presence_app_init(void);
void sd_card_presence_app_poll(void);
/* Task-context readers may run concurrently with poll. Returns a snapshot
 * of last confirmed presence, or BUSY before initial confirmation.
 * Read errors preserve the previous confirmed state. Not an ISR API. */
platform_err_t sd_card_presence_app_get(uint8_t *p_present);
/* Coherent presence and removal generation. Every confirmed removal advances
 * generation, so reinsertion cannot make an old file/session valid again. */
platform_err_t sd_card_presence_app_snapshot_get(uint8_t *p_present,
                                                uint32_t *p_generation);

#ifdef __cplusplus
}
#endif
#endif
