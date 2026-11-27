/**
 **************************************************************************************************
 *@file               :   firmware_staging_service.c
 *@brief              :   Manage the reserved external Flash firmware staging area.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* includes -------------------------------------------------------------------------------------*/
#include <string.h>
#include "firmware_staging_service.h"
#include "board_storage_layout.h"
#include "bsp_storage_flash.h"

/* define   -------------------------------------------------------------------------------------*/

#define FIRMWARE_STAGING_BASE (BOARD_STORAGE_OTA_BASE_ADDRESS + BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES)
#define FIRMWARE_STAGING_CAPACITY (BOARD_STORAGE_OTA_SIZE_BYTES - BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES)

_Static_assert(BOARD_STORAGE_OTA_BASE_ADDRESS % BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES == 0U,
               "OTA base must be sector aligned");
_Static_assert(BOARD_STORAGE_OTA_SIZE_BYTES % BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES == 0U &&
                   BOARD_STORAGE_OTA_SIZE_BYTES > BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES,
               "OTA region must contain whole sectors and room for the image");

_Static_assert(BOARD_OTA_APP_SIZE_BYTES <= FIRMWARE_STAGING_CAPACITY,
               "Internal APP partition must fit in QSPI staging");

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

/* private  functions  --------------------------------------------------------------------------*/

static uint8_t firmware_staging_range_valid(uint32_t offset, uint32_t size) {
    return 0U != size && offset < FIRMWARE_STAGING_CAPACITY &&
           size <= FIRMWARE_STAGING_CAPACITY - offset;
}

/* exported functions ---------------------------------------------------------------------------*/

uint32_t firmware_staging_capacity(void) {
    return BOARD_OTA_APP_SIZE_BYTES;
}

uint32_t firmware_staging_sector_size(void) {
    return BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES;
}

platform_err_t firmware_staging_erase_metadata(uint32_t timeout_ms) {
    return bsp_storage_flash_sector_erase(BOARD_STORAGE_OTA_BASE_ADDRESS, timeout_ms);
}

platform_err_t firmware_staging_publish(uint32_t image_size, uint32_t crc32, uint32_t timeout_ms) {
    if (image_size <= 8U || image_size > firmware_staging_capacity()) {
        return PLATFORM_ERR_PARAM;
    }

    const uint32_t record[]  = {image_size, crc32, BOARD_OTA_APP_BASE_ADDRESS, BOARD_OTA_COMMIT_V1};
    const uint32_t body_size = sizeof(record) - sizeof(record[0U]);
    uint32_t       readback[sizeof(record) / sizeof(record[0U])] = {0};
    const uint8_t *p_record                                      = (const uint8_t *)record;
    platform_err_t ret = bsp_storage_flash_program(BOARD_STORAGE_OTA_BASE_ADDRESS, p_record,
                                                   body_size, timeout_ms);
    if (PLATFORM_ERR_OK == ret) {
        ret = bsp_storage_flash_read(BOARD_STORAGE_OTA_BASE_ADDRESS, (uint8_t *)readback, body_size,
                                     timeout_ms);
    }
    if (PLATFORM_ERR_OK != ret || 0 != memcmp(record, readback, body_size)) {
        return (PLATFORM_ERR_OK == ret) ? PLATFORM_ERR_HW : ret;
    }

    ret = bsp_storage_flash_program(BOARD_STORAGE_OTA_BASE_ADDRESS + body_size,
                                    p_record + body_size, sizeof(record[0U]), timeout_ms);
    if (PLATFORM_ERR_OK == ret) {
        ret = bsp_storage_flash_read(BOARD_STORAGE_OTA_BASE_ADDRESS, (uint8_t *)readback,
                                     sizeof(readback), timeout_ms);
    }
    if (PLATFORM_ERR_OK == ret && 0 != memcmp(record, readback, sizeof(record))) {
        ret = PLATFORM_ERR_HW;
    }
    return ret;
}

platform_err_t firmware_staging_erase(uint32_t offset, uint32_t timeout_ms) {
    if (0U != offset % BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES || offset >= FIRMWARE_STAGING_CAPACITY) {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_storage_flash_sector_erase(FIRMWARE_STAGING_BASE + offset, timeout_ms);
}

platform_err_t
firmware_staging_write(uint32_t offset, const uint8_t *p_data, uint32_t size, uint32_t timeout_ms) {
    if (NULL == p_data || !firmware_staging_range_valid(offset, size)) {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_storage_flash_program(FIRMWARE_STAGING_BASE + offset, p_data, size, timeout_ms);
}

platform_err_t
firmware_staging_read(uint32_t offset, uint8_t *p_data, uint32_t size, uint32_t timeout_ms) {
    if (NULL == p_data || !firmware_staging_range_valid(offset, size)) {
        return PLATFORM_ERR_PARAM;
    }
    return bsp_storage_flash_read(FIRMWARE_STAGING_BASE + offset, p_data, size, timeout_ms);
}

/* end of file ----------------------------------------------------------------------------------*/
