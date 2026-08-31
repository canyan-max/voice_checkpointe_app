/**
 ******************************************************************************
 *@file               :   plat_error.h
 *@brief              :   Provide the HAL APIs of description.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PLATFORM_ERROR_H
#define PLATFORM_ERROR_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* typedef ------------------------------------------------------------------*/
typedef enum
{
    PLATFORM_ERR_OK       = 0x00U, /**< Operation completed successfully. */
    PLATFORM_ERR_PARAM    = 0x01U, /**< Invalid parameter. */
    PLATFORM_ERR_HW       = 0x02U, /**< Hardware error. */
    PLATFORM_ERR_BUSY     = 0x03U, /**< Resource busy. */
    PLATFORM_ERR_TIMEOUT  = 0x04U, /**< Operation timed out. */
} platform_err_t;

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* PLATFORM_ERROR_H */
