/**
 **************************************************************************************************
 *@file               :   firmware_upgrade_app.h
 *@brief              :   Stage and verify one firmware image at a time.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef FIRMWARE_UPGRADE_APP_H
#define FIRMWARE_UPGRADE_APP_H

/* includes -------------------------------------------------------------------------------------*/
#include <stdint.h>
// #include <stddef.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif // end of __cplusplus
/* define ---------------------------------------------------------------------------------------*/

/* typedef --------------------------------------------------------------------------------------*/

typedef struct FIRMWARE_UPGRADE_IO_T {
    /* Fill exactly size bytes; BUSY means the next block is not ready yet. */
    platform_err_t (*pf_read)(void *p_context, uint8_t *p_data, uint32_t size);
    void *p_context;
} firmware_upgrade_io_t;

typedef struct FIRMWARE_UPGRADE_LOCK_T {
    uint8_t (*pf_try_lock)(void);
    void (*pf_unlock)(void);
} firmware_upgrade_lock_t;

typedef enum FIRMWARE_UPGRADE_PROGRESS_T {
    FIRMWARE_UPGRADE_PROGRESS_IDLE = 0U,
    FIRMWARE_UPGRADE_PROGRESS_WORKING,
    FIRMWARE_UPGRADE_PROGRESS_DONE,
    FIRMWARE_UPGRADE_PROGRESS_FAILED,
} firmware_upgrade_progress_t;

/* variables ------------------------------------------------------------------------------------*/

/* function  ------------------------------------------------------------------------------------*/

/* All session calls belong to one task. Sources in other tasks must hand work to it. */
void           firmware_upgrade_app_init(const firmware_upgrade_lock_t *p_lock);
uint32_t       firmware_upgrade_app_capacity(void);
platform_err_t firmware_upgrade_app_start(uint32_t image_size, const firmware_upgrade_io_t *p_io);
/* The optional CRC output is written only when verification succeeds. */
firmware_upgrade_progress_t firmware_upgrade_app_poll(uint32_t *p_verified_crc32);
void                        firmware_upgrade_app_cancel(void);

#ifdef __cplusplus
}
#endif // end of __cplusplus

#endif /* FIRMWARE_UPGRADE_APP_H */
/* end of file ----------------------------------------------------------------------------------*/
