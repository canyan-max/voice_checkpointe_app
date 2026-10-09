#ifndef BSP_SD_CARD_H
#define BSP_SD_CARD_H

#include <stdint.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns normalized presence (0 absent, 1 present). Output is unchanged
 * on failure. Generated GPIO initialization must already be complete. */
platform_err_t bsp_sd_card_is_present(uint8_t *p_present);

#ifdef __cplusplus
}
#endif
#endif
