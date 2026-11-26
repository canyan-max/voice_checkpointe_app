/**
 **************************************************************************************************
 *@file               :   checkpoint_config.c
 *@brief              :   Validate and update checkpoint configuration data.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* includes -------------------------------------------------------------------------------------*/
#include <string.h>
#include "checkpoint_config.h"
/* define   -------------------------------------------------------------------------------------*/

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

/* private  functions  --------------------------------------------------------------------------*/
static uint8_t checkpoint_config_item_valid(uint8_t item_number) {
    return (0U != item_number) && (item_number <= CHECKPOINT_CONFIG_ITEM_COUNT);
}

/* exported functions ---------------------------------------------------------------------------*/
platform_err_t checkpoint_config_voice_set(checkpoint_config_t *p_config,
                                           uint8_t              item_number,
                                           const uint8_t       *p_text,
                                           uint16_t             text_size) {
    if ((NULL == p_config) || (0U == checkpoint_config_item_valid(item_number)) ||
        (text_size > CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES) ||
        ((0U != text_size) && (NULL == p_text))) {
        return PLATFORM_ERR_PARAM;
    }

    checkpoint_item_config_t *p_item = &p_config->items[item_number - 1U];
    if (0U != text_size) {
        memmove(p_item->voice_text, p_text, text_size);
    }
    p_item->voice_text_size = text_size;
    return PLATFORM_ERR_OK;
}

platform_err_t checkpoint_config_display_set(checkpoint_config_t *p_config,
                                             uint8_t              item_number,
                                             const uint8_t       *p_text,
                                             uint16_t             text_size) {
    if ((NULL == p_config) || (0U == checkpoint_config_item_valid(item_number)) ||
        (text_size > CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES) ||
        ((0U != text_size) && (NULL == p_text))) {
        return PLATFORM_ERR_PARAM;
    }

    checkpoint_item_config_t *p_item = &p_config->items[item_number - 1U];
    if (0U != text_size) {
        memmove(p_item->display_text, p_text, text_size);
    }
    p_item->display_text_size = text_size;
    return PLATFORM_ERR_OK;
}

const checkpoint_item_config_t *checkpoint_config_item_get(const checkpoint_config_t *p_config,
                                                           uint8_t                    item_number) {
    if ((NULL == p_config) || (0U == checkpoint_config_item_valid(item_number))) {
        return NULL;
    }
    return &p_config->items[item_number - 1U];
}

platform_err_t checkpoint_config_voice_repeat_set(checkpoint_config_t *p_config,
                                                  uint8_t              repeat_count) {
    if ((NULL == p_config) || (repeat_count > CHECKPOINT_CONFIG_VOICE_REPEAT_MAX)) {
        return PLATFORM_ERR_PARAM;
    }
    p_config->voice_repeat = repeat_count;
    return PLATFORM_ERR_OK;
}

platform_err_t checkpoint_config_display_repeat_set(checkpoint_config_t *p_config,
                                                    uint8_t              repeat_count) {
    if ((NULL == p_config) || (repeat_count > CHECKPOINT_CONFIG_DISPLAY_REPEAT_MAX)) {
        return PLATFORM_ERR_PARAM;
    }
    p_config->display_repeat = repeat_count;
    return PLATFORM_ERR_OK;
}

platform_err_t checkpoint_config_pir_enable_set(checkpoint_config_t *p_config, uint8_t enabled) {
    if ((NULL == p_config) || (enabled > 1U)) {
        return PLATFORM_ERR_PARAM;
    }
    p_config->pir_enabled = enabled;
    return PLATFORM_ERR_OK;
}

platform_err_t checkpoint_config_pir_item_set(checkpoint_config_t *p_config, uint8_t item_number) {
    if ((NULL == p_config) || (0U == checkpoint_config_item_valid(item_number))) {
        return PLATFORM_ERR_PARAM;
    }
    p_config->pir_item_number = item_number;
    return PLATFORM_ERR_OK;
}

/* end of file ----------------------------------------------------------------------------------*/
