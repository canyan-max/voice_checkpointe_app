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
#define AUDIO_PLAYBACK_APP_CAPTION_MAX_BYTES     (200U)

typedef struct AUDIO_PLAYBACK_VOICE_REQUEST_T {
    bsp_voice_synthesis_encoding_t encoding;
    const uint8_t                 *p_text;
    uint16_t                       text_size;
    uint8_t                        repeat_count;
    const uint8_t                 *p_caption_gbk;
    uint16_t                       caption_size;
} audio_playback_voice_request_t;

platform_err_t audio_playback_app_init(void);
platform_err_t audio_playback_app_play(const audio_data_source_t *p_source);
/* Built-in SD source: BUSY until card presence is confirmed, HW when absent.
 * OK means request enqueued; presence is checked again before startup. */
platform_err_t audio_playback_app_play_default(void);
/* Copies voice and caption text before returning. Zero caption size uses the
 * voice text as caption. 0 or 1 plays once; 2 to 16 repeats as
 * requested. OK means enqueued; the player task can still reject a busy or
 * failed start. */
platform_err_t audio_playback_app_voice_request(
    const audio_playback_voice_request_t *p_request);
platform_err_t
audio_playback_app_voice_speak(bsp_voice_synthesis_encoding_t encoding,
                               const uint8_t                 *p_text,
                               uint16_t                       text_size);
platform_err_t audio_playback_app_stop(void);
platform_err_t audio_playback_app_emergency_set(uint8_t is_active);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PLAYBACK_APP_H */
