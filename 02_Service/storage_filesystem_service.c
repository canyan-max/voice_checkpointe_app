/**
 ******************************************************************************
 *@file               :   storage_filesystem_service.c
 *@brief              :   Manage LittleFS over the board storage Flash.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "storage_filesystem_service.h"
#include "bsp_storage_flash.h"
#include "lfs.h"

#define STORAGE_FILESYSTEM_SERVICE_BLOCK_SIZE          (4096U)
#define STORAGE_FILESYSTEM_SERVICE_BLOCK_COUNT         (3584U)
#define STORAGE_FILESYSTEM_SERVICE_READ_SIZE           (16U  )
#define STORAGE_FILESYSTEM_SERVICE_PROGRAM_SIZE        (16U  )
#define STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE          (256U )
#define STORAGE_FILESYSTEM_SERVICE_LOOKAHEAD_SIZE      (64U  )
#define STORAGE_FILESYSTEM_SERVICE_BLOCK_CYCLES        (500U )

static uint8_t
    storage_filesystem_read_buffer[STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE];
static uint8_t
    storage_filesystem_program_buffer[STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE];
static uint8_t storage_filesystem_lookahead_buffer
    [STORAGE_FILESYSTEM_SERVICE_LOOKAHEAD_SIZE];
static uint8_t
    storage_filesystem_file_buffer[STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE];
static lfs_t      storage_filesystem;
static lfs_file_t storage_filesystem_file;
static uint32_t   storage_filesystem_timeout_ms;
static uint8_t    storage_filesystem_mounted;
static uint8_t    storage_filesystem_file_opened;

static int storage_filesystem_service_read(const struct lfs_config *p_config,
                                           lfs_block_t              block,
                                           lfs_off_t                offset,
                                           void                    *p_buffer,
                                           lfs_size_t               size);
static int storage_filesystem_service_program(const struct lfs_config *p_config,
                                              lfs_block_t              block,
                                              lfs_off_t                offset,
                                              const void              *p_buffer,
                                              lfs_size_t               size);
static int storage_filesystem_service_erase(const struct lfs_config *p_config,
                                            lfs_block_t              block);
static int storage_filesystem_service_sync(const struct lfs_config *p_config);

static const struct lfs_file_config storage_filesystem_file_config = {
    .buffer = storage_filesystem_file_buffer};

static const struct lfs_config storage_filesystem_config =
    {.context          = NULL,
     .read             = storage_filesystem_service_read,
     .prog             = storage_filesystem_service_program,
     .erase            = storage_filesystem_service_erase,
     .sync             = storage_filesystem_service_sync,
     .read_size        = STORAGE_FILESYSTEM_SERVICE_READ_SIZE,
     .prog_size        = STORAGE_FILESYSTEM_SERVICE_PROGRAM_SIZE,
     .block_size       = STORAGE_FILESYSTEM_SERVICE_BLOCK_SIZE,
     .block_count      = STORAGE_FILESYSTEM_SERVICE_BLOCK_COUNT,
     .block_cycles     = STORAGE_FILESYSTEM_SERVICE_BLOCK_CYCLES,
     .cache_size       = STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE,
     .lookahead_size   = STORAGE_FILESYSTEM_SERVICE_LOOKAHEAD_SIZE,
     .read_buffer      = storage_filesystem_read_buffer,
     .prog_buffer      = storage_filesystem_program_buffer,
     .lookahead_buffer = storage_filesystem_lookahead_buffer};

_Static_assert(STORAGE_FILESYSTEM_SERVICE_BLOCK_SIZE ==
                   BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES,
               "LittleFS erase block size mismatch");
_Static_assert(STORAGE_FILESYSTEM_SERVICE_REGION_SIZE_BYTES ==
                   (STORAGE_FILESYSTEM_SERVICE_BLOCK_SIZE *
                    STORAGE_FILESYSTEM_SERVICE_BLOCK_COUNT),
               "LittleFS region geometry mismatch");
_Static_assert((STORAGE_FILESYSTEM_SERVICE_BASE_ADDRESS +
                STORAGE_FILESYSTEM_SERVICE_REGION_SIZE_BYTES) <=
                   BSP_STORAGE_FLASH_CAPACITY_BYTES,
               "LittleFS region exceeds storage Flash");
_Static_assert(BOARD_STORAGE_CAPACITY_BYTES == BSP_STORAGE_FLASH_CAPACITY_BYTES,
               "Board storage capacity mismatch");
_Static_assert(0U == (STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE %
                      STORAGE_FILESYSTEM_SERVICE_READ_SIZE),
               "LittleFS cache/read size mismatch");
_Static_assert(0U == (STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE %
                      STORAGE_FILESYSTEM_SERVICE_PROGRAM_SIZE),
               "LittleFS cache/program size mismatch");
_Static_assert(0U == (STORAGE_FILESYSTEM_SERVICE_BLOCK_SIZE %
                      STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE),
               "LittleFS block/cache size mismatch");
_Static_assert((0U != STORAGE_FILESYSTEM_SERVICE_LOOKAHEAD_SIZE) &&
                   (0U == (STORAGE_FILESYSTEM_SERVICE_LOOKAHEAD_SIZE % 8U)),
               "LittleFS lookahead size mismatch");

static storage_filesystem_service_ret_t
storage_filesystem_service_ret_from_lfs(int ret)
{
    switch(ret)
    {
        case LFS_ERR_OK:
            return STORAGE_FILESYSTEM_SERVICE_RET_OK;
        case LFS_ERR_INVAL:
            return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
        case LFS_ERR_NOENT:
            return STORAGE_FILESYSTEM_SERVICE_RET_NOT_FOUND;
        case LFS_ERR_CORRUPT:
            return STORAGE_FILESYSTEM_SERVICE_RET_CORRUPT;
        default:
            return STORAGE_FILESYSTEM_SERVICE_RET_HW;
    }
}

static int storage_filesystem_service_ret_to_lfs(platform_err_t ret)
{
    return (PLATFORM_ERR_OK == ret) ? LFS_ERR_OK : LFS_ERR_IO;
}

static int
storage_filesystem_service_address_get(const struct lfs_config *p_config,
                                       lfs_block_t              block,
                                       lfs_off_t                offset,
                                       lfs_size_t               size,
                                       uint32_t                *p_address)
{
    if((NULL == p_config) || (NULL == p_address) ||
       (block >= p_config->block_count) || (offset > p_config->block_size) ||
       (size > (p_config->block_size - offset)))
    {
        return LFS_ERR_INVAL;
    }

    *p_address = STORAGE_FILESYSTEM_SERVICE_BASE_ADDRESS +
                 ((uint32_t)block * p_config->block_size) + offset;
    return LFS_ERR_OK;
}

static int storage_filesystem_service_read(const struct lfs_config *p_config,
                                           lfs_block_t              block,
                                           lfs_off_t                offset,
                                           void                    *p_buffer,
                                           lfs_size_t               size)
{
    uint32_t address;
    int      ret;

    if((NULL == p_config) || (NULL == p_buffer) || (0U == size) ||
       (0U != (offset % p_config->read_size)) ||
       (0U != (size % p_config->read_size)))
    {
        return LFS_ERR_INVAL;
    }
    ret = storage_filesystem_service_address_get(p_config, block, offset, size,
                                                 &address);
    if(LFS_ERR_OK != ret)
    {
        return ret;
    }
    return storage_filesystem_service_ret_to_lfs(
        bsp_storage_flash_read(address, (uint8_t *)p_buffer, size,
                               storage_filesystem_timeout_ms));
}

static int storage_filesystem_service_program(const struct lfs_config *p_config,
                                              lfs_block_t              block,
                                              lfs_off_t                offset,
                                              const void              *p_buffer,
                                              lfs_size_t               size)
{
    uint32_t address;
    int      ret;

    if((NULL == p_config) || (NULL == p_buffer) || (0U == size) ||
       (0U != (offset % p_config->prog_size)) ||
       (0U != (size % p_config->prog_size)))
    {
        return LFS_ERR_INVAL;
    }
    ret = storage_filesystem_service_address_get(p_config, block, offset, size,
                                                 &address);
    if(LFS_ERR_OK != ret)
    {
        return ret;
    }
    return storage_filesystem_service_ret_to_lfs(
        bsp_storage_flash_program(address, (const uint8_t *)p_buffer, size,
                                  storage_filesystem_timeout_ms));
}

static int storage_filesystem_service_erase(const struct lfs_config *p_config,
                                            lfs_block_t              block)
{
    uint32_t address;
    int      ret;

    if(NULL == p_config)
    {
        return LFS_ERR_INVAL;
    }
    ret = storage_filesystem_service_address_get(p_config, block, 0U,
                                                 p_config->block_size,
                                                 &address);
    if(LFS_ERR_OK != ret)
    {
        return ret;
    }
    return storage_filesystem_service_ret_to_lfs(
        bsp_storage_flash_sector_erase(address, storage_filesystem_timeout_ms));
}

static int storage_filesystem_service_sync(const struct lfs_config *p_config)
{
    (void)p_config;
    return LFS_ERR_OK;
}

static storage_filesystem_service_ret_t
storage_filesystem_service_blank_check(uint8_t *p_is_blank)
{
    platform_err_t ret;
    uint32_t       offset;
    uint32_t       index;

    if(NULL == p_is_blank)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    *p_is_blank = 1U;
    for(offset = 0U; offset < STORAGE_FILESYSTEM_SERVICE_REGION_SIZE_BYTES;
        offset += STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE)
    {
        ret = bsp_storage_flash_read(STORAGE_FILESYSTEM_SERVICE_BASE_ADDRESS +
                                         offset,
                                     storage_filesystem_read_buffer,
                                     STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE,
                                     storage_filesystem_timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            return STORAGE_FILESYSTEM_SERVICE_RET_HW;
        }
        for(index = 0U; index < STORAGE_FILESYSTEM_SERVICE_CACHE_SIZE; index++)
        {
            if(0xFFU != storage_filesystem_read_buffer[index])
            {
                *p_is_blank = 0U;
                return STORAGE_FILESYSTEM_SERVICE_RET_OK;
            }
        }
    }
    return STORAGE_FILESYSTEM_SERVICE_RET_OK;
}

storage_filesystem_service_ret_t
storage_filesystem_service_init(uint32_t timeout_ms, uint8_t *p_was_formatted)
{
    storage_filesystem_service_ret_t ret;
    platform_err_t                   flash_ret;
    uint8_t                          is_blank;
    int                              lfs_ret;

    if((0U == timeout_ms) || (NULL == p_was_formatted))
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    if(0U != storage_filesystem_mounted)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_BUSY;
    }

    *p_was_formatted              = 0U;
    storage_filesystem_timeout_ms = timeout_ms;
    flash_ret                     = bsp_storage_flash_init(timeout_ms);
    if(PLATFORM_ERR_OK != flash_ret)
    {
        return (PLATFORM_ERR_TIMEOUT == flash_ret)
                   ? STORAGE_FILESYSTEM_SERVICE_RET_TIMEOUT
                   : STORAGE_FILESYSTEM_SERVICE_RET_HW;
    }

    lfs_ret = lfs_mount(&storage_filesystem, &storage_filesystem_config);
    if(LFS_ERR_OK != lfs_ret)
    {
        ret = storage_filesystem_service_blank_check(&is_blank);
        if(STORAGE_FILESYSTEM_SERVICE_RET_OK != ret)
        {
            return ret;
        }
        if(0U == is_blank)
        {
            return storage_filesystem_service_ret_from_lfs(lfs_ret);
        }
        lfs_ret = lfs_format(&storage_filesystem, &storage_filesystem_config);
        if(LFS_ERR_OK != lfs_ret)
        {
            return storage_filesystem_service_ret_from_lfs(lfs_ret);
        }
        lfs_ret = lfs_mount(&storage_filesystem, &storage_filesystem_config);
        if(LFS_ERR_OK != lfs_ret)
        {
            return storage_filesystem_service_ret_from_lfs(lfs_ret);
        }
        *p_was_formatted = 1U;
    }

    storage_filesystem_mounted = 1U;
    return STORAGE_FILESYSTEM_SERVICE_RET_OK;
}

storage_filesystem_service_ret_t storage_filesystem_service_deinit(void)
{
    int ret;

    if((0U == storage_filesystem_mounted) ||
       (0U != storage_filesystem_file_opened))
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    ret = lfs_unmount(&storage_filesystem);
    if(LFS_ERR_OK == ret)
    {
        storage_filesystem_mounted = 0U;
    }
    return storage_filesystem_service_ret_from_lfs(ret);
}

storage_filesystem_service_ret_t storage_filesystem_service_file_open(
    const char *p_path, storage_filesystem_service_open_mode_t mode)
{
    int flags;
    int ret;

    if(NULL == p_path)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    if((0U == storage_filesystem_mounted) ||
       (0U != storage_filesystem_file_opened))
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    switch(mode)
    {
        case STORAGE_FILESYSTEM_SERVICE_OPEN_READ:
            flags = LFS_O_RDONLY;
            break;
        case STORAGE_FILESYSTEM_SERVICE_OPEN_WRITE_TRUNCATE:
            flags = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
            break;
        default:
            return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }

    ret = lfs_file_opencfg(&storage_filesystem, &storage_filesystem_file,
                           p_path, flags, &storage_filesystem_file_config);
    if(LFS_ERR_OK == ret)
    {
        storage_filesystem_file_opened = 1U;
    }
    return storage_filesystem_service_ret_from_lfs(ret);
}

storage_filesystem_service_ret_t
storage_filesystem_service_file_size_get(uint32_t *p_size)
{
    lfs_soff_t size;

    if(NULL == p_size)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    if(0U == storage_filesystem_file_opened)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    size = lfs_file_size(&storage_filesystem, &storage_filesystem_file);
    if(size < 0)
    {
        return storage_filesystem_service_ret_from_lfs((int)size);
    }
    *p_size = (uint32_t)size;
    return STORAGE_FILESYSTEM_SERVICE_RET_OK;
}

storage_filesystem_service_ret_t storage_filesystem_service_file_read(
    uint8_t *p_data, uint32_t data_size, uint32_t *p_read_size)
{
    lfs_ssize_t size;

    if((NULL == p_data) || (0U == data_size) || (NULL == p_read_size))
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    if(0U == storage_filesystem_file_opened)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    size = lfs_file_read(&storage_filesystem, &storage_filesystem_file, p_data,
                         data_size);
    if(size < 0)
    {
        return storage_filesystem_service_ret_from_lfs((int)size);
    }
    *p_read_size = (uint32_t)size;
    return STORAGE_FILESYSTEM_SERVICE_RET_OK;
}

storage_filesystem_service_ret_t storage_filesystem_service_file_write(
    const uint8_t *p_data, uint32_t data_size, uint32_t *p_write_size)
{
    lfs_ssize_t size;

    if((NULL == p_data) || (0U == data_size) || (NULL == p_write_size))
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_PARAM;
    }
    if(0U == storage_filesystem_file_opened)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    size = lfs_file_write(&storage_filesystem, &storage_filesystem_file, p_data,
                          data_size);
    if(size < 0)
    {
        return storage_filesystem_service_ret_from_lfs((int)size);
    }
    *p_write_size = (uint32_t)size;
    return STORAGE_FILESYSTEM_SERVICE_RET_OK;
}

storage_filesystem_service_ret_t storage_filesystem_service_file_sync(void)
{
    if(0U == storage_filesystem_file_opened)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    return storage_filesystem_service_ret_from_lfs(
        lfs_file_sync(&storage_filesystem, &storage_filesystem_file));
}

storage_filesystem_service_ret_t storage_filesystem_service_file_close(void)
{
    int ret;

    if(0U == storage_filesystem_file_opened)
    {
        return STORAGE_FILESYSTEM_SERVICE_RET_STATE;
    }
    ret = lfs_file_close(&storage_filesystem, &storage_filesystem_file);
    if(LFS_ERR_OK == ret)
    {
        storage_filesystem_file_opened = 0U;
    }
    return storage_filesystem_service_ret_from_lfs(ret);
}
