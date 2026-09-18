/**
 ******************************************************************************
 *@file               :   gt20l16s1y.c
 *@brief              :   Implement GT20L16S1Y GB2312 font reads.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "gt20l16s1y.h"

#define GT20L16S1Y_POWER_UP_DELAY_MS      10U
#define GT20L16S1Y_COMMAND_READ           0x03U
#define GT20L16S1Y_READ_HEADER_SIZE        4U
#define GT20L16S1Y_GB2312_COLUMNS         94U
#define GT20L16S1Y_GB2312_CHINESE_OFFSET 846U
#define GT20L16S1Y_GB2312_A9_OFFSET      282U
#define GT20L16S1Y_ASCII_FIRST           0x20U
#define GT20L16S1Y_ASCII_LAST            0x7EU
#define GT20L16S1Y_ASCII_8X16_BASE       0x03B7C0UL
#define GT20L16S1Y_DUMMY_BYTE             0xFFU

static gt20l16s1y_ret_t gt20l16s1y_validate(
    const gt20l16s1y_device_t *p_device)
{
    if(NULL == p_device)
    {
        return GT20L16S1Y_RET_PARAM;
    }
    if((0U == p_device->is_initialized) ||
       (NULL == p_device->io.read) ||
       (NULL == p_device->io.delay_ms))
    {
        return GT20L16S1Y_RET_NOT_READY;
    }
    return GT20L16S1Y_RET_OK;
}

static gt20l16s1y_ret_t gt20l16s1y_gb2312_address_get(
    uint8_t   high_byte,
    uint8_t   low_byte,
    uint32_t *p_address)
{
    uint32_t glyph_index;

    if((NULL == p_address) || (low_byte < 0xA1U) ||
       (low_byte > 0xFEU))
    {
        return GT20L16S1Y_RET_PARAM;
    }

    if((high_byte >= 0xA1U) && (high_byte <= 0xA3U))
    {
        glyph_index = ((uint32_t)(high_byte - 0xA1U) *
                       GT20L16S1Y_GB2312_COLUMNS) +
                      (uint32_t)(low_byte - 0xA1U);
    }
    else if(0xA9U == high_byte)
    {
        glyph_index = GT20L16S1Y_GB2312_A9_OFFSET +
                      (uint32_t)(low_byte - 0xA1U);
    }
    else if((high_byte >= 0xB0U) && (high_byte <= 0xF7U))
    {
        glyph_index = ((uint32_t)(high_byte - 0xB0U) *
                       GT20L16S1Y_GB2312_COLUMNS) +
                      (uint32_t)(low_byte - 0xA1U) +
                      GT20L16S1Y_GB2312_CHINESE_OFFSET;
    }
    else
    {
        return GT20L16S1Y_RET_PARAM;
    }

    *p_address = glyph_index * GT20L16S1Y_GLYPH_16X16_SIZE;
    return GT20L16S1Y_RET_OK;
}

static gt20l16s1y_ret_t gt20l16s1y_read(
    gt20l16s1y_device_t *p_device,
    uint32_t             address,
    uint8_t             *p_data,
    uint16_t             data_size,
    uint32_t             timeout_ms)
{
    uint8_t command[GT20L16S1Y_READ_HEADER_SIZE];
    gt20l16s1y_ret_t ret;

    if((NULL == p_data) || (0U == data_size) || (0U == timeout_ms))
    {
        return GT20L16S1Y_RET_PARAM;
    }
    ret = gt20l16s1y_validate(p_device);
    if(GT20L16S1Y_RET_OK != ret)
    {
        return ret;
    }

    command[0] = GT20L16S1Y_COMMAND_READ;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;

    return p_device->io.read(p_device->io.p_context,
                             command,
                             (uint16_t)sizeof(command),
                             p_data,
                             data_size,
                             GT20L16S1Y_DUMMY_BYTE,
                             timeout_ms);
}

gt20l16s1y_ret_t gt20l16s1y_init(gt20l16s1y_device_t   *p_device,
                                  const gt20l16s1y_io_t *p_io)
{
    if((NULL == p_device) || (NULL == p_io) ||
       (NULL == p_io->read) || (NULL == p_io->delay_ms))
    {
        return GT20L16S1Y_RET_PARAM;
    }

    p_device->io = *p_io;
    p_device->is_initialized = 0U;
    p_device->io.delay_ms(p_device->io.p_context,
                          GT20L16S1Y_POWER_UP_DELAY_MS);
    p_device->is_initialized = 1U;
    return GT20L16S1Y_RET_OK;
}

gt20l16s1y_ret_t gt20l16s1y_gb2312_16x16_read(
    gt20l16s1y_device_t *p_device,
    uint8_t              high_byte,
    uint8_t              low_byte,
    uint8_t             *p_bitmap,
    uint16_t             bitmap_size,
    uint32_t             timeout_ms)
{
    gt20l16s1y_ret_t ret;
    uint32_t address;

    if((NULL == p_bitmap) ||
       (GT20L16S1Y_GLYPH_16X16_SIZE != bitmap_size) ||
       (0U == timeout_ms))
    {
        return GT20L16S1Y_RET_PARAM;
    }
    ret = gt20l16s1y_gb2312_address_get(high_byte,
                                        low_byte,
                                        &address);
    if(GT20L16S1Y_RET_OK != ret)
    {
        return ret;
    }

    return gt20l16s1y_read(p_device,
                           address,
                           p_bitmap,
                           bitmap_size,
                           timeout_ms);
}

gt20l16s1y_ret_t gt20l16s1y_ascii_8x16_read(
    gt20l16s1y_device_t *p_device,
    uint8_t              ascii_code,
    uint8_t             *p_bitmap,
    uint16_t             bitmap_size,
    uint32_t             timeout_ms)
{
    uint32_t address;

    if((ascii_code < GT20L16S1Y_ASCII_FIRST) ||
       (ascii_code > GT20L16S1Y_ASCII_LAST) ||
       (GT20L16S1Y_ASCII_8X16_SIZE != bitmap_size))
    {
        return GT20L16S1Y_RET_PARAM;
    }

    address = GT20L16S1Y_ASCII_8X16_BASE +
              ((uint32_t)(ascii_code - GT20L16S1Y_ASCII_FIRST) *
               GT20L16S1Y_ASCII_8X16_SIZE);
    return gt20l16s1y_read(p_device,
                           address,
                           p_bitmap,
                           bitmap_size,
                           timeout_ms);
}
