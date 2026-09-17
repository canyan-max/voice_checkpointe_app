/**
 ******************************************************************************
 *@file               :   hub_display_service.c
 *@brief              :   Prepare and advance the vendor HUB12 test pattern.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "hub_display_service.h"

/* define -------------------------------------------------------------------*/
#define HUB_DISPLAY_GLYPH_BYTES                 32U
#define HUB_DISPLAY_GLYPH_COUNT                  4U
#define HUB_DISPLAY_VENDOR_SLOT_COUNT            5U
#define HUB_DISPLAY_VISIBLE_SLOT_COUNT           4U
#define HUB_DISPLAY_VENDOR_BLOCK_BYTES          16U
#define HUB_DISPLAY_VENDOR_STEP_COUNT            16U
#define HUB_DISPLAY_SCROLL_STEP_FRAMES            5U
#define HUB_DISPLAY_GLYPH_PHASE_COUNT \
    (HUB_DISPLAY_GLYPH_COUNT + HUB_DISPLAY_VISIBLE_SLOT_COUNT + 1U)

/* variables ----------------------------------------------------------------*/
static const uint8_t hub_display_vendor_glyph
[HUB_DISPLAY_GLYPH_COUNT][HUB_DISPLAY_GLYPH_BYTES] =
{
    {
        0x10U, 0x60U, 0x02U, 0x8CU, 0x00U, 0x08U, 0x08U, 0x08U,
        0x09U, 0xFAU, 0x08U, 0x08U, 0x08U, 0x08U, 0x00U, 0x00U,
        0x04U, 0x04U, 0x7EU, 0x01U, 0x40U, 0x40U, 0x41U, 0x41U,
        0x41U, 0x7FU, 0x41U, 0x41U, 0x41U, 0x41U, 0x40U, 0x00U
    },
    {
        0x10U, 0x10U, 0x12U, 0xD2U, 0x56U, 0x5AU, 0x52U, 0x53U,
        0x52U, 0x5AU, 0x56U, 0xD2U, 0x12U, 0x10U, 0x10U, 0x00U,
        0x40U, 0x30U, 0x00U, 0x77U, 0x85U, 0x85U, 0x8DU, 0xB5U,
        0x85U, 0x85U, 0x85U, 0xE7U, 0x00U, 0x10U, 0x60U, 0x00U
    },
    {
        0x80U, 0x90U, 0x8CU, 0x84U, 0x84U, 0x84U, 0xF5U, 0x86U,
        0x84U, 0x84U, 0x84U, 0x84U, 0x94U, 0x8CU, 0x80U, 0x00U,
        0x00U, 0x80U, 0x80U, 0x84U, 0x46U, 0x49U, 0x28U, 0x10U,
        0x10U, 0x2CU, 0x23U, 0x40U, 0x80U, 0x00U, 0x00U, 0x00U
    },
    {
        0x80U, 0x80U, 0x40U, 0x20U, 0x50U, 0x48U, 0x44U, 0xC3U,
        0x44U, 0x48U, 0x50U, 0x20U, 0x40U, 0x80U, 0x80U, 0x00U,
        0x00U, 0x40U, 0x40U, 0x44U, 0x44U, 0x44U, 0x44U, 0x7FU,
        0x44U, 0x44U, 0x44U, 0x44U, 0x40U, 0x40U, 0x00U, 0x00U
    }
};

/* private functions --------------------------------------------------------*/
static uint8_t hub_display_service_vendor_source_get(
    const hub_display_service_t *p_service,
    uint16_t source_index)
{
    uint16_t glyph_byte;
    uint8_t staging_slot;
    uint8_t stream_position;
    uint8_t glyph_index;

    if((HUB_DISPLAY_VENDOR_SLOT_COUNT * HUB_DISPLAY_GLYPH_BYTES) <=
            source_index)
    {
        return 0U;
    }

    staging_slot = (uint8_t)(source_index / HUB_DISPLAY_GLYPH_BYTES);
    glyph_byte = source_index % HUB_DISPLAY_GLYPH_BYTES;
    stream_position = (uint8_t)(p_service->glyph_phase + staging_slot);

    if(HUB_DISPLAY_VISIBLE_SLOT_COUNT > stream_position)
    {
        return 0U;
    }

    glyph_index = stream_position - HUB_DISPLAY_VISIBLE_SLOT_COUNT;

    if(HUB_DISPLAY_GLYPH_COUNT <= glyph_index)
    {
        return 0U;
    }

    return hub_display_vendor_glyph[glyph_index][glyph_byte];
}

static uint8_t hub_display_service_vendor_byte_get(
    const hub_display_service_t *p_service,
    uint8_t block,
    uint8_t column)
{
    uint16_t current_index;
    uint8_t current_byte;
    uint8_t next_byte;
    uint8_t shift;

    current_index = (uint16_t)column +
                    ((uint16_t)block * HUB_DISPLAY_VENDOR_BLOCK_BYTES);

    if(8U > p_service->vendor_step)
    {
        shift = p_service->vendor_step;
        current_byte = hub_display_service_vendor_source_get(
                           p_service,
                           current_index);
        next_byte = hub_display_service_vendor_source_get(
                        p_service,
                        current_index + HUB_DISPLAY_VENDOR_BLOCK_BYTES);
    }
    else
    {
        shift = p_service->vendor_step - 8U;
        current_byte = hub_display_service_vendor_source_get(
                           p_service,
                           current_index + HUB_DISPLAY_VENDOR_BLOCK_BYTES);
        next_byte = hub_display_service_vendor_source_get(
                        p_service,
                        current_index + (2U * HUB_DISPLAY_VENDOR_BLOCK_BYTES));
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
                    hub_display_service_vendor_byte_get(
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
    p_service->vendor_step++;

    if(HUB_DISPLAY_VENDOR_STEP_COUNT <= p_service->vendor_step)
    {
        p_service->vendor_step = 0U;
        p_service->glyph_phase++;

        if(HUB_DISPLAY_GLYPH_PHASE_COUNT <= p_service->glyph_phase)
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
    p_service->animation_frame_count = 0U;
    p_service->vendor_step = 0U;
    p_service->glyph_phase = HUB_DISPLAY_VISIBLE_SLOT_COUNT;
    p_service->scroll_cycle_count = 0U;
    p_service->scroll_enabled = 0U;
    p_service->active = 0U;
    p_service->initialized = 0U;
    hub_display_service_scan_data_prepare(p_service);

    ret = bsp_hub_display_init();

    if(PLATFORM_ERR_OK == ret)
    {
        p_service->initialized = 1U;
    }

    return ret;
}

platform_err_t hub_display_service_start(hub_display_service_t *p_service,
                                         uint8_t scroll_enabled)
{
    platform_err_t ret;

    if((NULL == p_service) || (0U == p_service->initialized) ||
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
    p_service->vendor_step = 0U;
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

                if(HUB_DISPLAY_SCROLL_STEP_FRAMES <=
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
    p_service->vendor_step = 0U;
    p_service->glyph_phase = HUB_DISPLAY_VISIBLE_SLOT_COUNT;
    p_service->scroll_cycle_count = 0U;
    p_service->scroll_enabled = 0U;
    p_service->active = 0U;
    return ret;
}

/* end of file --------------------------------------------------------------*/
