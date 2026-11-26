/**
 **************************************************************************************************
 *@file               :   checkpoint_config_storage.h
 *@brief              :   Load and save checkpoint configuration through LittleFS.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* define to prevent recursive inclusion --------------------------------------------------------*/
#ifndef CHECKPOINT_CONFIG_STORAGE_H
#define CHECKPOINT_CONFIG_STORAGE_H

/* includes -------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "checkpoint_config.h"
#include "storage_filesystem_service.h"

#ifdef __cplusplus
extern "C" {
#endif // end of __cplusplus
/* define ---------------------------------------------------------------------------------------*/

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

/* function  ------------------------------------------------------------------------------------*/
/* Caller serializes access and mounts storage before loading or saving. */
/* On load failure the caller's configuration is unchanged. */
storage_filesystem_service_ret_t checkpoint_config_storage_load(checkpoint_config_t *p_config);
storage_filesystem_service_ret_t
checkpoint_config_storage_save(const checkpoint_config_t *p_config);

#ifdef __cplusplus
}
#endif // end of __cplusplus

#endif /* CHECKPOINT_CONFIG_STORAGE_H */
/* end of file ----------------------------------------------------------------------------------*/
