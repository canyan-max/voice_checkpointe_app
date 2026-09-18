/**
 ******************************************************************************
 *@file               :   hub_display_service.h
 *@brief              :   Prepare and advance HUB12 GB2312 captions.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef HUB_DISPLAY_SERVICE_H
#define HUB_DISPLAY_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "bsp_font_rom.h"
#include "bsp_hub_display.h"
#include "plat_error.h"

/* define -------------------------------------------------------------------*/
#define HUB_DISPLAY_SERVICE_MAX_GLYPHS  64U

/* typedef ------------------------------------------------------------------*/
typedef struct HUB_DISPLAY_SERVICE_T
{
    uint8_t glyph_data[HUB_DISPLAY_SERVICE_MAX_GLYPHS]
                      [BSP_FONT_ROM_GB2312_GLYPH_16X16_SIZE];
    uint8_t scan_data[BSP_HUB_DISPLAY_SCAN_ROW_COUNT]
                     [BSP_HUB_DISPLAY_SCAN_ROW_BYTES];
    uint8_t glyph_count;
    uint8_t current_row;
    uint8_t animation_frame_count;
    uint8_t scroll_step_frames;
    uint8_t scroll_pixel_step;
    uint8_t glyph_phase;
    uint8_t scroll_cycle_count;
    uint8_t scroll_enabled;
    uint8_t active;
    uint8_t initialized;
} hub_display_service_t;

/* functions ----------------------------------------------------------------*/
platform_err_t hub_display_service_init(hub_display_service_t *p_service);

platform_err_t hub_display_service_gbk_text_set(
    hub_display_service_t *p_service,
    const uint8_t         *p_text,
    uint16_t               text_size,
    uint32_t               timeout_ms);

platform_err_t hub_display_service_start(hub_display_service_t *p_service,
                                         uint8_t scroll_enabled);

platform_err_t hub_display_service_refresh(hub_display_service_t *p_service);

platform_err_t hub_display_service_scroll_step_frames_set(
    hub_display_service_t *p_service,
    uint8_t                step_frames);

platform_err_t hub_display_service_scroll_cycle_count_get(
    const hub_display_service_t *p_service,
    uint8_t                     *p_cycle_count);

platform_err_t hub_display_service_stop(hub_display_service_t *p_service);

#ifdef __cplusplus
}
#endif

#endif /* HUB_DISPLAY_SERVICE_H */
