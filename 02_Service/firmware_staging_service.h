/**
 **************************************************************************************************
 *@file               :   firmware_staging_service.h
 *@brief              :   Manage the reserved external Flash firmware staging area.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef FIRMWARE_STAGING_SERVICE_H
#define FIRMWARE_STAGING_SERVICE_H

/* includes -------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif // end of __cplusplus
/* define ---------------------------------------------------------------------------------------*/

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

/* function  ------------------------------------------------------------------------------------*/

/* Offset zero is the start of the image, one sector after the metadata. */
uint32_t firmware_staging_capacity(void);
uint32_t firmware_staging_sector_size(void);

/* Caller serializes these operations with any other access to board Flash. */
platform_err_t firmware_staging_erase_metadata(uint32_t timeout_ms);
platform_err_t firmware_staging_publish(uint32_t image_size, uint32_t crc32, uint32_t timeout_ms);
platform_err_t firmware_staging_erase(uint32_t offset, uint32_t timeout_ms);
platform_err_t
firmware_staging_write(uint32_t offset, const uint8_t *p_data, uint32_t size, uint32_t timeout_ms);
platform_err_t
firmware_staging_read(uint32_t offset, uint8_t *p_data, uint32_t size, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif // end of __cplusplus

#endif /* FIRMWARE_STAGING_SERVICE_H */
/* end of file ----------------------------------------------------------------------------------*/
