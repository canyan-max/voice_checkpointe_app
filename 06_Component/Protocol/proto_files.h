/**
 ******************************************************************************
 *@file               :   proto_files.h
 *@brief              :   Provide the HAL APIs of description.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PROTO_FILES_H
#define PROTO_FILES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h> /* stdint lib header file. */
#include <stddef.h> /* stdint lib header file. */

/* define -------------------------------------------------------------------*/
#define PACKET_DATA_INDEX     (4U)    /* frame header len: pack_type+reserved+pack_num+~pack_num */
#define PACK_FRAME_SIZE_128B  (128U)
#define PACK_FRAME_SIZE_1K    (1024U)
/* typedef ------------------------------------------------------------------*/
typedef enum
{
    PROTO_FILE_RET_OK = 0x00U,
    PROTO_FILE_RET_ERR,
    PROTO_FILE_RET_PACKNUM_ERR,
    PROTO_FILE_RET_CRC_ERR,
    PROTO_FILE_RET_HEAD_ERR
} proto_files_ret_t;

typedef enum
{
    PACK_TYPE_128_FRAM  = 0x01U,
    PACK_TYPE_1024_FRAM = 0x02U,
} pack_type_frame_t;

typedef enum
{
    PROTO_FILE_IDLE                = 0x00U,
    PROTO_FILE_RECEIVE_FILE_INFO   = 0x01U,
    PROTO_FILE_RECEIVE_DATA_INFO   = 0x02U,
    PROTO_FILE_RECEIVE_END_SESSION = 0x03U,
} proto_files_state_t;

typedef struct
{
    proto_files_state_t state;
    pack_type_frame_t   pack_type;
    uint8_t             reserver;
    uint8_t             package_number;
    uint8_t             package_un_numer;
    uint16_t idx;       /* bytes accumulated into buf for the current frame */
    uint16_t frame_len; /* total bytes needed (RECEIVE_DATA_INFO only, depends
                           on pack_type) */
    uint8_t esc_idx;    /* position inside the 3-byte "ESC" end-session marker
                           (RECEIVE_END_SESSION only); separate from idx so it
                           never collides with frame accumulation */
    uint8_t *buf;       /* points at a static max-frame accumulation buffer,
                           assigned by proto_files_init() (see proto_files.c) */
} proto_files_parser_t;
/* exported types -----------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
proto_files_ret_t proto_files_init(proto_files_parser_t *p_parser);
/**
 * @brief            : [proto_files_feed] feed one received byte into the
 *                     parser state machine. Frame length (handshake=7B,
 *                     file-info=134B, data-pack=134B/1030B) is inferred
 *                     internally, so the caller never needs to know how
 *                     many bytes make up "one frame" up front.
 * @retval           : [PROTO_FILE_RET_OK] byte accepted (frame may still
 *                     be accumulating, or just completed successfully)
 * @retval           : [PROTO_FILE_RET_ERR / _HEAD_ERR / _PACKNUM_ERR / _CRC_ERR]
 *                     frame validation failed; parser is reset to PROTO_FILE_IDLE
 * @param[in]        : [byte] next raw byte from the UART stream
 */
proto_files_ret_t proto_files_feed(proto_files_parser_t *p_parser,
                                   uint8_t               byte);
#ifdef __cplusplus
}
#endif

#endif /* PROTO_FILES_H */
