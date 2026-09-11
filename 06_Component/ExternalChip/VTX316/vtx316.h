/**
 ******************************************************************************
 *@file               :   vtx316.h
 *@brief              :   Provide a portable VTX316 UART control interface.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef VTX316_H
#define VTX316_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* define -------------------------------------------------------------------*/
#define VTX316_TEXT_MAX_BYTES       4000U
#define VTX316_UTF8_TEXT_MAX_BYTES  2000U

/* typedef ------------------------------------------------------------------*/
typedef enum VTX316_RET_T
{
    VTX316_RET_OK = 0U,
    VTX316_RET_PARAM,
    VTX316_RET_IO,
    VTX316_RET_TIMEOUT,
    VTX316_RET_BUSY,
    VTX316_RET_NOT_READY
} vtx316_ret_t;

typedef enum VTX316_STATE_T
{
    VTX316_STATE_UNKNOWN = 0U,
    VTX316_STATE_IDLE,
    VTX316_STATE_COMMAND_PENDING,
    VTX316_STATE_SPEAKING,
    VTX316_STATE_SLEEPING,
    VTX316_STATE_ERROR
} vtx316_state_t;

typedef enum VTX316_EVENT_T
{
    VTX316_EVENT_NONE             = 0U,
    VTX316_EVENT_INITIALIZED      = (1U << 0),
    VTX316_EVENT_COMMAND_ACCEPTED = (1U << 1),
    VTX316_EVENT_COMMAND_REJECTED = (1U << 2),
    VTX316_EVENT_SPEAKING         = (1U << 3),
    VTX316_EVENT_IDLE             = (1U << 4),
    VTX316_EVENT_SLEEPING         = (1U << 5)
} vtx316_event_t;

typedef enum VTX316_ENCODING_T
{
    VTX316_ENCODING_GB2312         = 0x00U,
    VTX316_ENCODING_GBK            = 0x01U,
    VTX316_ENCODING_UNICODE_LITTLE = 0x03U,
    VTX316_ENCODING_UNICODE_BIG    = 0x04U,
    VTX316_ENCODING_UTF8           = 0x05U
} vtx316_encoding_t;

typedef vtx316_ret_t (*vtx316_transmit_t)(void          *p_context,
                                           const uint8_t *p_header,
                                           uint16_t       header_size,
                                           const uint8_t *p_payload,
                                           uint16_t       payload_size,
                                           uint32_t       timeout_ms);
typedef vtx316_ret_t (*vtx316_busy_read_t)(void    *p_context,
                                            uint8_t *p_is_busy);

typedef struct VTX316_IO_T
{
    vtx316_transmit_t  transmit;
    vtx316_busy_read_t busy_read;
    void              *p_context;
} vtx316_io_t;

typedef struct VTX316_DEVICE_T
{
    vtx316_io_t io;
    vtx316_state_t state;
    uint8_t     active_command;
    uint8_t     is_initialized;
} vtx316_device_t;

/* functions ----------------------------------------------------------------*/
vtx316_ret_t vtx316_init(vtx316_device_t *p_device, const vtx316_io_t *p_io);

/**
 * @brief Send one text synthesis frame.
 * @param p_text Text bytes already encoded as specified by encoding.
 * @note This function does not convert character encodings or wait for the
 *       asynchronous 0x41/0x4F response.
 */
vtx316_ret_t vtx316_speak(vtx316_device_t  *p_device,
                          vtx316_encoding_t  encoding,
                          const uint8_t      *p_text,
                          uint16_t            text_size,
                          uint32_t            timeout_ms);

vtx316_ret_t vtx316_stop(vtx316_device_t *p_device, uint32_t timeout_ms);

/**
 * @brief Read the R/B pin as a logical busy state.
 * @param p_is_busy Set to 1 while speaking, otherwise 0.
 */
vtx316_ret_t vtx316_busy_get(vtx316_device_t *p_device,
                             uint8_t          *p_is_busy);

/**
 * @brief Consume bytes returned by the chip and update its state.
 * @param p_event Receives ORed VTX316_EVENT_* flags found in this data block.
 */
vtx316_ret_t vtx316_response_process(vtx316_device_t *p_device,
                                     const uint8_t   *p_data,
                                     uint16_t         size,
                                     vtx316_event_t  *p_event);

vtx316_ret_t vtx316_state_get(vtx316_device_t *p_device,
                              vtx316_state_t  *p_state);

#ifdef __cplusplus
}
#endif

#endif /* VTX316_H */
