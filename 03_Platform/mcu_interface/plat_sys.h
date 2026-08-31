/**
 ******************************************************************************
 *@file               :   plat_sys.h
 *@brief              :   Provide the HAL APIs of description.
 *@version            :   V1.0 
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PLAT_SYS_H
#define PLAT_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>                               /* stdint lib header file. */
/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/

/* exported types -----------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
uint32_t plat_tick_get_ms(void);
void     plat_delay_ms(uint32_t ms);
void     plat_dcache_invalidate(void *p_addr, int32_t size);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_SYS_H */

