/**
 ******************************************************************************
 *@file               :   audio_playback_app.c
 *@brief              :   Connect the audio services to FreeRTOS tasks.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "ff.h"
#include "audio_playback_app.h"
#include "audio_pcm_block.h"
#include "audio_player_service.h"
#include "mp3_decoder_service.h"
#include "plat_log.h"

#define AUDIO_APP_PCM_BLOCK_COUNT             (4U)
#define AUDIO_APP_START_PREBUFFER_BLOCKS      (AUDIO_APP_PCM_BLOCK_COUNT)
#define AUDIO_APP_PLAYER_COMMAND_COUNT        (4U)
#define AUDIO_APP_DECODE_COMMAND_COUNT        (1U)
#define AUDIO_APP_PLAYER_STACK_WORDS          (512U)
#define AUDIO_APP_DECODER_STACK_WORDS         (6144U)
#define AUDIO_APP_PLAYER_PRIORITY             (6U)
#define AUDIO_APP_DECODER_PRIORITY            (5U)
#define AUDIO_APP_DECODER_WAIT_MS             (20U)
#define AUDIO_APP_OUTPUT_STABILIZE_MS         (20U)

#define AUDIO_APP_PLAYER_NOTIFY_COMMAND       (1UL << 0)
#define AUDIO_APP_PLAYER_NOTIFY_PCM_READY     (1UL << 1)
#define AUDIO_APP_PLAYER_NOTIFY_DECODE_EOF    (1UL << 2)
#define AUDIO_APP_PLAYER_NOTIFY_DECODE_ERROR  (1UL << 3)
#define AUDIO_APP_PLAYER_NOTIFY_DMA_HALF      (1UL << 4)
#define AUDIO_APP_PLAYER_NOTIFY_DMA_FULL      (1UL << 5)
#define AUDIO_APP_PLAYER_NOTIFY_DMA_ERROR     (1UL << 6)

#define AUDIO_APP_DECODER_NOTIFY_STOP         (1UL << 0)

typedef enum AUDIO_APP_PLAYER_COMMAND_TYPE_T
{
    AUDIO_APP_PLAYER_COMMAND_PLAY = 0U,
    AUDIO_APP_PLAYER_COMMAND_STOP,
    AUDIO_APP_PLAYER_COMMAND_EMERGENCY
} audio_app_player_command_type_t;

typedef struct AUDIO_APP_PLAYER_COMMAND_T
{
    audio_app_player_command_type_t type;
    audio_data_source_t             source;
    uint8_t                         emergency_active;
} audio_app_player_command_t;

typedef struct AUDIO_APP_DECODE_COMMAND_T
{
    audio_data_source_t source;
    uint32_t            session_id;
} audio_app_decode_command_t;

typedef struct AUDIO_APP_FATFS_SOURCE_T
{
    FATFS        filesystem;
    FIL          file;
    const TCHAR *p_path;
    uint8_t      is_mounted;
    uint8_t      is_open;
} audio_app_fatfs_source_t;

static const TCHAR audio_app_default_mp3_path[] =
{
    (TCHAR)'0', (TCHAR)':', (TCHAR)'/',
    (TCHAR)0x597DU, (TCHAR)0x597DU,
    (TCHAR)0x5B66U, (TCHAR)0x4E60U,
    (TCHAR)'.', (TCHAR)'m', (TCHAR)'p', (TCHAR)'3', (TCHAR)0
};

static audio_app_fatfs_source_t audio_app_default_fatfs_source =
{
    .p_path = audio_app_default_mp3_path
};

static audio_pcm_block_t       audio_app_pcm_blocks[AUDIO_APP_PCM_BLOCK_COUNT];
static audio_pcm_block_t       audio_app_startup_silence_block;
static audio_player_service_t  audio_app_player_service;
static mp3_decoder_service_t   audio_app_decoder_service;
static volatile mp3_decoder_service_status_t audio_app_decoder_last_status;

static QueueHandle_t audio_app_free_queue;
static QueueHandle_t audio_app_ready_queue;
static QueueHandle_t audio_app_player_command_queue;
static QueueHandle_t audio_app_decode_command_queue;
static TaskHandle_t  audio_app_player_task_handle;
static TaskHandle_t  audio_app_decoder_task_handle;

static StaticQueue_t audio_app_free_queue_control;
static StaticQueue_t audio_app_ready_queue_control;
static StaticQueue_t audio_app_player_command_queue_control;
static StaticQueue_t audio_app_decode_command_queue_control;
static uint8_t audio_app_free_queue_storage[
    AUDIO_APP_PCM_BLOCK_COUNT * sizeof(audio_pcm_block_t *)];
static uint8_t audio_app_ready_queue_storage[
    AUDIO_APP_PCM_BLOCK_COUNT * sizeof(audio_pcm_block_t *)];
static uint8_t audio_app_player_command_queue_storage[
    AUDIO_APP_PLAYER_COMMAND_COUNT * sizeof(audio_app_player_command_t)];
static uint8_t audio_app_decode_command_queue_storage[
    AUDIO_APP_DECODE_COMMAND_COUNT * sizeof(audio_app_decode_command_t)];

static StaticTask_t audio_app_player_task_control;
static StaticTask_t audio_app_decoder_task_control;
static StackType_t  audio_app_player_stack[AUDIO_APP_PLAYER_STACK_WORDS];
static StackType_t  audio_app_decoder_stack[AUDIO_APP_DECODER_STACK_WORDS];

static audio_data_status_t audio_app_fatfs_open(void *p_context);
static audio_data_status_t audio_app_fatfs_read(void     *p_context,
                                                uint8_t  *p_buffer,
                                                uint32_t  buffer_size,
                                                uint32_t *p_read_size);
static audio_data_status_t audio_app_fatfs_seek(void    *p_context,
                                                uint32_t offset);
static void audio_app_fatfs_close(void *p_context);
static void audio_app_output_callback(bsp_audio_output_event_t event);
static void audio_app_player_task(void *p_parameter);
static void audio_app_decoder_task(void *p_parameter);

static const audio_data_source_ops_t audio_app_fatfs_source_ops =
{
    audio_app_fatfs_open,
    audio_app_fatfs_read,
    audio_app_fatfs_seek,
    audio_app_fatfs_close
};

static const audio_data_source_t audio_app_default_source =
{
    &audio_app_fatfs_source_ops,
    &audio_app_default_fatfs_source
};

static audio_data_status_t audio_app_fatfs_open(void *p_context)
{
    audio_app_fatfs_source_t *p_source =
        (audio_app_fatfs_source_t *)p_context;
    FRESULT result;

    if((NULL == p_source) || (NULL == p_source->p_path) ||
       (0U != p_source->is_open))
    {
        return AUDIO_DATA_STATUS_PARAM;
    }

    result = f_mount(&p_source->filesystem, _T("0:"), 1U);
    if(FR_OK != result)
    {
        return AUDIO_DATA_STATUS_IO;
    }
    p_source->is_mounted = 1U;

    result = f_open(&p_source->file, p_source->p_path, FA_READ);
    if(FR_OK != result)
    {
        (void)f_mount(NULL, _T("0:"), 0U);
        p_source->is_mounted = 0U;
        return AUDIO_DATA_STATUS_IO;
    }
    p_source->is_open = 1U;
    return AUDIO_DATA_STATUS_OK;
}

static audio_data_status_t audio_app_fatfs_read(void     *p_context,
                                                uint8_t  *p_buffer,
                                                uint32_t  buffer_size,
                                                uint32_t *p_read_size)
{
    audio_app_fatfs_source_t *p_source =
        (audio_app_fatfs_source_t *)p_context;
    FRESULT result;
    UINT    read_size = 0U;

    if((NULL == p_source) || (NULL == p_buffer) ||
       (NULL == p_read_size) || (0U == buffer_size) ||
       (0U == p_source->is_open) || (buffer_size > (uint32_t)UINT_MAX))
    {
        return AUDIO_DATA_STATUS_PARAM;
    }

    result = f_read(&p_source->file,
                    p_buffer,
                    (UINT)buffer_size,
                    &read_size);
    *p_read_size = (uint32_t)read_size;
    if(FR_OK != result)
    {
        return AUDIO_DATA_STATUS_IO;
    }
    return (read_size < (UINT)buffer_size) ? AUDIO_DATA_STATUS_EOF :
                                             AUDIO_DATA_STATUS_OK;
}

static audio_data_status_t audio_app_fatfs_seek(void    *p_context,
                                                uint32_t offset)
{
    audio_app_fatfs_source_t *p_source =
        (audio_app_fatfs_source_t *)p_context;

    if((NULL == p_source) || (0U == p_source->is_open))
    {
        return AUDIO_DATA_STATUS_PARAM;
    }
    return (FR_OK == f_lseek(&p_source->file, (FSIZE_t)offset)) ?
           AUDIO_DATA_STATUS_OK : AUDIO_DATA_STATUS_IO;
}

static void audio_app_fatfs_close(void *p_context)
{
    audio_app_fatfs_source_t *p_source =
        (audio_app_fatfs_source_t *)p_context;

    if(NULL == p_source)
    {
        return;
    }
    if(0U != p_source->is_open)
    {
        (void)f_close(&p_source->file);
        p_source->is_open = 0U;
    }
    if(0U != p_source->is_mounted)
    {
        (void)f_mount(NULL, _T("0:"), 0U);
        p_source->is_mounted = 0U;
    }
}

static void audio_app_output_callback(bsp_audio_output_event_t event)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint32_t notify_bits = 0U;

    if(0U != ((uint32_t)event &
              (uint32_t)BSP_AUDIO_OUTPUT_EVENT_FIRST_HALF_WRITABLE))
    {
        notify_bits |= AUDIO_APP_PLAYER_NOTIFY_DMA_HALF;
    }
    if(0U != ((uint32_t)event &
              (uint32_t)BSP_AUDIO_OUTPUT_EVENT_SECOND_HALF_WRITABLE))
    {
        notify_bits |= AUDIO_APP_PLAYER_NOTIFY_DMA_FULL;
    }
    if(0U != ((uint32_t)event &
              (uint32_t)BSP_AUDIO_OUTPUT_EVENT_ERROR))
    {
        notify_bits |= AUDIO_APP_PLAYER_NOTIFY_DMA_ERROR;
    }

    if((0U != notify_bits) && (NULL != audio_app_player_task_handle))
    {
        (void)xTaskNotifyFromISR(audio_app_player_task_handle,
                                 notify_bits,
                                 eSetBits,
                                 &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static void audio_app_block_return(audio_pcm_block_t *p_block)
{
    if(NULL != p_block)
    {
        (void)xQueueSend(audio_app_free_queue, &p_block, portMAX_DELAY);
    }
}

static void audio_app_ready_queue_flush(void)
{
    audio_pcm_block_t *p_block;

    while(pdPASS == xQueueReceive(audio_app_ready_queue, &p_block, 0U))
    {
        audio_app_block_return(p_block);
    }
}

static audio_pcm_block_t *audio_app_active_block_take(void)
{
    audio_pcm_block_t *p_block;

    while(pdPASS == xQueueReceive(audio_app_ready_queue, &p_block, 0U))
    {
        if(p_block->session_id == audio_app_player_service.session_id)
        {
            return p_block;
        }
        audio_app_block_return(p_block);
    }
    return NULL;
}

static void audio_app_decoder_stop_request(void)
{
    if(NULL != audio_app_decoder_task_handle)
    {
        (void)xTaskNotify(audio_app_decoder_task_handle,
                          AUDIO_APP_DECODER_NOTIFY_STOP,
                          eSetBits);
    }
}

static void audio_app_player_stop(uint8_t decoder_stop_required)
{
    if(0U != decoder_stop_required)
    {
        audio_app_decoder_stop_request();
    }
    (void)audio_player_service_stop(&audio_app_player_service);
    audio_app_ready_queue_flush();
}

static void audio_app_player_start_if_ready(uint8_t decoder_eof)
{
    audio_pcm_block_t *p_first_block = NULL;
    audio_pcm_block_t *p_second_block = NULL;
    platform_err_t ret;

    if(AUDIO_PLAYER_SERVICE_STATE_PREPARING !=
       audio_player_service_state_get(&audio_app_player_service))
    {
        return;
    }

    if((0U == decoder_eof) &&
       (uxQueueMessagesWaiting(audio_app_ready_queue) <
        AUDIO_APP_START_PREBUFFER_BLOCKS))
    {
        return;
    }

    p_first_block = audio_app_active_block_take();
    if(NULL == p_first_block)
    {
        return;
    }
    p_second_block = audio_app_active_block_take();
    if(NULL == p_second_block)
    {
        if(0U == decoder_eof)
        {
            (void)xQueueSend(audio_app_ready_queue, &p_first_block, 0U);
            return;
        }
        memset(&audio_app_startup_silence_block,
               0,
               sizeof(audio_app_startup_silence_block));
        audio_app_startup_silence_block.session_id =
            p_first_block->session_id;
        audio_app_startup_silence_block.sample_rate_hz =
            p_first_block->sample_rate_hz;
        p_second_block = &audio_app_startup_silence_block;
    }

    ret = audio_player_service_start(&audio_app_player_service,
                                     p_first_block,
                                     p_second_block);
    audio_app_block_return(p_first_block);
    if(p_second_block != &audio_app_startup_silence_block)
    {
        audio_app_block_return(p_second_block);
    }

    plat_log_i("Audio MP3 output start ret=%d, rate=%lu",
               (int32_t)ret,
               (unsigned long)audio_app_player_service.sample_rate_hz);
    if(PLATFORM_ERR_OK == ret)
    {
        vTaskDelay(pdMS_TO_TICKS(AUDIO_APP_OUTPUT_STABILIZE_MS));
        ret = audio_player_service_unmute(&audio_app_player_service);
        plat_log_i("Audio MP3 unmute ret=%d", (int32_t)ret);
    }
    if(PLATFORM_ERR_OK != ret)
    {
        audio_app_player_stop(1U);
    }
}

static void audio_app_player_refill(bsp_audio_output_event_t event,
                                    uint8_t                  decoder_eof)
{
    audio_pcm_block_t *p_block = NULL;
    platform_err_t ret;

    p_block = audio_app_active_block_take();
    ret = audio_player_service_refill(&audio_app_player_service,
                                      event,
                                      p_block,
                                      (uint8_t)((NULL == p_block) &&
                                                (0U == decoder_eof)));
    audio_app_block_return(p_block);

    if(PLATFORM_ERR_OK != ret)
    {
        plat_log_e("Audio MP3 refill failed, ret=%d", (int32_t)ret);
        audio_app_player_stop(1U);
        return;
    }
    if((NULL == p_block) && (0U == decoder_eof))
    {
        plat_log_w("Audio MP3 underrun, count=%lu",
                   (unsigned long)audio_app_player_service.underrun_count);
    }
}

static void audio_app_player_task(void *p_parameter)
{
    audio_app_player_command_t player_command;
    audio_app_decode_command_t decode_command;
    platform_err_t ret;
    uint32_t notify_bits;
    uint32_t session_id = 0U;
    uint8_t decoder_eof = 0U;

    (void)p_parameter;
    ret = audio_player_service_init(&audio_app_player_service,
                                    audio_app_output_callback);
    plat_log_i("Audio player service init ret=%d", (int32_t)ret);

    for(;;)
    {
        notify_bits = 0U;
        (void)xTaskNotifyWait(0U, UINT32_MAX, &notify_bits, portMAX_DELAY);

        if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_COMMAND))
        {
            while(pdPASS == xQueueReceive(audio_app_player_command_queue,
                                           &player_command,
                                           0U))
            {
                if(AUDIO_APP_PLAYER_COMMAND_PLAY == player_command.type)
                {
                    if(AUDIO_PLAYER_SERVICE_STATE_IDLE !=
                       audio_player_service_state_get(&audio_app_player_service))
                    {
                        plat_log_w("Audio play request rejected: busy");
                        continue;
                    }
                    session_id++;
                    if(0U == session_id)
                    {
                        session_id = 1U;
                    }
                    decoder_eof = 0U;
                    ret = audio_player_service_prepare(
                        &audio_app_player_service,
                        session_id);
                    if(PLATFORM_ERR_OK == ret)
                    {
                        decode_command.source = player_command.source;
                        decode_command.session_id = session_id;
                        if(pdPASS != xQueueSend(audio_app_decode_command_queue,
                                                &decode_command,
                                                0U))
                        {
                            ret = PLATFORM_ERR_BUSY;
                        }
                    }
                    plat_log_i("Audio MP3 prepare ret=%d, session=%lu",
                               (int32_t)ret,
                               (unsigned long)session_id);
                    if(PLATFORM_ERR_OK != ret)
                    {
                        audio_app_player_stop(0U);
                    }
                }
                else if(AUDIO_APP_PLAYER_COMMAND_STOP == player_command.type)
                {
                    audio_app_player_stop(1U);
                    plat_log_i("Audio playback stopped");
                }
                else
                {
                    audio_app_decoder_stop_request();
                    ret = audio_player_service_emergency_set(
                        &audio_app_player_service,
                        player_command.emergency_active);
                    audio_app_ready_queue_flush();
                    plat_log_i("Audio emergency=%u ret=%d",
                               (unsigned int)player_command.emergency_active,
                               (int32_t)ret);
                }
            }
        }

        if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_DECODE_ERROR))
        {
            plat_log_e("Audio MP3 decode failed, status=%u",
                       (unsigned int)audio_app_decoder_last_status);
            audio_app_player_stop(0U);
        }
        if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_DECODE_EOF))
        {
            decoder_eof = 1U;
            plat_log_i("Audio MP3 decoder EOF");
        }

        if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_PCM_READY))
        {
            audio_app_player_start_if_ready(decoder_eof);
        }
        if((0U != decoder_eof) &&
           (AUDIO_PLAYER_SERVICE_STATE_PREPARING ==
            audio_player_service_state_get(&audio_app_player_service)))
        {
            audio_app_player_start_if_ready(decoder_eof);
        }

        if(AUDIO_PLAYER_SERVICE_STATE_PLAYING_CS4344 ==
           audio_player_service_state_get(&audio_app_player_service))
        {
            if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_DMA_HALF))
            {
                audio_app_player_refill(
                    BSP_AUDIO_OUTPUT_EVENT_FIRST_HALF_WRITABLE,
                    decoder_eof);
            }
            if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_DMA_FULL))
            {
                audio_app_player_refill(
                    BSP_AUDIO_OUTPUT_EVENT_SECOND_HALF_WRITABLE,
                    decoder_eof);
            }
            if(0U != (notify_bits & AUDIO_APP_PLAYER_NOTIFY_DMA_ERROR))
            {
                plat_log_e("Audio MP3 EDMA error");
                audio_app_player_stop(1U);
            }
            else if((0U != decoder_eof) &&
                    (0U == uxQueueMessagesWaiting(audio_app_ready_queue)) &&
                    (0U == audio_player_service_has_pending_audio(
                              &audio_app_player_service)))
            {
                plat_log_i("Audio MP3 playback complete, underrun=%lu",
                           (unsigned long)
                               audio_app_player_service.underrun_count);
                audio_app_player_stop(0U);
                plat_log_i("Audio player stack min free=%lu words",
                           (unsigned long)uxTaskGetStackHighWaterMark(NULL));
            }
        }
    }
}

static void audio_app_decoder_task(void *p_parameter)
{
    audio_app_decode_command_t command;
    audio_pcm_block_t *p_block;
    mp3_decoder_service_status_t status;
    uint32_t notify_value;
    uint8_t stop_requested;

    (void)p_parameter;
    mp3_decoder_service_init(&audio_app_decoder_service);

    for(;;)
    {
        (void)xQueueReceive(audio_app_decode_command_queue,
                            &command,
                            portMAX_DELAY);
        status = mp3_decoder_service_start(&audio_app_decoder_service,
                                           &command.source,
                                           command.session_id);
        plat_log_i("Audio MP3 decoder start status=%u",
                   (unsigned int)status);
        if(MP3_DECODER_SERVICE_STATUS_OK != status)
        {
            audio_app_decoder_last_status = status;
            (void)xTaskNotify(audio_app_player_task_handle,
                              AUDIO_APP_PLAYER_NOTIFY_DECODE_ERROR,
                              eSetBits);
            continue;
        }

        stop_requested = 0U;
        while(0U == stop_requested)
        {
            notify_value = 0U;
            if(pdPASS == xTaskNotifyWait(0U,
                                         UINT32_MAX,
                                         &notify_value,
                                         0U))
            {
                stop_requested = (uint8_t)(
                    0U != (notify_value & AUDIO_APP_DECODER_NOTIFY_STOP));
                if(0U != stop_requested)
                {
                    break;
                }
            }

            if(pdPASS != xQueueReceive(
                             audio_app_free_queue,
                             &p_block,
                             pdMS_TO_TICKS(AUDIO_APP_DECODER_WAIT_MS)))
            {
                continue;
            }

            status = mp3_decoder_service_process(&audio_app_decoder_service,
                                                  p_block);
            if(MP3_DECODER_SERVICE_STATUS_PCM_READY == status)
            {
                (void)xQueueSend(audio_app_ready_queue,
                                 &p_block,
                                 portMAX_DELAY);
                (void)xTaskNotify(audio_app_player_task_handle,
                                  AUDIO_APP_PLAYER_NOTIFY_PCM_READY,
                                  eSetBits);
            }
            else
            {
                audio_app_block_return(p_block);
                if(MP3_DECODER_SERVICE_STATUS_AGAIN == status)
                {
                    vTaskDelay(1U);
                }
                else if(MP3_DECODER_SERVICE_STATUS_EOF == status)
                {
                    (void)xTaskNotify(audio_app_player_task_handle,
                                      AUDIO_APP_PLAYER_NOTIFY_DECODE_EOF,
                                      eSetBits);
                    break;
                }
                else
                {
                    audio_app_decoder_last_status = status;
                    (void)xTaskNotify(audio_app_player_task_handle,
                                      AUDIO_APP_PLAYER_NOTIFY_DECODE_ERROR,
                                      eSetBits);
                    break;
                }
            }
        }

        mp3_decoder_service_stop(&audio_app_decoder_service);
        plat_log_i("Audio decoder stack min free=%lu words",
                   (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    }
}

platform_err_t audio_playback_app_init(void)
{
    uint32_t block_index;
    audio_pcm_block_t *p_block;

    audio_app_free_queue = xQueueCreateStatic(
        AUDIO_APP_PCM_BLOCK_COUNT,
        sizeof(audio_pcm_block_t *),
        audio_app_free_queue_storage,
        &audio_app_free_queue_control);
    audio_app_ready_queue = xQueueCreateStatic(
        AUDIO_APP_PCM_BLOCK_COUNT,
        sizeof(audio_pcm_block_t *),
        audio_app_ready_queue_storage,
        &audio_app_ready_queue_control);
    audio_app_player_command_queue = xQueueCreateStatic(
        AUDIO_APP_PLAYER_COMMAND_COUNT,
        sizeof(audio_app_player_command_t),
        audio_app_player_command_queue_storage,
        &audio_app_player_command_queue_control);
    audio_app_decode_command_queue = xQueueCreateStatic(
        AUDIO_APP_DECODE_COMMAND_COUNT,
        sizeof(audio_app_decode_command_t),
        audio_app_decode_command_queue_storage,
        &audio_app_decode_command_queue_control);

    if((NULL == audio_app_free_queue) || (NULL == audio_app_ready_queue) ||
       (NULL == audio_app_player_command_queue) ||
       (NULL == audio_app_decode_command_queue))
    {
        return PLATFORM_ERR_HW;
    }

    for(block_index = 0U;
        block_index < AUDIO_APP_PCM_BLOCK_COUNT;
        block_index++)
    {
        p_block = &audio_app_pcm_blocks[block_index];
        if(pdPASS != xQueueSend(audio_app_free_queue, &p_block, 0U))
        {
            return PLATFORM_ERR_HW;
        }
    }

    audio_app_decoder_task_handle = xTaskCreateStatic(
        audio_app_decoder_task,
        "mp3_decode",
        AUDIO_APP_DECODER_STACK_WORDS,
        NULL,
        AUDIO_APP_DECODER_PRIORITY,
        audio_app_decoder_stack,
        &audio_app_decoder_task_control);
    audio_app_player_task_handle = xTaskCreateStatic(
        audio_app_player_task,
        "audio_player",
        AUDIO_APP_PLAYER_STACK_WORDS,
        NULL,
        AUDIO_APP_PLAYER_PRIORITY,
        audio_app_player_stack,
        &audio_app_player_task_control);
    if((NULL == audio_app_decoder_task_handle) ||
       (NULL == audio_app_player_task_handle))
    {
        return PLATFORM_ERR_HW;
    }
    return PLATFORM_ERR_OK;
}

platform_err_t audio_playback_app_play(
    const audio_data_source_t *p_source)
{
    audio_app_player_command_t command;

    if((NULL == p_source) || (NULL == p_source->p_ops) ||
       (NULL == audio_app_player_command_queue) ||
       (NULL == audio_app_player_task_handle))
    {
        return PLATFORM_ERR_PARAM;
    }
    memset(&command, 0, sizeof(command));
    command.type = AUDIO_APP_PLAYER_COMMAND_PLAY;
    command.source = *p_source;
    if(pdPASS != xQueueSend(audio_app_player_command_queue, &command, 0U))
    {
        return PLATFORM_ERR_BUSY;
    }
    (void)xTaskNotify(audio_app_player_task_handle,
                      AUDIO_APP_PLAYER_NOTIFY_COMMAND,
                      eSetBits);
    return PLATFORM_ERR_OK;
}

platform_err_t audio_playback_app_play_default(void)
{
    return audio_playback_app_play(&audio_app_default_source);
}

platform_err_t audio_playback_app_stop(void)
{
    audio_app_player_command_t command;

    if((NULL == audio_app_player_command_queue) ||
       (NULL == audio_app_player_task_handle))
    {
        return PLATFORM_ERR_HW;
    }
    memset(&command, 0, sizeof(command));
    command.type = AUDIO_APP_PLAYER_COMMAND_STOP;
    if(pdPASS != xQueueSend(audio_app_player_command_queue, &command, 0U))
    {
        return PLATFORM_ERR_BUSY;
    }
    (void)xTaskNotify(audio_app_player_task_handle,
                      AUDIO_APP_PLAYER_NOTIFY_COMMAND,
                      eSetBits);
    return PLATFORM_ERR_OK;
}

platform_err_t audio_playback_app_emergency_set(uint8_t is_active)
{
    audio_app_player_command_t command;

    if((is_active > 1U) || (NULL == audio_app_player_command_queue) ||
       (NULL == audio_app_player_task_handle))
    {
        return PLATFORM_ERR_PARAM;
    }
    memset(&command, 0, sizeof(command));
    command.type = AUDIO_APP_PLAYER_COMMAND_EMERGENCY;
    command.emergency_active = is_active;
    if(pdPASS != xQueueSend(audio_app_player_command_queue, &command, 0U))
    {
        return PLATFORM_ERR_BUSY;
    }
    (void)xTaskNotify(audio_app_player_task_handle,
                      AUDIO_APP_PLAYER_NOTIFY_COMMAND,
                      eSetBits);
    return PLATFORM_ERR_OK;
}
