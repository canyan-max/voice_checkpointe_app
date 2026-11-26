/**
 **************************************************************************************************
 *@file               :   checkpoint_config_storage.c
 *@brief              :   Load and save checkpoint configuration through LittleFS.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* includes -------------------------------------------------------------------------------------*/
#include <string.h>
#include "checkpoint_config_storage.h"

/* define   -------------------------------------------------------------------------------------*/
/* V1: magic/version, five scalar settings, six LE16 lengths with their data,
 * then LE32 CRC over all preceding bytes. Only live text bytes are serialized. */
#define CONFIG_PATH          "/checkpoint.cfg"
#define CONFIG_TEMP_PATH     "/checkpoint.tmp"
#define CONFIG_VERSION       (1U)
#define CONFIG_MAGIC_BYTES   (4U)
#define CONFIG_HEADER_BYTES  (10U)
#define CONFIG_CRC_BYTES     (4U)
#define CONFIG_LENGTH_BYTES  (2U)
#define CONFIG_MIN_BYTES     (CONFIG_HEADER_BYTES + CONFIG_CRC_BYTES + \
                              CHECKPOINT_CONFIG_ITEM_COUNT * 2U * CONFIG_LENGTH_BYTES)
#define CONFIG_MAX_BYTES     (CONFIG_MIN_BYTES + CHECKPOINT_CONFIG_ITEM_COUNT * \
                              (CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES + \
                               CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES))

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/
/* Startup load and runtime saves are serialized; no large RTOS stack buffer. */
static uint8_t       s_config_file[CONFIG_MAX_BYTES];
static const uint8_t s_config_magic[CONFIG_MAGIC_BYTES] = {'C', 'P', 'C', 'F'};

/* private  functions  --------------------------------------------------------------------------*/
static uint32_t config_crc(const uint8_t *p_data, uint32_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0U; i < size; i++) {
        crc ^= p_data[i];
        for (uint32_t bit = 0U; bit < 8U; bit++) {
            crc = (crc >> 1U) ^ ((0U != (crc & 1U)) ? 0xEDB88320U : 0U);
        }
    }
    return ~crc;
}

static uint8_t config_settings_valid(uint8_t voice_repeat,
                                     uint8_t display_repeat,
                                     uint8_t pir_enabled,
                                     uint8_t pir_item_number) {
    return (voice_repeat <= CHECKPOINT_CONFIG_VOICE_REPEAT_MAX) &&
           (display_repeat <= CHECKPOINT_CONFIG_DISPLAY_REPEAT_MAX) && (pir_enabled <= 1U) &&
           (pir_item_number >= 1U) && (pir_item_number <= CHECKPOINT_CONFIG_ITEM_COUNT);
}

static uint32_t config_encode(const checkpoint_config_t *p_config) {
    if (0U == config_settings_valid(p_config->voice_repeat, p_config->display_repeat,
                                    p_config->pir_enabled, p_config->pir_item_number)) {
        return 0U;
    }
    memcpy(s_config_file, s_config_magic, CONFIG_MAGIC_BYTES);
    s_config_file[4U] = CONFIG_VERSION;
    s_config_file[5U] = p_config->device_id;
    s_config_file[6U] = p_config->voice_repeat;
    s_config_file[7U] = p_config->display_repeat;
    s_config_file[8U] = p_config->pir_enabled;
    s_config_file[9U] = p_config->pir_item_number;

    uint32_t offset = CONFIG_HEADER_BYTES;
    for (uint32_t i = 0U; i < CHECKPOINT_CONFIG_ITEM_COUNT; i++) {
        const checkpoint_item_config_t *p_item = &p_config->items[i];
        if ((p_item->voice_text_size > CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES) ||
            (p_item->display_text_size > CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES)) {
            return 0U;
        }
        const uint8_t *p_text[]  = {p_item->voice_text, p_item->display_text};
        const uint16_t lengths[] = {p_item->voice_text_size, p_item->display_text_size};
        for (uint32_t j = 0U; j < sizeof(lengths) / sizeof(lengths[0U]); j++) {
            const uint16_t length   = lengths[j];
            s_config_file[offset++] = (uint8_t)length;
            s_config_file[offset++] = (uint8_t)(length >> 8U);
            memcpy(&s_config_file[offset], p_text[j], length);
            offset += length;
        }
    }
    const uint32_t crc = config_crc(s_config_file, offset);
    for (uint32_t i = 0U; i < CONFIG_CRC_BYTES; i++) {
        s_config_file[offset++] = (uint8_t)(crc >> (8U * i));
    }
    return offset;
}

static uint8_t config_decode(checkpoint_config_t *p_config, uint32_t size) {
    if ((size < CONFIG_MIN_BYTES) || (size > sizeof(s_config_file)) ||
        (0 != memcmp(s_config_file, s_config_magic, CONFIG_MAGIC_BYTES)) ||
        (CONFIG_VERSION != s_config_file[4U]) ||
        (0U == config_settings_valid(s_config_file[6U], s_config_file[7U], s_config_file[8U],
                                     s_config_file[9U]))) {
        return 0U;
    }
    const uint32_t end          = size - CONFIG_CRC_BYTES;
    uint32_t       expected_crc = 0U;
    for (uint32_t i = 0U; i < CONFIG_CRC_BYTES; i++) {
        expected_crc |= (uint32_t)s_config_file[end + i] << (8U * i);
    }
    if (expected_crc != config_crc(s_config_file, end)) {
        return 0U;
    }

    uint32_t       offset                                    = CONFIG_HEADER_BYTES;
    uint16_t       lengths[CHECKPOINT_CONFIG_ITEM_COUNT][2U] = {0};
    uint32_t       offsets[CHECKPOINT_CONFIG_ITEM_COUNT][2U] = {0};
    const uint16_t limits[] = {CHECKPOINT_CONFIG_VOICE_TEXT_MAX_BYTES,
                               CHECKPOINT_CONFIG_DISPLAY_TEXT_MAX_BYTES};
    for (uint32_t i = 0U; i < CHECKPOINT_CONFIG_ITEM_COUNT; i++) {
        for (uint32_t j = 0U; j < sizeof(limits) / sizeof(limits[0U]); j++) {
            if (CONFIG_LENGTH_BYTES > (end - offset)) {
                return 0U;
            }
            const uint16_t length = (uint16_t)((uint16_t)s_config_file[offset] |
                                               ((uint16_t)s_config_file[offset + 1U] << 8U));
            offset += CONFIG_LENGTH_BYTES;
            if ((length > limits[j]) || (length > (end - offset))) {
                return 0U;
            }
            lengths[i][j] = length;
            offsets[i][j] = offset;
            offset += length;
        }
    }
    if (end != offset) {
        return 0U;
    }

    /* Only apply after every field has passed validation. */
    p_config->device_id       = s_config_file[5U];
    p_config->voice_repeat    = s_config_file[6U];
    p_config->display_repeat  = s_config_file[7U];
    p_config->pir_enabled     = s_config_file[8U];
    p_config->pir_item_number = s_config_file[9U];
    for (uint32_t i = 0U; i < CHECKPOINT_CONFIG_ITEM_COUNT; i++) {
        checkpoint_item_config_t *p_item = &p_config->items[i];
        p_item->voice_text_size          = lengths[i][0U];
        p_item->display_text_size        = lengths[i][1U];
        memcpy(p_item->voice_text, &s_config_file[offsets[i][0U]], p_item->voice_text_size);
        memcpy(p_item->display_text, &s_config_file[offsets[i][1U]], p_item->display_text_size);
    }
    return 1U;
}

/* exported functions ---------------------------------------------------------------------------*/
storage_filesystem_service_ret_t checkpoint_config_storage_load(checkpoint_config_t *p_config) {
    if (NULL == p_config) {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    storage_filesystem_service_ret_t
        ret = storage_filesystem_service_file_open(CONFIG_PATH,
                                                   STORAGE_FILESYSTEM_SERVICE_OPEN_READ);
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != ret) {
        return ret;
    }
    uint32_t size = 0U;
    ret           = storage_filesystem_service_file_size_get(&size);
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK == ret) {
        if ((size < CONFIG_MIN_BYTES) || (size > sizeof(s_config_file))) {
            ret = STORAGE_FILESYSTEM_SERVICE_RET_CORRUPT;
        } else {
            uint32_t read_size = 0U;
            ret = storage_filesystem_service_file_read(s_config_file, size, &read_size);
            if ((STORAGE_FILESYSTEM_SERVICE_RET_OK == ret) && (size != read_size)) {
                ret = STORAGE_FILESYSTEM_SERVICE_RET_CORRUPT;
            }
        }
    }
    const storage_filesystem_service_ret_t close_ret = storage_filesystem_service_file_close();
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != ret) {
        return ret;
    }
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != close_ret) {
        return close_ret;
    }
    return (0U != config_decode(p_config, size)) ? STORAGE_FILESYSTEM_SERVICE_RET_OK
                                                 : STORAGE_FILESYSTEM_SERVICE_RET_CORRUPT;
}

storage_filesystem_service_ret_t
checkpoint_config_storage_save(const checkpoint_config_t *p_config) {
    if (NULL == p_config) {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    const uint32_t size = config_encode(p_config);
    if (0U == size) {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    storage_filesystem_service_ret_t
        ret = storage_filesystem_service_file_open(CONFIG_TEMP_PATH,
                                                   STORAGE_FILESYSTEM_SERVICE_OPEN_WRITE_TRUNCATE);
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != ret) {
        return ret;
    }
    uint32_t written = 0U;
    ret              = storage_filesystem_service_file_write(s_config_file, size, &written);
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK == ret) {
        ret = (size == written) ? storage_filesystem_service_file_sync()
                                : STORAGE_FILESYSTEM_SERVICE_RET_HW;
    }
    const storage_filesystem_service_ret_t close_ret = storage_filesystem_service_file_close();
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != ret) {
        return ret;
    }
    if (STORAGE_FILESYSTEM_SERVICE_RET_OK != close_ret) {
        return close_ret;
    }
    return storage_filesystem_service_file_replace(CONFIG_TEMP_PATH, CONFIG_PATH);
}

/* end of file ----------------------------------------------------------------------------------*/
