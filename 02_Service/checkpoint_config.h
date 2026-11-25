/**
 ******************************************************************************
 *@file               :   checkpoint_config.h
 *@brief              :   Define the checkpoint configuration data and access.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* define to prevent recursive inclusion ------------------------------------*/
#ifndef CHECKPOINT_CONFIG_H
#define CHECKPOINT_CONFIG_H

/* includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* define -------------------------------------------------------------------*/
#define CHECKPOINT_CONFIG_ITEM_COUNT           (3U)
#define CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES (400U)
#define CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES (200U)
#define CHECKPOINT_CONFIG_VOICE_REPEAT_MAX     (16U)
#define CHECKPOINT_CONFIG_VOICE_REPEAT_DEFAULT (1U)

/* typedef ------------------------------------------------------------------*/
typedef struct CHECKPOINT_ITEM_CONFIG_T {
    uint8_t  voice_text[CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES];
    uint16_t voice_text_size;
    uint8_t  display_text[CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES];
    uint16_t display_text_size;
} checkpoint_item_config_t;

typedef struct CHECKPOINT_CONFIG_T {
    checkpoint_item_config_t items[CHECKPOINT_CONFIG_ITEM_COUNT];
    uint8_t                  voice_repeat;
} checkpoint_config_t;

/* variables ----------------------------------------------------------------*/

/* function -----------------------------------------------------------------*/
/* The caller owns the config and serializes access. Item numbers are 1 to 3. */
/* A zero length clears the item; p_text may be NULL only in that case. */
platform_err_t checkpoint_config_voice_set(checkpoint_config_t *p_config,
                                           uint8_t              item_number,
                                           const uint8_t       *p_text,
                                           uint16_t             text_size);

/* A zero length clears the configured caption. */
platform_err_t checkpoint_config_display_set(checkpoint_config_t *p_config,
                                             uint8_t              item_number,
                                             const uint8_t       *p_text,
                                             uint16_t             text_size);

/* Borrowed view. Consume before modifying the config; NULL for invalid item. */
const checkpoint_item_config_t *
checkpoint_config_item_get(const checkpoint_config_t *p_config,
                           uint8_t                    item_number);

platform_err_t checkpoint_config_voice_repeat_set(checkpoint_config_t *p_config,
                                                  uint8_t repeat_count);

#ifdef __cplusplus
}
#endif

#endif /* CHECKPOINT_CONFIG_H */
/* end of file --------------------------------------------------------------*/
