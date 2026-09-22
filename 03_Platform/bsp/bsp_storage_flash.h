/**
 ******************************************************************************
 *@file               :   bsp_storage_flash.h
 *@brief              :   Board nonvolatile storage Flash capability.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef BSP_STORAGE_FLASH_H
#define BSP_STORAGE_FLASH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"

#define BSP_STORAGE_FLASH_CAPACITY_BYTES       0x01000000UL
#define BSP_STORAGE_FLASH_PAGE_SIZE_BYTES             256U
#define BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES          4096U
#define BSP_STORAGE_FLASH_JEDEC_ID_SIZE                  3U

typedef struct BSP_STORAGE_FLASH_INFO_T
{
    uint32_t capacity_bytes;
    uint32_t page_size_bytes;
    uint32_t sector_size_bytes;
    uint8_t  jedec_id[BSP_STORAGE_FLASH_JEDEC_ID_SIZE];
} bsp_storage_flash_info_t;

/**
 * @brief Initialize the board storage Flash and enable its quad-read path.
 * @note This blocking BSP owns the single board Flash instance. Callers must
 *       serialize concurrent access.
 */
platform_err_t bsp_storage_flash_init(uint32_t timeout_ms);

platform_err_t bsp_storage_flash_info_get(
    bsp_storage_flash_info_t *p_info);

/**
 * @brief Read an arbitrary byte range through the board's fast read path.
 */
platform_err_t bsp_storage_flash_read(uint32_t address,
                                      uint8_t *p_data,
                                      uint32_t data_size,
                                      uint32_t timeout_ms);

/**
 * @brief Program an arbitrary byte range and split it at page boundaries.
 * @note The destination must already be erased. Programming only changes bits
 *       from 1 to 0.
 */
platform_err_t bsp_storage_flash_program(uint32_t address,
                                         const uint8_t *p_data,
                                         uint32_t data_size,
                                         uint32_t timeout_ms);

/**
 * @brief Erase one 4 KiB sector. Address must be sector aligned.
 */
platform_err_t bsp_storage_flash_sector_erase(uint32_t address,
                                              uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_STORAGE_FLASH_H */
