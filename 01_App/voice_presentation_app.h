/**
 ******************************************************************************
 *@file               :   voice_presentation_app.h
 *@brief              :   Coordinate voice captions and LED indication.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef VOICE_PRESENTATION_APP_H
#define VOICE_PRESENTATION_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"

/* functions ----------------------------------------------------------------*/
platform_err_t voice_presentation_app_init(void);

platform_err_t voice_presentation_app_start(const uint8_t *p_gbk_text,
                                             uint16_t       text_size);

platform_err_t voice_presentation_app_audio_finished(void);

platform_err_t voice_presentation_app_process(void);

platform_err_t voice_presentation_app_abort(void);

uint8_t voice_presentation_app_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* VOICE_PRESENTATION_APP_H */
