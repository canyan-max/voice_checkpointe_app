/**
 ******************************************************************************
 *@file               :   audio_pcm_block.h
 *@brief              :   Define one decoded stereo PCM block.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef AUDIO_PCM_BLOCK_H
#define AUDIO_PCM_BLOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define AUDIO_PCM_CHANNEL_COUNT          2U
#define AUDIO_PCM_MAX_FRAMES_PER_BLOCK   1152U
#define AUDIO_PCM_MAX_SAMPLES_PER_BLOCK  \
    (AUDIO_PCM_MAX_FRAMES_PER_BLOCK * AUDIO_PCM_CHANNEL_COUNT)

typedef struct AUDIO_PCM_BLOCK_T
{
    uint32_t session_id;
    uint32_t sample_rate_hz;
    uint16_t frame_count;
    int16_t  samples[AUDIO_PCM_MAX_SAMPLES_PER_BLOCK];
} audio_pcm_block_t;

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PCM_BLOCK_H */
