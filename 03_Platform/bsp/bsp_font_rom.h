/**
 ******************************************************************************
 *@file               :   bsp_font_rom.h
 *@brief              :   Board font ROM capability.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef BSP_FONT_ROM_H
#define BSP_FONT_ROM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"

#define BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE  32U
#define BSP_FONT_ROM_ASCII_GLYPH_8X16_SIZE    16U

platform_err_t bsp_font_rom_init(void);

/**
 * @brief Read one 16 x 16 GB2312 glyph from the board font ROM.
 * @note p_bitmap is valid only when PLATFORM_ERR_OK is returned.
 */
platform_err_t bsp_font_rom_gb2312_16x16_read(
    uint8_t  high_byte,
    uint8_t  low_byte,
    uint8_t *p_bitmap,
    uint16_t bitmap_size,
    uint32_t timeout_ms);

platform_err_t bsp_font_rom_ascii_8x16_read(
    uint8_t  ascii_code,
    uint8_t *p_bitmap,
    uint16_t bitmap_size,
    uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_FONT_ROM_H */
