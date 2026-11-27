/**
 **************************************************************************************************
 *@file               :   firmware_upgrade_app.c
 *@brief              :   Stage and verify one firmware image at a time.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* includes -------------------------------------------------------------------------------------*/
#include "firmware_upgrade_app.h"
#include "firmware_staging_service.h"
#include "plat_log.h"
/* define   -------------------------------------------------------------------------------------*/

#define FIRMWARE_UPGRADE_BLOCK_BYTES (512U)
#define FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS (500U)

/* typedef --------------------------------------------------------------------------------------*/

typedef enum FIRMWARE_UPGRADE_STAGE_T {
    FIRMWARE_UPGRADE_IDLE = 0U,
    FIRMWARE_UPGRADE_ERASE_METADATA,
    FIRMWARE_UPGRADE_ERASE_IMAGE,
    FIRMWARE_UPGRADE_READ_SOURCE,
    FIRMWARE_UPGRADE_WRITE_IMAGE,
    FIRMWARE_UPGRADE_VERIFY_IMAGE,
    FIRMWARE_UPGRADE_PUBLISH,
} firmware_upgrade_stage_t;

typedef struct FIRMWARE_UPGRADE_SESSION_T {
    firmware_upgrade_io_t    io;
    firmware_upgrade_stage_t stage;
    uint32_t                 image_size;
    uint32_t                 offset;
    uint32_t                 source_crc;
    uint32_t                 flash_crc;
} firmware_upgrade_session_t;

/* variables ------------------------------------------------------------------------------------*/

static firmware_upgrade_lock_t    s_lock;
static firmware_upgrade_session_t s_session;
/* Owned by this session; reused for source data and QSPI verification. */
static uint8_t s_block[FIRMWARE_UPGRADE_BLOCK_BYTES];

/* private  functions  --------------------------------------------------------------------------*/

static uint32_t firmware_upgrade_crc32(uint32_t crc, const uint8_t *p_data, uint32_t size) {
    for (uint32_t i = 0U; i < size; ++i) {
        crc ^= p_data[i];
        for (uint32_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc >> 1U) ^ ((0U != (crc & 1U)) ? 0xEDB88320UL : 0U);
        }
    }
    return crc;
}

static firmware_upgrade_progress_t firmware_upgrade_complete(firmware_upgrade_progress_t progress) {
    s_session.stage = FIRMWARE_UPGRADE_IDLE;
    return progress;
}

static firmware_upgrade_progress_t firmware_upgrade_fail(const char *p_area, platform_err_t ret) {
    plat_log_w("Firmware upgrade %s failed: ret=%u stage=%u offset=%lu", p_area, (unsigned int)ret,
               (unsigned int)s_session.stage, (unsigned long)s_session.offset);
    return firmware_upgrade_complete(FIRMWARE_UPGRADE_PROGRESS_FAILED);
}

static firmware_upgrade_progress_t firmware_upgrade_read_source(uint32_t size) {
    const platform_err_t ret = s_session.io.pf_read(s_session.io.p_context, s_block, size);
    if (PLATFORM_ERR_OK == ret) {
        s_session.source_crc = firmware_upgrade_crc32(s_session.source_crc, s_block, size);
        s_session.stage      = FIRMWARE_UPGRADE_WRITE_IMAGE;
    } else if (PLATFORM_ERR_BUSY != ret) {
        return firmware_upgrade_fail("source", ret);
    }
    return FIRMWARE_UPGRADE_PROGRESS_WORKING;
}

static firmware_upgrade_progress_t firmware_upgrade_process_flash(uint32_t size) {
    if (0U == s_lock.pf_try_lock()) {
        return FIRMWARE_UPGRADE_PROGRESS_WORKING;
    }
    platform_err_t ret = PLATFORM_ERR_PARAM;
    switch (s_session.stage) {
        case FIRMWARE_UPGRADE_ERASE_METADATA:
            ret = firmware_staging_erase_metadata(FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS);
            break;
        case FIRMWARE_UPGRADE_ERASE_IMAGE:
            ret = firmware_staging_erase(s_session.offset, FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS);
            break;
        case FIRMWARE_UPGRADE_WRITE_IMAGE:
            ret = firmware_staging_write(s_session.offset, s_block, size,
                                         FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS);
            break;
        case FIRMWARE_UPGRADE_VERIFY_IMAGE:
            ret = firmware_staging_read(s_session.offset, s_block, size,
                                        FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS);
            break;
        case FIRMWARE_UPGRADE_PUBLISH:
            ret = firmware_staging_publish(s_session.image_size, s_session.flash_crc ^ UINT32_MAX,
                                           FIRMWARE_UPGRADE_FLASH_TIMEOUT_MS);
            break;
        default:
            break;
    }
    s_lock.pf_unlock();
    if (PLATFORM_ERR_OK != ret) {
        return firmware_upgrade_fail("QSPI", ret);
    }

    switch (s_session.stage) {
        case FIRMWARE_UPGRADE_ERASE_METADATA:
            s_session.stage = FIRMWARE_UPGRADE_ERASE_IMAGE;
            break;
        case FIRMWARE_UPGRADE_ERASE_IMAGE:
            s_session.offset += firmware_staging_sector_size();
            if (s_session.offset >= s_session.image_size) {
                s_session.offset = 0U;
                s_session.stage  = FIRMWARE_UPGRADE_READ_SOURCE;
            }
            break;
        case FIRMWARE_UPGRADE_WRITE_IMAGE:
            s_session.offset += size;
            if (s_session.offset == s_session.image_size) {
                s_session.offset = 0U;
                s_session.stage  = FIRMWARE_UPGRADE_VERIFY_IMAGE;
            } else {
                s_session.stage = FIRMWARE_UPGRADE_READ_SOURCE;
            }
            break;
        case FIRMWARE_UPGRADE_VERIFY_IMAGE:
            s_session.flash_crc = firmware_upgrade_crc32(s_session.flash_crc, s_block, size);
            s_session.offset += size;
            if (s_session.offset == s_session.image_size) {
                if (s_session.source_crc != s_session.flash_crc) {
                    plat_log_w("Firmware upgrade CRC mismatch: source=%08lX qspi=%08lX",
                               (unsigned long)(s_session.source_crc ^ UINT32_MAX),
                               (unsigned long)(s_session.flash_crc ^ UINT32_MAX));
                    return firmware_upgrade_complete(FIRMWARE_UPGRADE_PROGRESS_FAILED);
                }
                s_session.stage = FIRMWARE_UPGRADE_PUBLISH;
            }
            break;
        case FIRMWARE_UPGRADE_PUBLISH:
            return firmware_upgrade_complete(FIRMWARE_UPGRADE_PROGRESS_DONE);
        default:
            break;
    }
    return FIRMWARE_UPGRADE_PROGRESS_WORKING;
}

/* exported functions ---------------------------------------------------------------------------*/

void firmware_upgrade_app_init(const firmware_upgrade_lock_t *p_lock) {
    if (NULL != p_lock && FIRMWARE_UPGRADE_IDLE == s_session.stage) {
        s_lock = *p_lock;
    }
}

uint32_t firmware_upgrade_app_capacity(void) {
    return firmware_staging_capacity();
}

platform_err_t firmware_upgrade_app_start(uint32_t image_size, const firmware_upgrade_io_t *p_io) {
    if (FIRMWARE_UPGRADE_IDLE != s_session.stage) {
        return PLATFORM_ERR_BUSY;
    }
    if (NULL == p_io || NULL == p_io->pf_read || image_size <= 8U ||
        image_size > firmware_upgrade_app_capacity()) {
        return PLATFORM_ERR_PARAM;
    }
    if (NULL == s_lock.pf_try_lock || NULL == s_lock.pf_unlock) {
        return PLATFORM_ERR_HW;
    }
    s_session.io         = *p_io;
    s_session.image_size = image_size;
    s_session.offset     = 0U;
    s_session.source_crc = UINT32_MAX;
    s_session.flash_crc  = UINT32_MAX;
    s_session.stage      = FIRMWARE_UPGRADE_ERASE_METADATA;
    return PLATFORM_ERR_OK;
}

firmware_upgrade_progress_t firmware_upgrade_app_poll(uint32_t *p_verified_crc32) {
    if (FIRMWARE_UPGRADE_IDLE == s_session.stage) {
        return FIRMWARE_UPGRADE_PROGRESS_IDLE;
    }
    uint32_t size = s_session.image_size - s_session.offset;
    if (size > sizeof(s_block)) {
        size = sizeof(s_block);
    }
    if (FIRMWARE_UPGRADE_READ_SOURCE == s_session.stage) {
        return firmware_upgrade_read_source(size);
    }

    const firmware_upgrade_progress_t progress = firmware_upgrade_process_flash(size);
    if (FIRMWARE_UPGRADE_PROGRESS_DONE == progress && NULL != p_verified_crc32) {
        *p_verified_crc32 = s_session.flash_crc ^ UINT32_MAX;
    }
    return progress;
}

void firmware_upgrade_app_cancel(void) {
    s_session.stage = FIRMWARE_UPGRADE_IDLE;
}

/* end of file ----------------------------------------------------------------------------------*/
