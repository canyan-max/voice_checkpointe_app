/**
 ******************************************************************************
 *@file               :   audio_data_source.h
 *@brief              :   Describe a reusable read-only audio byte stream.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef AUDIO_DATA_SOURCE_H
#define AUDIO_DATA_SOURCE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum AUDIO_DATA_STATUS_T
{
    AUDIO_DATA_STATUS_OK = 0U,
    AUDIO_DATA_STATUS_EOF,
    AUDIO_DATA_STATUS_AGAIN,
    AUDIO_DATA_STATUS_IO,
    AUDIO_DATA_STATUS_PARAM
} audio_data_status_t;

typedef struct AUDIO_DATA_SOURCE_OPS_T
{
    audio_data_status_t (*open)(void *p_context);
    audio_data_status_t (*read)(void     *p_context,
                                uint8_t  *p_buffer,
                                uint32_t  buffer_size,
                                uint32_t *p_read_size);
    audio_data_status_t (*seek)(void *p_context, uint32_t offset);
    void (*close)(void *p_context);
} audio_data_source_ops_t;

typedef struct AUDIO_DATA_SOURCE_T
{
    const audio_data_source_ops_t *p_ops;
    void                          *p_context;
} audio_data_source_t;

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_DATA_SOURCE_H */
