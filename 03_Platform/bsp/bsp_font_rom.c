/**
 ******************************************************************************
 *@file               :   bsp_font_rom.c
 *@brief              :   Bind the board SPI font ROM capability.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "board_resources.h"
#include "bsp_font_rom.h"
#include "gt20l16s1y.h"
#include "plat_spi.h"
#include "plat_sys.h"

static gt20l16s1y_device_t font_rom_device;

_Static_assert(BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE ==
                   GT20L16S1Y_GLYPH_16X16_SIZE,
               "Font ROM glyph size mismatch");
_Static_assert(BSP_FONT_ROM_ASCII_GLYPH_8X16_SIZE ==
                   GT20L16S1Y_ASCII_8X16_SIZE,
               "Font ROM ASCII glyph size mismatch");

static gt20l16s1y_ret_t bsp_font_rom_ret_from_platform(platform_err_t ret)
{
    switch(ret)
    {
        case PLATFORM_ERR_OK:
            return GT20L16S1Y_RET_OK;
        case PLATFORM_ERR_PARAM:
            return GT20L16S1Y_RET_PARAM;
        case PLATFORM_ERR_BUSY:
            return GT20L16S1Y_RET_BUSY;
        case PLATFORM_ERR_TIMEOUT:
            return GT20L16S1Y_RET_TIMEOUT;
        default:
            return GT20L16S1Y_RET_IO;
    }
}

static platform_err_t bsp_font_rom_ret_to_platform(gt20l16s1y_ret_t ret)
{
    switch(ret)
    {
        case GT20L16S1Y_RET_OK:
            return PLATFORM_ERR_OK;
        case GT20L16S1Y_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case GT20L16S1Y_RET_BUSY:
            return PLATFORM_ERR_BUSY;
        case GT20L16S1Y_RET_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;
        default:
            return PLATFORM_ERR_HW;
    }
}

static gt20l16s1y_ret_t bsp_font_rom_spi_read(
    void          *p_context,
    const uint8_t *p_command,
    uint16_t       command_size,
    uint8_t       *p_data,
    uint16_t       data_size,
    uint8_t        dummy_byte,
    uint32_t       timeout_ms)
{
    (void)p_context;
    return bsp_font_rom_ret_from_platform(
        plat_spi_write_read(BOARD_SPI_FONT_ROM,
                            p_command,
                            command_size,
                            p_data,
                            data_size,
                            dummy_byte,
                            timeout_ms));
}

static void bsp_font_rom_delay_ms(void *p_context, uint32_t delay_ms)
{
    (void)p_context;
    plat_delay_ms(delay_ms);
}

platform_err_t bsp_font_rom_init(void)
{
    static const gt20l16s1y_io_t io =
    {
        bsp_font_rom_spi_read,
        bsp_font_rom_delay_ms,
        NULL
    };

    return bsp_font_rom_ret_to_platform(
        gt20l16s1y_init(&font_rom_device, &io));
}

platform_err_t bsp_font_rom_gb2312_16x16_read(
    uint8_t  high_byte,
    uint8_t  low_byte,
    uint8_t *p_bitmap,
    uint16_t bitmap_size,
    uint32_t timeout_ms)
{
    return bsp_font_rom_ret_to_platform(
        gt20l16s1y_gb2312_16x16_read(&font_rom_device,
                                     high_byte,
                                     low_byte,
                                     p_bitmap,
                                     bitmap_size,
                                     timeout_ms));
}

platform_err_t bsp_font_rom_ascii_8x16_read(
    uint8_t  ascii_code,
    uint8_t *p_bitmap,
    uint16_t bitmap_size,
    uint32_t timeout_ms)
{
    return bsp_font_rom_ret_to_platform(
        gt20l16s1y_ascii_8x16_read(&font_rom_device,
                                   ascii_code,
                                   p_bitmap,
                                   bitmap_size,
                                   timeout_ms));
}
