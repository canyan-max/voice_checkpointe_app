/**
 ******************************************************************************
 *@file               :   audio_playback_app.h
 *@brief              :   Connect the audio services to FreeRTOS tasks.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef AUDIO_PLAYBACK_APP_H
#define AUDIO_PLAYBACK_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "audio_data_source.h"
#include "bsp_voice_synthesis.h"
#include "plat_error.h"

#define AUDIO_PLAYBACK_APP_VOICE_TEXT_MAX_BYTES  512U

platform_err_t audio_playback_app_init(void);
platform_err_t audio_playback_app_play(
    const audio_data_source_t *p_source);
platform_err_t audio_playback_app_play_default(void);
platform_err_t audio_playback_app_voice_speak(
    bsp_voice_synthesis_encoding_t encoding,
    const uint8_t                 *p_text,
    uint16_t                       text_size);
platform_err_t audio_playback_app_stop(void);
platform_err_t audio_playback_app_emergency_set(uint8_t is_active);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PLAYBACK_APP_H */
