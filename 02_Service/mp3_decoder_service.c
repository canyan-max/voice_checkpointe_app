/**
 ******************************************************************************
 *@file               :   mp3_decoder_service.c
 *@brief              :   Decode MP3 byte streams into stereo PCM blocks.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include <string.h>
#include "mp3_decoder_service.h"

#define MP3_DECODER_PROCESS_ATTEMPT_LIMIT 64U

static mp3_decoder_service_status_t mp3_decoder_service_input_refill(
    mp3_decoder_service_t *p_service)
{
    audio_data_status_t source_status;
    uint32_t            read_size = 0U;
    uint32_t            free_size;

    if(0U != p_service->source_eof)
    {
        return MP3_DECODER_SERVICE_STATUS_EOF;
    }

    free_size = MP3_DECODER_INPUT_BUFFER_SIZE - p_service->input_size;
    if(0U == free_size)
    {
        return MP3_DECODER_SERVICE_STATUS_OK;
    }

    source_status = p_service->source.p_ops->read(
        p_service->source.p_context,
        &p_service->input_buffer[p_service->input_size],
        free_size,
        &read_size);
    if(read_size > free_size)
    {
        return MP3_DECODER_SERVICE_STATUS_IO;
    }
    p_service->input_size += read_size;

    switch(source_status)
    {
        case AUDIO_DATA_STATUS_OK:
            return (0U != read_size) ? MP3_DECODER_SERVICE_STATUS_OK :
                                      MP3_DECODER_SERVICE_STATUS_AGAIN;
        case AUDIO_DATA_STATUS_EOF:
            p_service->source_eof = 1U;
            return MP3_DECODER_SERVICE_STATUS_EOF;
        case AUDIO_DATA_STATUS_AGAIN:
            return MP3_DECODER_SERVICE_STATUS_AGAIN;
        case AUDIO_DATA_STATUS_PARAM:
            return MP3_DECODER_SERVICE_STATUS_PARAM;
        case AUDIO_DATA_STATUS_IO:
        default:
            return MP3_DECODER_SERVICE_STATUS_IO;
    }
}

void mp3_decoder_service_init(mp3_decoder_service_t *p_service)
{
    if(NULL != p_service)
    {
        memset(p_service, 0, sizeof(*p_service));
    }
}

mp3_decoder_service_status_t mp3_decoder_service_start(
    mp3_decoder_service_t     *p_service,
    const audio_data_source_t *p_source,
    uint32_t                   session_id)
{
    audio_data_status_t source_status;

    if((NULL == p_service) || (NULL == p_source) ||
       (NULL == p_source->p_ops) ||
       (NULL == p_source->p_ops->open) ||
       (NULL == p_source->p_ops->read) ||
       (NULL == p_source->p_ops->close))
    {
        return MP3_DECODER_SERVICE_STATUS_PARAM;
    }
    if(0U != p_service->is_started)
    {
        return MP3_DECODER_SERVICE_STATUS_PARAM;
    }

    source_status = p_source->p_ops->open(p_source->p_context);
    if(AUDIO_DATA_STATUS_OK != source_status)
    {
        return (AUDIO_DATA_STATUS_PARAM == source_status) ?
               MP3_DECODER_SERVICE_STATUS_PARAM :
               MP3_DECODER_SERVICE_STATUS_IO;
    }

    p_service->source         = *p_source;
    p_service->session_id     = session_id;
    p_service->sample_rate_hz = 0U;
    p_service->input_size     = 0U;
    p_service->source_eof     = 0U;
    p_service->is_started     = 1U;
    mp3dec_init(&p_service->decoder);
    return MP3_DECODER_SERVICE_STATUS_OK;
}

mp3_decoder_service_status_t mp3_decoder_service_process(
    mp3_decoder_service_t *p_service,
    audio_pcm_block_t     *p_block)
{
    mp3dec_frame_info_t info;
    mp3_decoder_service_status_t refill_status;
    uint32_t attempt;
    uint32_t consumed_size;
    int      frame_count;
    int32_t  frame_index;

    if((NULL == p_service) || (NULL == p_block) ||
       (0U == p_service->is_started))
    {
        return MP3_DECODER_SERVICE_STATUS_PARAM;
    }

    for(attempt = 0U; attempt < MP3_DECODER_PROCESS_ATTEMPT_LIMIT; attempt++)
    {
        if((p_service->input_size < MP3_DECODER_INPUT_BUFFER_SIZE) &&
           (0U == p_service->source_eof))
        {
            refill_status = mp3_decoder_service_input_refill(p_service);
            if((MP3_DECODER_SERVICE_STATUS_IO == refill_status) ||
               (MP3_DECODER_SERVICE_STATUS_PARAM == refill_status))
            {
                return refill_status;
            }
            if((MP3_DECODER_SERVICE_STATUS_AGAIN == refill_status) &&
               (p_service->input_size < 4U))
            {
                return refill_status;
            }
        }

        if(0U == p_service->input_size)
        {
            return (0U != p_service->source_eof) ?
                   MP3_DECODER_SERVICE_STATUS_EOF :
                   MP3_DECODER_SERVICE_STATUS_AGAIN;
        }

        memset(&info, 0, sizeof(info));
        frame_count = mp3dec_decode_frame(
            &p_service->decoder,
            p_service->input_buffer,
            (int)p_service->input_size,
            p_block->samples,
            &info);
        if((info.frame_bytes < 0) ||
           ((uint32_t)info.frame_bytes > p_service->input_size))
        {
            return MP3_DECODER_SERVICE_STATUS_DECODE;
        }

        consumed_size = (uint32_t)info.frame_bytes;
        if(0U != consumed_size)
        {
            p_service->input_size -= consumed_size;
            if(0U != p_service->input_size)
            {
                memmove(p_service->input_buffer,
                        &p_service->input_buffer[consumed_size],
                        p_service->input_size);
            }
        }

        if(frame_count > 0)
        {
            if((frame_count > (int)AUDIO_PCM_MAX_FRAMES_PER_BLOCK) ||
               ((1 != info.channels) && (2 != info.channels)) ||
               (info.hz <= 0))
            {
                return MP3_DECODER_SERVICE_STATUS_FORMAT;
            }
            if((0U != p_service->sample_rate_hz) &&
               (p_service->sample_rate_hz != (uint32_t)info.hz))
            {
                return MP3_DECODER_SERVICE_STATUS_FORMAT;
            }
            p_service->sample_rate_hz = (uint32_t)info.hz;

            if(1 == info.channels)
            {
                for(frame_index = frame_count - 1;
                    frame_index >= 0;
                    frame_index--)
                {
                    p_block->samples[(uint32_t)frame_index * 2U] =
                        p_block->samples[frame_index];
                    p_block->samples[((uint32_t)frame_index * 2U) + 1U] =
                        p_block->samples[frame_index];
                }
            }

            p_block->session_id     = p_service->session_id;
            p_block->sample_rate_hz = p_service->sample_rate_hz;
            p_block->frame_count    = (uint16_t)frame_count;
            return MP3_DECODER_SERVICE_STATUS_PCM_READY;
        }

        if(0U == consumed_size)
        {
            if(0U != p_service->source_eof)
            {
                return MP3_DECODER_SERVICE_STATUS_EOF;
            }
            if(MP3_DECODER_INPUT_BUFFER_SIZE == p_service->input_size)
            {
                memmove(p_service->input_buffer,
                        &p_service->input_buffer[p_service->input_size - 3U],
                        3U);
                p_service->input_size = 3U;
            }
            else
            {
                return MP3_DECODER_SERVICE_STATUS_AGAIN;
            }
        }
    }

    return MP3_DECODER_SERVICE_STATUS_DECODE;
}

void mp3_decoder_service_stop(mp3_decoder_service_t *p_service)
{
    if((NULL != p_service) && (0U != p_service->is_started))
    {
        p_service->source.p_ops->close(p_service->source.p_context);
        p_service->is_started = 0U;
        p_service->input_size = 0U;
        p_service->source_eof = 0U;
    }
}
