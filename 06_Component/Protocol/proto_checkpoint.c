/**
 ******************************************************************************
 *@file               :   proto_checkpoint.c
 *@brief              :   Parse checkpoint requests and encode replies.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* includes -----------------------------------------------------------------*/
#include "proto_checkpoint.h"
/* define   -----------------------------------------------------------------*/
#define REQUEST_TAIL (0x9FU)
#define REPLY_TAIL   (0xAFU)
#define QUERY_ID     (0xFFU)
#define DISPLAY_TEXT_MAX_BYTES (200U)
#define REPEAT_MAX             (16U)
#define ITEM_MAX               (3U)

/* typedef ------------------------------------------------------------------*/
typedef enum RECEIVE_STATE_T {
    RECEIVE_ID = 0U,
    RECEIVE_COMMAND,
    RECEIVE_LENGTH_HIGH,
    RECEIVE_LENGTH_LOW,
    RECEIVE_PAYLOAD,
    RECEIVE_SUM_HIGH,
    RECEIVE_SUM_LOW,
    RECEIVE_TAIL,
} receive_state_t;

/* variables ----------------------------------------------------------------*/

/* private functions --------------------------------------------------------*/
static uint8_t command_is_valid(uint8_t command) {
    return (uint8_t)((command >= PROTO_CHECKPOINT_VOICE_1) &&
                     (command <= PROTO_CHECKPOINT_PIR_ITEM));
}

static proto_checkpoint_result_t
finish_request(const proto_checkpoint_parser_t *p_parser,
               uint8_t                          tail,
               proto_checkpoint_request_t      *p_request) {
    if ((REQUEST_TAIL != tail) || (p_parser->length > p_parser->capacity)) {
        return PROTO_CHECKPOINT_INVALID;
    }

    if (p_parser->id != p_parser->device_id) {
        if ((QUERY_ID != p_parser->id) ||
            (PROTO_CHECKPOINT_DEVICE_ID != p_parser->command) ||
            (1U != p_parser->length) || (QUERY_ID != p_parser->p_payload[0U])) {
            return (p_parser->sum == p_parser->wire_sum)
                       ? PROTO_CHECKPOINT_IGNORED
                       : PROTO_CHECKPOINT_INVALID;
        }
    }

    p_request->id      = p_parser->id;
    p_request->command = p_parser->command;
    if (p_parser->sum != p_parser->wire_sum) {
        p_request->p_payload = NULL;
        p_request->length    = 0U;
        return PROTO_CHECKPOINT_BAD_CHECKSUM;
    }

    p_request->p_payload = p_parser->p_payload;
    p_request->length    = p_parser->length;
    return PROTO_CHECKPOINT_FRAME;
}

static uint8_t
text_valid(const uint8_t *p_data, uint16_t length, uint16_t max_bytes) {
    if (length > max_bytes) {
        return 0U;
    }
    for (uint16_t i = 0U; i < length; i++) {
        uint8_t lead = p_data[i];
        if (lead <= 0x7FU) {
            continue;
        }
        if ((lead < 0x81U) || (lead > 0xFEU) || (++i >= length)) {
            return 0U;
        }
        uint8_t trail = p_data[i];
        if ((trail < 0x40U) || (0x7FU == trail) || (trail > 0xFEU)) {
            return 0U;
        }
    }
    return 1U;
}

/* exported functions -------------------------------------------------------*/
uint8_t
proto_checkpoint_payload_valid(const proto_checkpoint_request_t *p_request) {
    if ((NULL == p_request) || (NULL == p_request->p_payload)) {
        return 0U;
    }

    switch (p_request->command) {
        case PROTO_CHECKPOINT_VOICE_1:
        case PROTO_CHECKPOINT_VOICE_2:
        case PROTO_CHECKPOINT_VOICE_3:
            return text_valid(p_request->p_payload, p_request->length,
                              PROTO_CHECKPOINT_VOICE_TEXT_MAX_BYTES);
        case PROTO_CHECKPOINT_DISPLAY_1:
        case PROTO_CHECKPOINT_DISPLAY_2:
        case PROTO_CHECKPOINT_DISPLAY_3:
            return text_valid(p_request->p_payload, p_request->length,
                              DISPLAY_TEXT_MAX_BYTES);
        default:
            break;
    }
    if (1U != p_request->length) {
        return 0U;
    }

    uint8_t value = p_request->p_payload[0U];
    switch (p_request->command) {
        case PROTO_CHECKPOINT_DEVICE_ID:
            return 1U;
        case PROTO_CHECKPOINT_PIR_ENABLE:
            return (uint8_t)((0x00U == value) || (0xFFU == value));
        case PROTO_CHECKPOINT_VOICE_REPEAT:
        case PROTO_CHECKPOINT_DISPLAY_REPEAT:
            return (uint8_t)(value <= REPEAT_MAX);
        case PROTO_CHECKPOINT_PLAY:
            return (uint8_t)(value <= ITEM_MAX);
        case PROTO_CHECKPOINT_PIR_ITEM:
            return (uint8_t)((0U != value) && (value <= ITEM_MAX));
        default:
            return 0U;
    }
}

uint8_t proto_checkpoint_init(proto_checkpoint_parser_t *p_parser,
                              uint8_t                    device_id,
                              uint8_t                   *p_payload,
                              uint16_t                   capacity) {
    if ((NULL == p_parser) || (NULL == p_payload) || (0U == capacity)) {
        return 0U;
    }

    *p_parser = (proto_checkpoint_parser_t){
        .p_payload = p_payload,
        .capacity  = capacity,
        .device_id = device_id,
        .state     = RECEIVE_ID,
    };
    return 1U;
}

void proto_checkpoint_reset(proto_checkpoint_parser_t *p_parser) {
    if (NULL != p_parser) {
        p_parser->state = RECEIVE_ID;
    }
}

proto_checkpoint_result_t
proto_checkpoint_feed(proto_checkpoint_parser_t  *p_parser,
                      uint8_t                     byte,
                      proto_checkpoint_request_t *p_request) {
    if ((NULL == p_parser) || (NULL == p_request) ||
        (NULL == p_parser->p_payload) || (0U == p_parser->capacity)) {
        return PROTO_CHECKPOINT_INVALID;
    }

    switch ((receive_state_t)p_parser->state) {
        case RECEIVE_ID:
            p_parser->id    = byte;
            p_parser->state = RECEIVE_COMMAND;
            break;
        case RECEIVE_COMMAND:
            if (0U == command_is_valid(byte)) {
                /* With no fixed start byte, this byte could be the next ID. */
                p_parser->id = byte;
                return PROTO_CHECKPOINT_INVALID;
            }
            p_parser->command = byte;
            p_parser->state   = RECEIVE_LENGTH_HIGH;
            break;
        case RECEIVE_LENGTH_HIGH:
            p_parser->length = (uint16_t)((uint16_t)byte << 8U);
            p_parser->state  = RECEIVE_LENGTH_LOW;
            break;
        case RECEIVE_LENGTH_LOW:
            p_parser->length |= byte;
            p_parser->received = 0U;
            p_parser->sum      = 0U;
            p_parser->state    = (0U == p_parser->length) ? RECEIVE_SUM_HIGH
                                                          : RECEIVE_PAYLOAD;
            break;
        case RECEIVE_PAYLOAD:
            /* Consume oversized frames without writing any of their payload. */
            if (p_parser->length <= p_parser->capacity) {
                p_parser->p_payload[p_parser->received] = byte;
            }
            p_parser->sum = (uint16_t)(p_parser->sum + byte);
            p_parser->received++;
            if (p_parser->received == p_parser->length) {
                p_parser->state = RECEIVE_SUM_HIGH;
            }
            break;
        case RECEIVE_SUM_HIGH:
            p_parser->wire_sum = (uint16_t)((uint16_t)byte << 8U);
            p_parser->state    = RECEIVE_SUM_LOW;
            break;
        case RECEIVE_SUM_LOW:
            p_parser->wire_sum |= byte;
            p_parser->state = RECEIVE_TAIL;
            break;
        case RECEIVE_TAIL:
            proto_checkpoint_reset(p_parser);
            return finish_request(p_parser, byte, p_request);
        default:
            proto_checkpoint_reset(p_parser);
            return PROTO_CHECKPOINT_INVALID;
    }
    return PROTO_CHECKPOINT_MORE;
}

size_t proto_checkpoint_encode_reply(uint8_t *p_reply,
                                     size_t   capacity,
                                     uint8_t  id,
                                     uint8_t  command,
                                     uint8_t  data) {
    if ((NULL == p_reply) || (capacity < PROTO_CHECKPOINT_REPLY_SIZE) ||
        (0U == command_is_valid(command))) {
        return 0U;
    }

    p_reply[0U] = id;
    p_reply[1U] = command;
    p_reply[2U] = 1U;
    p_reply[3U] = data;
    p_reply[4U] = 0U; /* The checksum sums the single data byte. */
    p_reply[5U] = data;
    p_reply[6U] = REPLY_TAIL;
    return PROTO_CHECKPOINT_REPLY_SIZE;
}
/* end of file --------------------------------------------------------------*/
