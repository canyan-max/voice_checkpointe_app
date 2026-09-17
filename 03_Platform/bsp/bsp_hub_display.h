/**
 ******************************************************************************
 *@file               :   bsp_hub_display.h
 *@brief              :   Drive the board HUB display interface.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BSP_HUB_DISPLAY_H
#define BSP_HUB_DISPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"

/* define -------------------------------------------------------------------*/
#define BSP_HUB_DISPLAY_SCAN_ROW_COUNT       4U
#define BSP_HUB_DISPLAY_SCAN_ROW_BYTES      32U

/* functions ----------------------------------------------------------------*/
platform_err_t bsp_hub_display_init(void);

platform_err_t bsp_hub_display_start(void);

platform_err_t bsp_hub_display_scan_row(const uint8_t *p_data,
                                        uint16_t data_length,
                                        uint8_t row);

platform_err_t bsp_hub_display_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_HUB_DISPLAY_H */
