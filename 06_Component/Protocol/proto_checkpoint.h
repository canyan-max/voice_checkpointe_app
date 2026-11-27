/**
 ******************************************************************************
 *@file               :   proto_checkpoint.h
 *@brief              :   Define the checkpoint request parser and reply API.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* define to prevent recursive inclusion ------------------------------------*/
#ifndef PROTO_CHECKPOINT_H
#define PROTO_CHECKPOINT_H

/* includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* define -------------------------------------------------------------------*/
#define PROTO_CHECKPOINT_DEFAULT_ID (0x01U)
#define PROTO_CHECKPOINT_REPLY_SIZE (7U)
#define PROTO_CHECKPOINT_VOICE_TEXT_MAX_BYTES (400U)

/* typedef ------------------------------------------------------------------*/
typedef enum PROTO_CHECKPOINT_COMMAND_T {
    PROTO_CHECKPOINT_VOICE_1        = 0xF0U,
    PROTO_CHECKPOINT_DISPLAY_1      = 0xF1U,
    PROTO_CHECKPOINT_DEVICE_ID      = 0xF2U,
    PROTO_CHECKPOINT_PIR_ENABLE     = 0xF3U,
    PROTO_CHECKPOINT_VOICE_REPEAT   = 0xF4U,
    PROTO_CHECKPOINT_DISPLAY_REPEAT = 0xF5U,
    PROTO_CHECKPOINT_PLAY           = 0xF6U,
    PROTO_CHECKPOINT_VOICE_2        = 0xF7U,
    PROTO_CHECKPOINT_VOICE_3        = 0xF8U,
    PROTO_CHECKPOINT_DISPLAY_2      = 0xF9U,
    PROTO_CHECKPOINT_DISPLAY_3      = 0xFAU,
    PROTO_CHECKPOINT_PIR_ITEM       = 0xFBU,
} proto_checkpoint_command_t;

typedef enum PROTO_CHECKPOINT_RESULT_T {
    PROTO_CHECKPOINT_MORE = 0U,
    PROTO_CHECKPOINT_FRAME,
    PROTO_CHECKPOINT_IGNORED,
    PROTO_CHECKPOINT_INVALID,
    PROTO_CHECKPOINT_BAD_CHECKSUM,
} proto_checkpoint_result_t;

typedef struct PROTO_CHECKPOINT_REQUEST_T {
    const uint8_t *p_payload;
    uint16_t       length;
    uint8_t        id;
    uint8_t        command;
} proto_checkpoint_request_t;

typedef struct PROTO_CHECKPOINT_PARSER_T {
    uint8_t *p_payload;
    uint16_t capacity;
    uint16_t received;
    uint16_t length;
    uint16_t sum;
    uint16_t wire_sum;
    uint8_t  device_id;
    uint8_t  id;
    uint8_t  command;
    uint8_t  state; /* Internal receive state. */
} proto_checkpoint_parser_t;

/* variables ----------------------------------------------------------------*/

/* function  ----------------------------------------------------------------*/

/* Caller owns the payload buffer and lends it for the parser's lifetime.
 * Returns 1 on success or 0 for invalid arguments. */
uint8_t proto_checkpoint_init(proto_checkpoint_parser_t *p_parser,
                              uint8_t                    device_id,
                              uint8_t                   *p_payload,
                              uint16_t                   capacity);

/* Call after a receive error or a confirmed gap that ends a partial frame. */
void proto_checkpoint_reset(proto_checkpoint_parser_t *p_parser);

/* Bytes collected for the in-progress frame, including its header; zero if idle. */
uint32_t proto_checkpoint_partial_size(const proto_checkpoint_parser_t *p_parser);

/* FRAME exposes a borrowed payload, valid until the next feed or reset.
 * BAD_CHECKSUM exposes the addressed request's ID and command for a reply.
 * Its payload is unavailable and must not be processed.
 * IGNORED indicates a valid frame addressed to another device. */
proto_checkpoint_result_t
proto_checkpoint_feed(proto_checkpoint_parser_t  *p_parser,
                      uint8_t                     byte,
                      proto_checkpoint_request_t *p_request);

/* Check command data after the request envelope has passed validation. */
uint8_t
proto_checkpoint_payload_valid(const proto_checkpoint_request_t *p_request);

/* Return the number of bytes written, or zero on invalid arguments. */
size_t proto_checkpoint_encode_reply(uint8_t *p_reply,
                                     size_t   capacity,
                                     uint8_t  id,
                                     uint8_t  command,
                                     uint8_t  data);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_CHECKPOINT_H */
/* end of file --------------------------------------------------------------*/
