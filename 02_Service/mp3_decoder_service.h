/**
 ******************************************************************************
 *@file               :   mp3_decoder_service.h
 *@brief              :   Decode MP3 byte streams into stereo PCM blocks.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef MP3_DECODER_SERVICE_H
#define MP3_DECODER_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "audio_data_source.h"
#include "audio_pcm_block.h"
#include "minimp3.h"

#define MP3_DECODER_INPUT_BUFFER_SIZE 8192U

typedef enum MP3_DECODER_SERVICE_STATUS_T
{
    MP3_DECODER_SERVICE_STATUS_OK = 0U,
    MP3_DECODER_SERVICE_STATUS_PCM_READY,
    MP3_DECODER_SERVICE_STATUS_EOF,
    MP3_DECODER_SERVICE_STATUS_AGAIN,
    MP3_DECODER_SERVICE_STATUS_PARAM,
    MP3_DECODER_SERVICE_STATUS_IO,
    MP3_DECODER_SERVICE_STATUS_FORMAT,
    MP3_DECODER_SERVICE_STATUS_DECODE
} mp3_decoder_service_status_t;

typedef struct MP3_DECODER_SERVICE_T
{
    mp3dec_t            decoder;
    audio_data_source_t source;
    uint32_t            session_id;
    uint32_t            sample_rate_hz;
    uint32_t            input_size;
    uint8_t             source_eof;
    uint8_t             is_started;
    uint8_t             input_buffer[MP3_DECODER_INPUT_BUFFER_SIZE];
} mp3_decoder_service_t;

void mp3_decoder_service_init(mp3_decoder_service_t *p_service);

mp3_decoder_service_status_t
mp3_decoder_service_start(mp3_decoder_service_t     *p_service,
                          const audio_data_source_t *p_source,
                          uint32_t                   session_id);

mp3_decoder_service_status_t
mp3_decoder_service_process(mp3_decoder_service_t *p_service,
                            audio_pcm_block_t     *p_block);

void mp3_decoder_service_stop(mp3_decoder_service_t *p_service);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DECODER_SERVICE_H */
