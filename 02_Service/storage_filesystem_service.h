/**
 ******************************************************************************
 *@file               :   storage_filesystem_service.h
 *@brief              :   Nonvolatile storage filesystem service.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef STORAGE_FILESYSTEM_SERVICE_H
#define STORAGE_FILESYSTEM_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "board_storage_layout.h"

#define STORAGE_FILESYSTEM_SERVICE_BASE_ADDRESS      \
    BOARD_STORAGE_FILESYSTEM_BASE_ADDRESS
#define STORAGE_FILESYSTEM_SERVICE_REGION_SIZE_BYTES \
    BOARD_STORAGE_FILESYSTEM_SIZE_BYTES

typedef enum STORAGE_FILESYSTEM_SERVICE_RET_T
{
    STORAGE_FILESYSTEM_SERVICE_RET_OK = 0,
    STORAGE_FILESYSTEM_SERVICE_RET_PARAM,
    STORAGE_FILESYSTEM_SERVICE_RET_HW,
    STORAGE_FILESYSTEM_SERVICE_RET_BUSY,
    STORAGE_FILESYSTEM_SERVICE_RET_TIMEOUT,
    STORAGE_FILESYSTEM_SERVICE_RET_NOT_FOUND,
    STORAGE_FILESYSTEM_SERVICE_RET_CORRUPT,
    STORAGE_FILESYSTEM_SERVICE_RET_STATE
} storage_filesystem_service_ret_t;

typedef enum STORAGE_FILESYSTEM_SERVICE_OPEN_MODE_T
{
    STORAGE_FILESYSTEM_SERVICE_OPEN_READ = 0,
    STORAGE_FILESYSTEM_SERVICE_OPEN_WRITE_TRUNCATE
} storage_filesystem_service_open_mode_t;

/**
 * @brief Initialize the storage Flash and mount its filesystem.
 * @note If mounting fails, formatting is allowed only when the complete
 *       reserved region is blank. The module owns one filesystem instance and
 *       one open file; callers must serialize access.
 */
storage_filesystem_service_ret_t storage_filesystem_service_init(
    uint32_t timeout_ms,
    uint8_t *p_was_formatted);

storage_filesystem_service_ret_t storage_filesystem_service_deinit(void);

storage_filesystem_service_ret_t storage_filesystem_service_file_open(
    const char *p_path,
    storage_filesystem_service_open_mode_t mode);

storage_filesystem_service_ret_t storage_filesystem_service_file_size_get(
    uint32_t *p_size);

storage_filesystem_service_ret_t storage_filesystem_service_file_read(
    uint8_t *p_data,
    uint32_t data_size,
    uint32_t *p_read_size);

storage_filesystem_service_ret_t storage_filesystem_service_file_write(
    const uint8_t *p_data,
    uint32_t data_size,
    uint32_t *p_write_size);

storage_filesystem_service_ret_t storage_filesystem_service_file_sync(void);

storage_filesystem_service_ret_t storage_filesystem_service_file_close(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_FILESYSTEM_SERVICE_H */
