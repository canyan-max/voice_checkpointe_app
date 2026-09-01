/**
 ******************************************************************************
 *@file               :   plat_resource.h
 *@brief              :   Define common logical Platform resource ID types.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PLAT_RESOURCE_H
#define PLAT_RESOURCE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* typedef ------------------------------------------------------------------*/
typedef uint8_t plat_gpio_id_t;
typedef uint8_t plat_uart_id_t;
typedef uint8_t plat_i2s_id_t;

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* PLAT_RESOURCE_H */
