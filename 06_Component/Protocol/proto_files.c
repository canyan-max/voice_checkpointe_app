/**
 ******************************************************************************
 *@file               :   proto_files.c
 *@brief              :   Provide the HAL APIs of description.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>      /* stdint lib header file. */
#include "proto_files.h" /* proto_files lib header file. */
#include <string.h>      /* stdint lib header file. */
/* define   -----------------------------------------------------------------*/
#define FILE_NAME_LENGTH            (64U)
#define FILE_SIZE_LENGTH            ((uint32_t)16)
#define PROTO_FILES_HANDSHAKE_LEN   (7U)
#define PROTO_FILES_END_SESSION_LEN (3U)

// index
#define PACK_TYPE_INDEX             (0U)
#define PACK_RESERVER_INDEX         (1U)
#define PACK_PACK_NUMBER_INDEX      (2U)
#define PACK_UNPACK_NUMBER_INDEX    (3U)
#define PROTO_FILES_INFO_FRAME_LEN  (PACKET_DATA_INDEX + PACK_FRAME_SIZE_128B + 2U)
/* worst-case frame (1024B data pack): header + payload + 2B CRC */
#define PROTO_FILES_MAX_FRAME       (PACKET_DATA_INDEX + PACK_FRAME_SIZE_1K + 2U)

/* memset the parser back to IDLE, then re-point buf at its backing store
 * (memset also zeroes the pointer itself, so it must be restored after). */

#define PROTO_CLEAR(ptr) \
do { \
    memset((ptr), 0, sizeof(*(ptr))); \
    (ptr)->buf = g_file_buffer; \
    (ptr)->idx = 0; \
} while(0) //


#define PROTO_GET_ARR_PACK_TYPE(parser, arr) \
do { \
    (parser)->pack_type = (pack_type_frame_t)(arr)[PACK_TYPE_INDEX]; \
} while(0)

#define PROTO_GET_ARR_RESERVER(parser, arr) \
do { \
    (parser)->reserver = (arr)[PACK_RESERVER_INDEX]; \
} while(0)

#define PROTO_GET_ARR_PACK_NUM(parser, arr) \
do { \
    (parser)->package_number = (arr)[PACK_PACK_NUMBER_INDEX]; \
} while(0)

#define PROTO_GET_ARR_PACK_UN_NUM(parser, arr) \
do { \
    (parser)->package_un_numer = (arr)[PACK_UNPACK_NUMBER_INDEX]; \
} while(0)
/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/
static uint8_t g_file_name[FILE_NAME_LENGTH];
static uint8_t g_file_buffer[PROTO_FILES_MAX_FRAME]; /* proto_files_parser_t.buf backing store */
/* private  functions  ------------------------------------------------------*/
static uint16_t updata_crc16(uint16_t crc_in, uint8_t byte)
{
    uint32_t crc = crc_in;
    uint32_t in  = byte | 0x100;

    do
    {
        crc <<= 1;
        in <<= 1;
        if(in & 0x100)
            ++crc;
        if(crc & 0x10000)
            crc ^= 0x1021;
    }

    while(!(in & 0x10000));

    return crc & 0xffffu;
}

static uint16_t cal_crc16(const uint8_t *p_data, uint32_t size)
{
    uint32_t       crc     = 0;
    const uint8_t *dataEnd = p_data + size;

    while(p_data < dataEnd)
        crc = updata_crc16(crc, *p_data++);

    crc = updata_crc16(crc, 0);
    crc = updata_crc16(crc, 0);

    return crc & 0xffffu;
}

/* "ISREADY" */


/* Scan for the handshake byte-at-a-time instead of comparing a fixed 7-byte
 * window: a stream that loses alignment (e.g. leftover bytes after a mid-
 * frame error) must still be able to re-lock onto "ISREADY" no matter where
 * it lands, not only when it happens to start on a multiple-of-7 boundary. */
static proto_files_ret_t proto_files_feed_idle(proto_files_parser_t *p_parser,
                                               uint8_t               byte)
{
    static const uint8_t files_handshake[PROTO_FILES_HANDSHAKE_LEN] = {
    0x49U, 0x53U, 0x52U, 0x45U, 0x41U, 0x44U, 0x59U};
    if(byte != files_handshake[p_parser->idx])
    {
        /* mismatch: this byte might itself be the "I" that starts the real
         * handshake, so re-check it against position 0 instead of always
         * dropping back to an empty match. */
        p_parser->idx = (files_handshake[0] == byte) ? 1U : 0U;
        return PROTO_FILE_RET_OK;
    }

    p_parser->idx++;
    if(p_parser->idx < PROTO_FILES_HANDSHAKE_LEN)
    {
        return PROTO_FILE_RET_OK; /* still matching */
    }

    p_parser->idx   = 0U;
    p_parser->state = PROTO_FILE_RECEIVE_FILE_INFO;
    return PROTO_FILE_RET_OK;
}

/* Accumulate the fixed-size (134B) file-info frame, then validate + parse
 * it exactly like the previous whole-buffer version did. */
static proto_files_ret_t
proto_files_feed_file_info(proto_files_parser_t *p_parser, uint8_t byte)
{
    /* check head: a file-info frame must start with PACK_TYPE_128_FRAM (it
     * is always a 128B payload, unlike a data pack which can be 128B or 1K).
     * Any other first byte means the stream is misaligned (e.g. a stray
     * CR/LF tail left over after the handshake) — reject it immediately
     * instead of silently accumulating 134 bytes and only failing at the
     * PACKNUM/CRC check. Symmetric to proto_files_feed_data_info's head
     * check. */
    if(p_parser->idx == 0U)
    {
        if(PACK_TYPE_128_FRAM != (pack_type_frame_t)byte)
        {
            return PROTO_FILE_RET_HEAD_ERR;
        }
    }

    p_parser->buf[p_parser->idx] = byte;
    p_parser->idx++;

    if(p_parser->idx < PROTO_FILES_INFO_FRAME_LEN)
    {
        return PROTO_FILE_RET_OK; /* still accumulating */
    }

    // check file frame and crc code is valid
    uint8_t *data = p_parser->buf;
    PROTO_GET_ARR_PACK_TYPE(p_parser, data);
    uint8_t  file_pack_num    = data[2];
    uint8_t  file_pack_un_num = data[3];
    uint8_t *file_ptr         = NULL;
    uint8_t  file_size[FILE_SIZE_LENGTH];
    uint8_t  i           = 0;
    uint16_t packet_size = PACK_FRAME_SIZE_128B;
    uint32_t crc         = data[packet_size + PACKET_DATA_INDEX] << 8;
    crc += data[packet_size + PACKET_DATA_INDEX + 1];

    if(PACK_TYPE_128_FRAM != p_parser->pack_type || 0x5A != file_pack_num ||
       0xA5 != file_pack_un_num)
    {
        return PROTO_FILE_RET_PACKNUM_ERR;
    }
    if(cal_crc16(&data[PACKET_DATA_INDEX], packet_size) != crc)
    {
        return PROTO_FILE_RET_CRC_ERR;
    }

    file_ptr = data + PACKET_DATA_INDEX;
    /* loop bound is length-1: keeps one slot free for the trailing '\0', so
     * an over-long name is truncated (safe) instead of overflowing
     * g_file_name[64] by one byte. Same guard below for file_size[16]. */
    while((*file_ptr != 0) && (i < FILE_NAME_LENGTH - 1U))
    {
        g_file_name[i++] = *file_ptr++;
    }

    g_file_name[i++] = '\0';
    i                = 0;
    file_ptr++;
    while((*file_ptr != ' ') && (i < FILE_SIZE_LENGTH - 1U))
    {
        file_size[i++] = *file_ptr++;
    }
    file_size[i++] = '\0';

    // Str2Int(file_size, &filesize);

    p_parser->idx   = 0U;
    p_parser->state = PROTO_FILE_RECEIVE_DATA_INFO;

    return PROTO_FILE_RET_OK;
}

static proto_files_ret_t
proto_files_feed_session_end(proto_files_parser_t *p_parser, uint8_t byte)
{
    static const uint8_t files_end_session[PROTO_FILES_END_SESSION_LEN] = {
    0x45U, 0x53U, 0x43U};
    if(byte != files_end_session[p_parser->esc_idx])
    {
        /* expected 'E','S','C' but got something else mid-match: in
         * RECEIVE_DATA_INFO phase a byte that is neither a data-pack type
         * nor a valid ESC continuation means the stream is corrupted. Fail
         * fast back to IDLE (via the caller's PROTO_CLEAR) rather than
         * staying here — the host can re-send the session. */
        return PROTO_FILE_RET_ERR;
    }

    p_parser->esc_idx++;
    if(p_parser->esc_idx < PROTO_FILES_END_SESSION_LEN)
    {
        return PROTO_FILE_RET_OK; /* still matching */
    }

    /* full "ESC" matched: the session is over. Return to IDLE to await a
     * fresh handshake. This is a normal completion, not an error, so return
     * OK and reset here (otherwise proto_files_feed would log it as one). */
    PROTO_CLEAR(p_parser);
    return PROTO_FILE_RET_OK;
}
/* Accumulate one data pack. Its total length isn't known until the very
 * first byte (pack_type) arrives, so that byte decides frame_len before
 * we know how many more bytes to wait for. */
static proto_files_ret_t
proto_files_feed_data_info(proto_files_parser_t *p_parser, uint8_t byte)
{

    // check head
    if(p_parser->idx == 0U)
    {
        switch(byte)
        {
            case PACK_TYPE_128_FRAM:
            {
                break;
            }
            case PACK_TYPE_1024_FRAM:
            {
                break;
            }
            default: /* not a valid pack type: could be the "ESC" end-of-
                        session marker, so switch into the ESC-matching state
                        and let it consume this same byte. */
            {
                p_parser->state   = PROTO_FILE_RECEIVE_END_SESSION;
                p_parser->esc_idx = 0U;
                return proto_files_feed_session_end(p_parser, byte);
            }
        }
        p_parser->pack_type = (pack_type_frame_t)byte;
        p_parser->frame_len = PACKET_DATA_INDEX + 2U +
                              ((PACK_TYPE_128_FRAM == p_parser->pack_type)
                                   ? PACK_FRAME_SIZE_128B
                                   : PACK_FRAME_SIZE_1K);
    }

    p_parser->buf[p_parser->idx] = byte;
    p_parser->idx++;
    if(p_parser->idx < p_parser->frame_len)
    {
        return PROTO_FILE_RET_OK; /* still accumulating */
    }

    uint8_t *data                    = p_parser->buf;
    uint8_t  master_send_pack_num    = data[2];
    uint8_t  master_send_pack_un_num = data[3];
    uint8_t  self_next_pack_num      = p_parser->package_number + 1;
    uint8_t  self_next_pack_un_num   = ~self_next_pack_num;
    uint16_t packet_size = (PACK_TYPE_128_FRAM == p_parser->pack_type)
                               ? PACK_FRAME_SIZE_128B
                               : PACK_FRAME_SIZE_1K;
    uint32_t crc         = 0U;

    if(master_send_pack_num != self_next_pack_num ||
       master_send_pack_un_num != self_next_pack_un_num)
    {
        return PROTO_FILE_RET_PACKNUM_ERR;
    }

    crc = data[packet_size + PACKET_DATA_INDEX] << 8;
    crc += data[packet_size + PACKET_DATA_INDEX + 1];
    if(cal_crc16(&data[PACKET_DATA_INDEX], packet_size) != crc)
    {
        return PROTO_FILE_RET_CRC_ERR;
    }

    PROTO_GET_ARR_RESERVER(p_parser, data);
    PROTO_GET_ARR_PACK_NUM(p_parser, data);
    PROTO_GET_ARR_PACK_UN_NUM(p_parser, data);
    // save data

    p_parser->idx = 0U;
    return PROTO_FILE_RET_OK;
}
/* exported functions -------------------------------------------------------*/

proto_files_ret_t proto_files_init(proto_files_parser_t *p_parser)
{
    if(NULL == p_parser)
    {
        return PROTO_FILE_RET_ERR;
    }
    PROTO_CLEAR(p_parser);
    return PROTO_FILE_RET_OK;
}

proto_files_ret_t proto_files_feed(proto_files_parser_t *p_parser, uint8_t byte)
{
    if(NULL == p_parser)
    {
        return PROTO_FILE_RET_ERR;
    }
    proto_files_ret_t ret = PROTO_FILE_RET_OK;

    switch(p_parser->state)
    {
        case PROTO_FILE_IDLE:
        {
            ret = proto_files_feed_idle(p_parser, byte);
            break;
        }

        case PROTO_FILE_RECEIVE_FILE_INFO:
        {
            ret = proto_files_feed_file_info(p_parser, byte);
            break;
        }

        case PROTO_FILE_RECEIVE_DATA_INFO:
        {
            ret = proto_files_feed_data_info(p_parser, byte);
            break;
        }

        case PROTO_FILE_RECEIVE_END_SESSION:
        {
            ret = proto_files_feed_session_end(p_parser, byte);
            break;
        }

        default:
        {
            ret = PROTO_FILE_RET_ERR;
            break;
        }
    }

    if(PROTO_FILE_RET_OK != ret)
    {
        PROTO_CLEAR(p_parser);
    }
    return ret;
}

/* end of  file -------------------------------------------------------------*/
