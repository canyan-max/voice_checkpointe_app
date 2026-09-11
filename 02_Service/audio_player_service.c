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

platform_err_t audio_player_service_init(
    audio_player_service_t *p_service,
    bsp_audio_output_cb_t    output_callback)
{
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }

    memset(p_service, 0, sizeof(*p_service));
    ret = bsp_audio_output_set_callback(output_callback);
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_audio_amplifier_init();
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_audio_amplifier_enable();
    }
    if(PLATFORM_ERR_OK == ret)
    {
        /* Diagnostic: keep TPA3116 unmuted from system initialization. */
        ret = bsp_audio_amplifier_mute_set(UNMUTES_AMP);
    }
    if(PLATFORM_ERR_OK == ret)
    {
        ret = bsp_voice_synthesis_init();
    }
    p_service->state = (PLATFORM_ERR_OK == ret) ?
                       AUDIO_PLAYER_SERVICE_STATE_IDLE :
                       AUDIO_PLAYER_SERVICE_STATE_ERROR;
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
    ret = bsp_audio_amplifier_mute_set(UNMUTES_AMP);
    if(PLATFORM_ERR_OK != ret)
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    return ret;
}

platform_err_t audio_player_service_voice_synthesis_start(
    audio_player_service_t         *p_service,
    bsp_voice_synthesis_encoding_t  encoding,
    const uint8_t                  *p_text,
    uint16_t                        text_size,
    uint32_t                        timeout_ms)
{
    platform_err_t ret;

    if((NULL == p_service) || (NULL == p_text) ||
       (0U == text_size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_IDLE != p_service->state)
    {
        return PLATFORM_ERR_BUSY;
    }

    ret = bsp_voice_synthesis_speak(encoding,
                                    p_text,
                                    text_size,
                                    timeout_ms);
    if(PLATFORM_ERR_OK == ret)
    {
        p_service->state =
            AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS;
    }
    return ret;
}

platform_err_t audio_player_service_voice_synthesis_process(
    audio_player_service_t       *p_service,
    bsp_voice_synthesis_event_t  *p_event)
{
    platform_err_t ret;

    if((NULL == p_service) || (NULL == p_event))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS !=
       p_service->state)
    {
        return PLATFORM_ERR_BUSY;
    }

    ret = bsp_voice_synthesis_process(p_event);
    if(PLATFORM_ERR_OK != ret)
    {
        /* Release the shared player after an RX fault so a later request can
         * retry instead of leaving the service permanently occupied. */
        (void)bsp_voice_synthesis_stop(
            AUDIO_PLAYER_SERVICE_VOICE_STOP_TIMEOUT_MS);
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
    }
    else if(0U != (*p_event &
                   BSP_VOICE_SYNTHESIS_EVENT_COMMAND_REJECTED))
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
    }
    else if(0U != (*p_event & BSP_VOICE_SYNTHESIS_EVENT_IDLE))
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
    }
    return ret;
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
    platform_err_t ret;

    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(AUDIO_PLAYER_SERVICE_STATE_IDLE == p_service->state)
    {
        return PLATFORM_ERR_OK;
    }

    if(AUDIO_PLAYER_SERVICE_STATE_PLAYING_VOICE_SYNTHESIS ==
       p_service->state)
    {
        ret = bsp_voice_synthesis_stop(
            AUDIO_PLAYER_SERVICE_VOICE_STOP_TIMEOUT_MS);
    }
    else
    {
        ret = bsp_audio_output_stream_stop();
    }
    p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
    p_service->half_has_audio[0] = 0U;
    p_service->half_has_audio[1] = 0U;
    return ret;
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
            ret = bsp_voice_synthesis_stop(
                AUDIO_PLAYER_SERVICE_VOICE_STOP_TIMEOUT_MS);
        }
        if(PLATFORM_ERR_OK == ret)
        {
            ret = bsp_audio_amplifier_mute_set(UNMUTES_AMP);
        }
        p_service->state = (PLATFORM_ERR_OK == ret) ?
                           AUDIO_PLAYER_SERVICE_STATE_EMERGENCY :
                           AUDIO_PLAYER_SERVICE_STATE_ERROR;
    }
    else if(AUDIO_PLAYER_SERVICE_STATE_EMERGENCY == p_service->state)
    {
        p_service->state = AUDIO_PLAYER_SERVICE_STATE_IDLE;
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
