/**
 ******************************************************************************
 *@file               :   audio_player_service.c
 *@brief              :   Own the common audio output state and PCM DMA buffer.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include <string.h>
#include "audio_player_service.h"
#include "bsp_audio_amplifier.h"

#define AUDIO_PLAYER_SERVICE_VOICE_STOP_TIMEOUT_MS  100U
#define AUDIO_PLAYER_VOICE_ROUND_GAP_MS             (35U)
#define AUDIO_PLAYER_VOICE_ROUND_TIMEOUT_MS         (180000U)

static platform_err_t audio_player_service_half_write(
    audio_player_service_t  *p_service,
    uint32_t                 half_index,
    const audio_pcm_block_t *p_block,
    uint8_t                  count_underrun)
{
    uint16_t *p_dest;
    uint32_t  copy_size;

    if((half_index > 1U) || (NULL == p_service))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_dest = &p_service->dma_samples[half_index *
                                     AUDIO_PLAYER_DMA_HALF_SAMPLES];
    memset(p_dest, 0, AUDIO_PLAYER_DMA_HALF_SAMPLES * sizeof(uint16_t));
    p_service->half_has_audio[half_index] = 0U;

    if(NULL == p_block)
    {
        if(0U != count_underrun)
        {
            p_service->underrun_count++;
        }
        return PLATFORM_ERR_OK;
    }
    if((p_block->session_id != p_service->session_id) ||
       (p_block->sample_rate_hz != p_service->sample_rate_hz) ||
       (p_block->frame_count > AUDIO_PLAYER_DMA_HALF_FRAMES))
    {
        return PLATFORM_ERR_PARAM;
    }

    copy_size = (uint32_t)p_block->frame_count *
                AUDIO_PCM_CHANNEL_COUNT * sizeof(int16_t);
    memcpy(p_dest, p_block->samples, copy_size);
    p_service->half_has_audio[half_index] =
        (0U != p_block->frame_count) ? 1U : 0U;
    return PLATFORM_ERR_OK;
}

platform_err_t audio_player_service_init(audio_player_service_t              *p_service,
                                         bsp_audio_output_cb_t                output_callback,
                                         const bsp_voice_synthesis_tx_sync_t *p_voice_tx_sync) {
    platform_err_t ret;

    if ((NULL == p_service) || (NULL == p_voice_tx_sync)) {
        return PLATFORM_ERR_PARAM;
    }

    memset(p_service, 0, sizeof(*p_service));
    ret = bsp_audio_output_set_callback(output_callback);
    if (PLATFORM_ERR_OK == ret) {
        ret = bsp_audio_amplifier_init();
    }
    if (PLATFORM_ERR_OK == ret) {
        ret = bsp_audio_amplifier_enable();
    }
    if (PLATFORM_ERR_OK == ret) {
        ret = bsp_voice_synthesis_init(p_voice_tx_sync);
    }
    p_service->state = (PLATFORM_ERR_OK == ret) ? AUDIO_PLAYER_SERVICE_STATE_IDLE
                                                : AUDIO_PLAYER_SERVICE_STATE_ERROR;
    return ret;
}

platform_err_t audio_player_service_prepare(
    audio_player_service_t *p_service,
    uint32_t                session_id)
{
    if((NULL == p_service) || (0U == session_id))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_IDLE != p_service->state)
    {
        return PLATFORM_ERR_BUSY;
    }

    p_service->session_id      = session_id;
    p_service->sample_rate_hz  = 0U;
    p_service->underrun_count  = 0U;
    p_service->half_has_audio[0] = 0U;
    p_service->half_has_audio[1] = 0U;
    memset(p_service->dma_samples, 0, sizeof(p_service->dma_samples));
    p_service->state = AUDIO_PLAYER_SERVICE_STATE_PREPARING;
    return PLATFORM_ERR_OK;
}

platform_err_t audio_player_service_start(
    audio_player_service_t  *p_service,
    const audio_pcm_block_t *p_first_block,
    const audio_pcm_block_t *p_second_block)
{
    platform_err_t ret;

    if((NULL == p_service) || (NULL == p_first_block) ||
       (NULL == p_second_block))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_PREPARING != p_service->state)
    {
        return PLATFORM_ERR_BUSY;
    }
    if((p_first_block->session_id != p_service->session_id) ||
       (p_second_block->session_id != p_service->session_id) ||
       (0U == p_first_block->sample_rate_hz) ||
       (p_first_block->sample_rate_hz != p_second_block->sample_rate_hz))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_service->sample_rate_hz = p_first_block->sample_rate_hz;
    ret = audio_player_service_half_write(p_service,
                                          0U,
                                          p_first_block,
                                          0U);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = audio_player_service_half_write(p_service,
                                              1U,
                                              p_second_block,
                                              0U);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_audio_output_stream_start(
            p_service->sample_rate_hz,
            p_service->dma_samples,
            (uint16_t)AUDIO_PLAYER_DMA_SAMPLE_COUNT);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_PLAYING_CS4344;
    }
    else
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    return ret;
}

platform_err_t audio_player_service_unmute(
    audio_player_service_t *p_service)
{
    platform_err_t ret;

    if((NULL == p_service) ||
       (AUDIO_PLAYER_SERVICE_STATE_PREPARING != p_service->state))
    {
        return PLATFORM_ERR_PARAM;
    }
    ret = bsp_audio_amplifier_mute_set(BSP_AUDIO_AMPLIFIER_UNMUTED);
    if(PLATFORM_ERR_OK != ret)
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    return ret;
}

static platform_err_t
audio_player_service_voice_round_start(audio_player_voice_session_t *p_voice,
                                       uint32_t                      now_ms) {
    platform_err_t ret = bsp_voice_synthesis_speak(p_voice->encoding,
                                                   p_voice->p_text,
                                                   p_voice->text_size,
                                                   p_voice->send_timeout_ms);
    if (PLATFORM_ERR_OK == ret) {
        p_voice->phase_started_ms = now_ms;
        p_voice->waiting_next     = 0U;
        p_voice->round_confirmed  = 0U;
    }
    return ret;
}

static platform_err_t
audio_player_service_voice_cancel(audio_player_service_t *p_service) {
    uint8_t waiting_next = p_service->voice.waiting_next;
    p_service->voice     = (audio_player_voice_session_t){0};
    if (0U != waiting_next) {
        /* The previous round is already idle. */
        return PLATFORM_ERR_OK;
    }
    return bsp_voice_synthesis_stop(AUDIO_PLAYER_SERVICE_VOICE_STOP_TIMEOUT_MS);
}

static platform_err_t
audio_player_service_voice_finish(audio_player_service_t *p_service) {
    p_service->voice   = (audio_player_voice_session_t){0};
    platform_err_t ret = bsp_audio_amplifier_mute_set(
        BSP_AUDIO_AMPLIFIER_MUTED);
    p_service->state = (PLATFORM_ERR_OK == ret)
                           ? AUDIO_PLAYER_SERVICE_STATE_IDLE
                           : AUDIO_PLAYER_SERVICE_STATE_ERROR;
    return ret;
}

platform_err_t audio_player_service_voice_ready_poll(audio_player_service_t *p_service,
                                                     uint32_t                probe_timeout_ms) {
    if (NULL == p_service) {
        return PLATFORM_ERR_PARAM;
    }
    if (AUDIO_PLAYER_SERVICE_STATE_IDLE != p_service->state) {
        return PLATFORM_ERR_BUSY;
    }
    if (0U != p_service->voice_ready) {
        return PLATFORM_ERR_OK;
    }

    bsp_voice_synthesis_event_t event = BSP_VOICE_SYNTHESIS_EVENT_NONE;
    platform_err_t              ret   = bsp_voice_synthesis_poll(&event);
    if (PLATFORM_ERR_OK != ret) {
        return ret;
    }
    if (((0U == p_service->voice_probe_sent) &&
         (0U != (event & BSP_VOICE_SYNTHESIS_EVENT_INITIALIZED))) ||
        ((0U != p_service->voice_probe_sent) && (0U != (event & BSP_VOICE_SYNTHESIS_EVENT_IDLE)) &&
         (0U == (event & BSP_VOICE_SYNTHESIS_EVENT_SPEAKING)))) {
        p_service->voice_ready      = 1U;
        p_service->voice_probe_sent = 0U;
        return PLATFORM_ERR_OK;
    }
    if (0U != probe_timeout_ms) {
        ret = bsp_voice_synthesis_status_request(probe_timeout_ms);
        if (PLATFORM_ERR_OK != ret) {
            return ret;
        }
        p_service->voice_probe_sent = 1U;
    }
    return PLATFORM_ERR_BUSY;
}

platform_err_t audio_player_service_voice_synthesis_start(
    audio_player_service_t        *p_service,
    bsp_voice_synthesis_encoding_t encoding,
    const uint8_t                 *p_text,
    uint16_t                       text_size,
    uint8_t                        repeat_count,
    uint32_t                       now_ms,
    uint32_t                       timeout_ms) {
    platform_err_t ret;

    if ((NULL == p_service) || (NULL == p_text) || (0U == text_size) ||
        (0U == timeout_ms) || (repeat_count > AUDIO_PLAYER_VOICE_REPEAT_MAX)) {
        return PLATFORM_ERR_PARAM;
    }
    if (AUDIO_PLAYER_SERVICE_STATE_IDLE != p_service->state) {
        return PLATFORM_ERR_BUSY;
    }

    if (0U == p_service->voice_ready) {
        return PLATFORM_ERR_BUSY;
    }

    /* Discard buffered replies from a previous session before sending. */
    bsp_voice_synthesis_event_t stale = BSP_VOICE_SYNTHESIS_EVENT_NONE;
    ret                               = bsp_voice_synthesis_poll(&stale);
    if (PLATFORM_ERR_OK != ret) {
        p_service->voice_ready = 0U;
        return ret;
    }
    ret = bsp_audio_amplifier_mute_set(BSP_AUDIO_AMPLIFIER_UNMUTED);
    if (PLATFORM_ERR_OK != ret) {
        return ret;
    }

    p_service->voice = (audio_player_voice_session_t){
        .p_text          = p_text,
        .encoding        = encoding,
        .send_timeout_ms = timeout_ms,
        .text_size       = text_size,
        .remaining       = (0U == repeat_count) ? 1U : repeat_count,
    };
    ret = audio_player_service_voice_round_start(&p_service->voice, now_ms);
    if (PLATFORM_ERR_OK != ret) {
        (void)audio_player_service_voice_finish(p_service);
        return ret;
    }
    p_service->state = AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS;
    return PLATFORM_ERR_OK;
}

platform_err_t audio_player_service_voice_synthesis_process(
    audio_player_service_t      *p_service,
    uint32_t                     now_ms,
    bsp_voice_synthesis_event_t *p_event) {
    platform_err_t ret;

    if ((NULL == p_service) || (NULL == p_event)) {
        return PLATFORM_ERR_PARAM;
    }
    if (AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS !=
        p_service->state) {
        return PLATFORM_ERR_BUSY;
    }

    audio_player_voice_session_t *p_voice = &p_service->voice;
    uint32_t elapsed_ms = (uint32_t)(now_ms - p_voice->phase_started_ms);
    ret                 = bsp_voice_synthesis_poll(p_event);
    if (PLATFORM_ERR_OK != ret) {
        p_service->voice_ready = 0U;
        /* A failed poll may have collected only part of the event history. */
        *p_event = BSP_VOICE_SYNTHESIS_EVENT_NONE;
        (void)audio_player_service_voice_cancel(p_service);
        (void)audio_player_service_voice_finish(p_service);
        return ret;
    }

    if (0U != (*p_event & BSP_VOICE_SYNTHESIS_EVENT_INITIALIZED)) {
        p_service->voice_ready = 0U;
        (void)audio_player_service_voice_finish(p_service);
        return PLATFORM_ERR_HW;
    }

    if (0U != p_voice->waiting_next) {
        *p_event = BSP_VOICE_SYNTHESIS_EVENT_NONE;
        if (elapsed_ms < AUDIO_PLAYER_VOICE_ROUND_GAP_MS) {
            return PLATFORM_ERR_OK;
        }
        ret = audio_player_service_voice_round_start(p_voice, now_ms);
        if (PLATFORM_ERR_OK != ret) {
            (void)audio_player_service_voice_finish(p_service);
        }
        return ret;
    }

    if (0U != (*p_event & BSP_VOICE_SYNTHESIS_EVENT_COMMAND_REJECTED)) {
        return audio_player_service_voice_finish(p_service);
    }
    if (0U != (*p_event & (BSP_VOICE_SYNTHESIS_EVENT_COMMAND_ACCEPTED |
                           BSP_VOICE_SYNTHESIS_EVENT_SPEAKING))) {
        p_voice->round_confirmed = 1U;
    }
    if (0U != (*p_event & BSP_VOICE_SYNTHESIS_EVENT_IDLE)) {
        if (0U != p_voice->round_confirmed) {
            p_voice->remaining--;
            if (0U == p_voice->remaining) {
                return audio_player_service_voice_finish(p_service);
            }
            p_voice->phase_started_ms = now_ms;
            p_voice->waiting_next     = 1U;
        }
        *p_event =
            (bsp_voice_synthesis_event_t)((uint32_t)*p_event &
                                          ~(uint32_t)
                                              BSP_VOICE_SYNTHESIS_EVENT_IDLE);
    }
    if ((0U == p_voice->waiting_next) &&
        (elapsed_ms >= AUDIO_PLAYER_VOICE_ROUND_TIMEOUT_MS)) {
        (void)audio_player_service_voice_cancel(p_service);
        (void)audio_player_service_voice_finish(p_service);
        return PLATFORM_ERR_TIMEOUT;
    }
    return PLATFORM_ERR_OK;
}

platform_err_t audio_player_service_refill(
    audio_player_service_t  *p_service,
    bsp_audio_output_event_t output_event,
    const audio_pcm_block_t *p_block,
    uint8_t                  count_underrun)
{
    uint32_t half_index;

    if((NULL == p_service) ||
       (AUDIO_PLAYER_SERVICE_STATE_PLAYING_CS4344 != p_service->state))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(BSP_AUDIO_OUTPUT_EVENT_FIRST_HALF_WRITABLE == output_event)
    {
        half_index = 0U;
    }
    else if(BSP_AUDIO_OUTPUT_EVENT_SECOND_HALF_WRITABLE == output_event)
    {
        half_index = 1U;
    }
    else
    {
        return PLATFORM_ERR_PARAM;
    }
    return audio_player_service_half_write(p_service,
                                           half_index,
                                           p_block,
                                           count_underrun);
}

platform_err_t audio_player_service_stop(audio_player_service_t *p_service)
{
    platform_err_t mute_ret;
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_IDLE == p_service->state)
    {
        return PLATFORM_ERR_OK;
    }

    mute_ret = bsp_audio_amplifier_mute_set(BSP_AUDIO_AMPLIFIER_MUTED);
    if(AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS ==
       p_service->state)
    {
        ret = audio_player_service_voice_cancel(p_service);
    }
    else
    {
        ret = bsp_audio_output_stream_stop();
    }
    p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
    p_service->half_has_audio[0] = 0U;
    p_service->half_has_audio[1] = 0U;
    return (PLATFORM_ERR_OK != mute_ret) ? mute_ret : ret;
}

platform_err_t audio_player_service_emergency_set(
    audio_player_service_t *p_service,
    uint8_t                 is_active)
{
    platform_err_t ret = PLATFORM_ERR_OK;

    if((NULL == p_service) || (is_active > 1U))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U != is_active)
    {
        if((AUDIO_PLAYER_SERVICE_STATE_PREPARING == p_service->state) ||
           (AUDIO_PLAYER_SERVICE_STATE_PLAYING_CS4344 == p_service->state))
        {
            ret = bsp_audio_output_stream_stop();
        }
        else if(AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS ==
                p_service->state)
        {
            ret = audio_player_service_voice_cancel(p_service);
        }
        if(PLATFORM_ERR_OK == ret)
        {
            ret = bsp_audio_amplifier_mute_set(
                BSP_AUDIO_AMPLIFIER_UNMUTED);
        }
        p_service->state = (PLATFORM_ERR_OK == ret) ?
                           AUDIO_PLAYER_SERVICE_STATE_EMERGENCY :
                           AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    else if(AUDIO_PLAYER_SERVICE_STATE_EMERGENCY == p_service->state)
    {
        ret = bsp_audio_amplifier_mute_set(BSP_AUDIO_AMPLIFIER_MUTED);
        p_service->state = (PLATFORM_ERR_OK == ret) ?
                           AUDIO_PLAYER_SERVICE_STATE_IDLE :
                           AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    return ret;
}

audio_player_service_state_t audio_player_service_state_get(
    const audio_player_service_t *p_service)
{
    return (NULL != p_service) ? p_service->state :
           AUDIO_PLAYER_SERVICE_STATE_ERROR;
}

uint8_t audio_player_service_has_pending_audio(
    const audio_player_service_t *p_service)
{
    if(NULL == p_service)
    {
        return 0U;
    }
    return (uint8_t)((0U != p_service->half_has_audio[0]) ||
                     (0U != p_service->half_has_audio[1]));
}
