/**
 ******************************************************************************
 *@file               :   hub_display_service.c
 *@brief              :   Prepare and advance HUB12 GB2312 captions.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include <string.h>
#include "hub_display_service.h"

/* define -------------------------------------------------------------------*/
#define HUB_DISPLAY_STAGING_SLOT_COUNT           5U
#define HUB_DISPLAY_VISIBLE_SLOT_COUNT           4U
#define HUB_DISPLAY_GLYPH_HALF_BYTES            16U
#define HUB_DISPLAY_GLYPH_STEP_COUNT             16U
#define HUB_DISPLAY_SCROLL_STEP_FRAMES_NORMAL     5U
#define HUB_DISPLAY_ASCII_COLUMN_OFFSET            4U
/* private functions --------------------------------------------------------*/
static uint8_t hub_display_service_stream_source_get(
    const hub_display_service_t *p_service,
    uint16_t source_index)
{
    uint16_t glyph_byte;
    uint8_t staging_slot;
    uint8_t stream_position;
    uint8_t glyph_index;

    if((HUB_DISPLAY_STAGING_SLOT_COUNT *
        BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE) <=
            source_index)
    {
        return 0U;
    }

    staging_slot = (uint8_t)(source_index /
                             BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE);
    glyph_byte = source_index % BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE;
    stream_position = (uint8_t)(p_service->glyph_phase + staging_slot);

    if(HUB_DISPLAY_VISIBLE_SLOT_COUNT > stream_position)
    {
        return 0U;
    }

    glyph_index = stream_position - HUB_DISPLAY_VISIBLE_SLOT_COUNT;

    if(p_service->glyph_count <= glyph_index)
    {
        return 0U;
    }

    return p_service->glyph_data[glyph_index][glyph_byte];
}

static uint8_t hub_display_service_scroll_byte_get(
    const hub_display_service_t *p_service,
    uint8_t block,
    uint8_t column)
{
    uint16_t current_index;
    uint8_t current_byte;
    uint8_t next_byte;
    uint8_t shift;

    current_index = (uint16_t)column +
                    ((uint16_t)block * HUB_DISPLAY_GLYPH_HALF_BYTES);

    if(8U > p_service->scroll_pixel_step)
    {
        shift = p_service->scroll_pixel_step;
        current_byte = hub_display_service_stream_source_get(
                           p_service,
                           current_index);
        next_byte = hub_display_service_stream_source_get(
                        p_service,
                        current_index + HUB_DISPLAY_GLYPH_HALF_BYTES);
    }
    else
    {
        shift = p_service->scroll_pixel_step - 8U;
        current_byte = hub_display_service_stream_source_get(
                           p_service,
                           current_index + HUB_DISPLAY_GLYPH_HALF_BYTES);
        next_byte = hub_display_service_stream_source_get(
                        p_service,
                        current_index + (2U *
                                         HUB_DISPLAY_GLYPH_HALF_BYTES));
    }

    if(0U == shift)
    {
        return current_byte;
    }

    return (uint8_t)((current_byte >> shift) |
                     (uint8_t)(next_byte << (8U - shift)));
}

static void hub_display_service_scan_data_prepare(
    hub_display_service_t *p_service)
{
    uint8_t row;
    uint8_t source_block;
    uint8_t group;
    uint8_t output_index;

    for(row = 0U; row < BSP_HUB_DISPLAY_SCAN_ROW_COUNT; row++)
    {
        output_index = 0U;

        for(source_block = 0U;
                source_block < (2U * HUB_DISPLAY_VISIBLE_SLOT_COUNT);
                source_block++)
        {
            for(group = 0U; group < HUB_DISPLAY_VISIBLE_SLOT_COUNT; group++)
            {
                p_service->scan_data[row][output_index] =
                    hub_display_service_scroll_byte_get(
                        p_service,
                        (uint8_t)((2U * HUB_DISPLAY_VISIBLE_SLOT_COUNT - 1U) -
                                  source_block),
                        (uint8_t)(row +
                                  ((HUB_DISPLAY_VISIBLE_SLOT_COUNT - 1U -
                                    group) *
                                   BSP_HUB_DISPLAY_SCAN_ROW_COUNT)));
                output_index++;
            }
        }
    }
}

static void hub_display_service_animation_advance(
    hub_display_service_t *p_service)
{
    p_service->scroll_pixel_step++;

    if(HUB_DISPLAY_GLYPH_STEP_COUNT <= p_service->scroll_pixel_step)
    {
        p_service->scroll_pixel_step = 0U;
        p_service->glyph_phase++;

        if((p_service->glyph_count + HUB_DISPLAY_VISIBLE_SLOT_COUNT + 1U) <=
                p_service->glyph_phase)
        {
            p_service->glyph_phase = 0U;
            p_service->scroll_cycle_count++;
        }
    }

    hub_display_service_scan_data_prepare(p_service);
}

/* exported functions -------------------------------------------------------*/
platform_err_t hub_display_service_init(hub_display_service_t *p_service)
{
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }

    p_service->current_row = 0U;
    p_service->glyph_count = 0U;
    p_service->animation_frame_count = 0U;
    p_service->scroll_step_frames = HUB_DISPLAY_SCROLL_STEP_FRAMES_NORMAL;
    p_service->scroll_pixel_step = 0U;
    p_service->glyph_phase = HUB_DISPLAY_VISIBLE_SLOT_COUNT;
    p_service->scroll_cycle_count = 0U;
    p_service->scroll_enabled = 0U;
    p_service->active = 0U;
    p_service->initialized = 0U;
    hub_display_service_scan_data_prepare(p_service);

    ret = bsp_font_rom_init();
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_hub_display_init();
    }

    if(PLATFORM_ERR_OK == ret)
    {
        p_service->initialized = 1U;
    }

    return ret;
}

platform_err_t hub_display_service_gbk_text_set(
    hub_display_service_t *p_service,
    const uint8_t         *p_text,
    uint16_t               text_size,
    uint32_t               timeout_ms)
{
    platform_err_t ret;
    uint16_t text_index;
    uint16_t glyph_count;
    uint16_t glyph_index;
    uint8_t ascii_bitmap[BSP_FONT_ROM_ASCII_GLYPH_8X16_SIZE];
    uint8_t ascii_column;

    if((NULL == p_service) || (NULL == p_text) || (0U == text_size) ||
       (0U == timeout_ms) || (0U == p_service->initialized) ||
       (0U != p_service->active))
    {
        return PLATFORM_ERR_PARAM;
    }

    text_index = 0U;
    glyph_count = 0U;
    while(text_index < text_size)
    {
        if((p_text[text_index] >= 0x20U) &&
           (p_text[text_index] <= 0x7EU))
        {
            text_index++;
        }
        else
        {
            if((p_text[text_index] < 0xA1U) ||
               ((text_index + 1U) >= text_size))
            {
                return PLATFORM_ERR_PARAM;
            }
            text_index += 2U;
        }
        glyph_count++;
        if(glyph_count > HUB_DISPLAY_SERVICE_MAX_GLYPHS)
        {
            return PLATFORM_ERR_PARAM;
        }
    }

    p_service->glyph_count = 0U;
    text_index = 0U;
    glyph_index = 0U;
    while(text_index < text_size)
    {
        if((p_text[text_index] >= 0x20U) &&
           (p_text[text_index] <= 0x7EU))
        {
            ret = bsp_font_rom_ascii_8x16_read(
                      p_text[text_index],
                      ascii_bitmap,
                      (uint16_t)sizeof(ascii_bitmap),
                      timeout_ms);
            if(PLATFORM_ERR_OK == ret)
            {
                memset(p_service->glyph_data[glyph_index],
                       0,
                       BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE);
                for(ascii_column = 0U; ascii_column < 8U; ascii_column++)
                {
                    p_service->glyph_data[glyph_index]
                        [HUB_DISPLAY_ASCII_COLUMN_OFFSET + ascii_column] =
                            ascii_bitmap[ascii_column];
                    p_service->glyph_data[glyph_index]
                        [16U + HUB_DISPLAY_ASCII_COLUMN_OFFSET +
                         ascii_column] = ascii_bitmap[8U + ascii_column];
                }
            }
            text_index++;
        }
        else
        {
            ret = bsp_font_rom_gb2312_16x16_read(
                      p_text[text_index],
                      p_text[text_index + 1U],
                      p_service->glyph_data[glyph_index],
                      BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE,
                      timeout_ms);
            text_index += 2U;
        }
        if(PLATFORM_ERR_OK != ret)
        {
            return ret;
        }
        glyph_index++;
    }

    p_service->glyph_count = (uint8_t)glyph_count;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_service_start(hub_display_service_t *p_service,
                                         uint8_t scroll_enabled)
{
    platform_err_t ret;

    if((NULL == p_service) || (0U == p_service->initialized) ||
       (0U == p_service->glyph_count) ||
       (1U < scroll_enabled))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != p_service->active)
    {
        return PLATFORM_ERR_BUSY;
    }

    p_service->current_row = 0U;
    p_service->animation_frame_count = 0U;
    p_service->scroll_step_frames = HUB_DISPLAY_SCROLL_STEP_FRAMES_NORMAL;
    p_service->scroll_pixel_step = 0U;
    p_service->glyph_phase = (0U != scroll_enabled) ?
                             0U : HUB_DISPLAY_VISIBLE_SLOT_COUNT;
    p_service->scroll_cycle_count = 0U;
    p_service->scroll_enabled = scroll_enabled;
    hub_display_service_scan_data_prepare(p_service);

    ret = bsp_hub_display_start();
    if(PLATFORM_ERR_OK == ret)
    {
        p_service->active = 1U;
    }
    return ret;
}

platform_err_t hub_display_service_refresh(hub_display_service_t *p_service)
{
    platform_err_t ret;

    if((NULL == p_service) || (0U == p_service->initialized))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == p_service->active)
    {
        return PLATFORM_ERR_BUSY;
    }

    ret = bsp_hub_display_scan_row(
              p_service->scan_data[p_service->current_row],
              BSP_HUB_DISPLAY_SCAN_ROW_BYTES,
              p_service->current_row);

    if(PLATFORM_ERR_OK == ret)
    {
        p_service->current_row++;

        if(BSP_HUB_DISPLAY_SCAN_ROW_COUNT <= p_service->current_row)
        {
            p_service->current_row = 0U;
            if(0U != p_service->scroll_enabled)
            {
                p_service->animation_frame_count++;

                if(p_service->scroll_step_frames <=
                        p_service->animation_frame_count)
                {
                    p_service->animation_frame_count = 0U;
                    hub_display_service_animation_advance(p_service);
                }
            }
        }
    }

    return ret;
}

platform_err_t hub_display_service_scroll_step_frames_set(
    hub_display_service_t *p_service,
    uint8_t                step_frames)
{
    if((NULL == p_service) || (0U == step_frames) ||
       (0U == p_service->initialized))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == p_service->active)
    {
        return PLATFORM_ERR_BUSY;
    }

    p_service->scroll_step_frames = step_frames;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_service_scroll_cycle_count_get(
    const hub_display_service_t *p_service,
    uint8_t                     *p_cycle_count)
{
    if((NULL == p_service) || (NULL == p_cycle_count) ||
       (0U == p_service->initialized))
    {
        return PLATFORM_ERR_PARAM;
    }

    *p_cycle_count = p_service->scroll_cycle_count;
    return PLATFORM_ERR_OK;
}

platform_err_t hub_display_service_stop(hub_display_service_t *p_service)
{
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = bsp_hub_display_stop();
    p_service->current_row = 0U;
    p_service->animation_frame_count = 0U;
    p_service->scroll_step_frames = HUB_DISPLAY_SCROLL_STEP_FRAMES_NORMAL;
    p_service->scroll_pixel_step = 0U;
    p_service->glyph_phase = HUB_DISPLAY_VISIBLE_SLOT_COUNT;
    p_service->scroll_cycle_count = 0U;
    p_service->scroll_enabled = 0U;
    p_service->active = 0U;
    return ret;
}

/* end of file --------------------------------------------------------------*/
