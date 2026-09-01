/**
 ******************************************************************************
 *@file               :   audio_player_service.h
 *@brief              :   Own the common audio output state and PCM DMA buffer.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef AUDIO_PLAYER_SERVICE_H
#define AUDIO_PLAYER_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "audio_pcm_block.h"
#include "bsp_audio_output.h"
#include "plat_error.h"

#define AUDIO_PLAYER_DMA_HALF_FRAMES    AUDIO_PCM_MAX_FRAMES_PER_BLOCK
#define AUDIO_PLAYER_DMA_HALF_SAMPLES   \
    (AUDIO_PLAYER_DMA_HALF_FRAMES * AUDIO_PCM_CHANNEL_COUNT)
#define AUDIO_PLAYER_DMA_SAMPLE_COUNT   (AUDIO_PLAYER_DMA_HALF_SAMPLES * 2U)

typedef enum AUDIO_PLAYER_SERVICE_STATE_T
{
    AUDIO_PLAYER_SERVICE_STATE_IDLE = 0U,
    AUDIO_PLAYER_SERVICE_STATE_PREPARING,
    AUDIO_PLAYER_SERVICE_STATE_PLAYING_CS4344,
    AUDIO_PLAYER_SERVICE_STATE_EMERGENCY,
    AUDIO_PLAYER_SERVICE_STATE_ERROR
} audio_player_service_state_t;

typedef struct AUDIO_PLAYER_SERVICE_T
{
    audio_player_service_state_t state;
    uint32_t session_id;
    uint32_t sample_rate_hz;
    uint32_t underrun_count;
    uint8_t  half_has_audio[2];
    uint16_t dma_samples[AUDIO_PLAYER_DMA_SAMPLE_COUNT];
} audio_player_service_t;

platform_err_t audio_player_service_init(
    audio_player_service_t  *p_service,
    bsp_audio_output_cb_t     output_callback);

platform_err_t audio_player_service_prepare(
    audio_player_service_t *p_service,
    uint32_t                session_id);

platform_err_t audio_player_service_start(
    audio_player_service_t   *p_service,
    const audio_pcm_block_t  *p_first_block,
    const audio_pcm_block_t  *p_second_block);

platform_err_t audio_player_service_unmute(
    audio_player_service_t *p_service);

platform_err_t audio_player_service_refill(
    audio_player_service_t  *p_service,
    bsp_audio_output_event_t output_event,
    const audio_pcm_block_t  *p_block,
    uint8_t                   count_underrun);

platform_err_t audio_player_service_stop(audio_player_service_t *p_service);

platform_err_t audio_player_service_emergency_set(
    audio_player_service_t *p_service,
    uint8_t                 is_active);

audio_player_service_state_t audio_player_service_state_get(
    const audio_player_service_t *p_service);

uint8_t audio_player_service_has_pending_audio(
    const audio_player_service_t *p_service);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PLAYER_SERVICE_H */
