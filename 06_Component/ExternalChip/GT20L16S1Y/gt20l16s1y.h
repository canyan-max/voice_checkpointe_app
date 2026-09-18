/**
 ******************************************************************************
 *@file               :   gt20l16s1y.h
 *@brief              :   Portable GT20L16S1Y font ROM driver.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef GT20L16S1Y_H
#define GT20L16S1Y_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define GT20L16S1Y_GLYPH_16X16_SIZE  32U
#define GT20L16S1Y_ASCII_8X16_SIZE   16U

typedef enum GT20L16S1Y_RET_T
{
    GT20L16S1Y_RET_OK = 0U,
    GT20L16S1Y_RET_PARAM,
    GT20L16S1Y_RET_IO,
    GT20L16S1Y_RET_BUSY,
    GT20L16S1Y_RET_TIMEOUT,
    GT20L16S1Y_RET_NOT_READY
} gt20l16s1y_ret_t;

typedef gt20l16s1y_ret_t (*gt20l16s1y_read_t)(
    void          *p_context,
    const uint8_t *p_command,
    uint16_t       command_size,
    uint8_t       *p_data,
    uint16_t       data_size,
    uint8_t        dummy_byte,
    uint32_t       timeout_ms);

typedef void (*gt20l16s1y_delay_ms_t)(void     *p_context,
                                      uint32_t  delay_ms);

typedef struct GT20L16S1Y_IO_T
{
    gt20l16s1y_read_t     read;
    gt20l16s1y_delay_ms_t delay_ms;
    void                 *p_context;
} gt20l16s1y_io_t;

typedef struct GT20L16S1Y_DEVICE_T
{
    gt20l16s1y_io_t io;
    uint8_t         is_initialized;
} gt20l16s1y_device_t;

gt20l16s1y_ret_t gt20l16s1y_init(gt20l16s1y_device_t   *p_device,
                                  const gt20l16s1y_io_t *p_io);

/**
 * @brief Read one 16 x 16 GB2312 glyph.
 * @note p_bitmap is valid only when GT20L16S1Y_RET_OK is returned. An I/O
 *       failure can leave partial received data in the caller-owned buffer.
 */
gt20l16s1y_ret_t gt20l16s1y_gb2312_16x16_read(
    gt20l16s1y_device_t *p_device,
    uint8_t              high_byte,
    uint8_t              low_byte,
    uint8_t             *p_bitmap,
    uint16_t             bitmap_size,
    uint32_t             timeout_ms);

gt20l16s1y_ret_t gt20l16s1y_ascii_8x16_read(
    gt20l16s1y_device_t *p_device,
    uint8_t              ascii_code,
    uint8_t             *p_bitmap,
    uint16_t             bitmap_size,
    uint32_t             timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* GT20L16S1Y_H */
