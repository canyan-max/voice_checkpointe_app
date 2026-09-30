/** Portable E27-D AT protocol driver (AT manual V1.0.1).
 * No allocation, board resources, RTOS, logging or business configuration.
 */
#ifndef E27_D_H
#define E27_D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define E27_D_TEXT_MAX_SIZE 256U
#define E27_D_LINE_CAPACITY 384U

typedef enum
{
    E27_D_RET_OK = 0,
    E27_D_RET_PARAM,
    E27_D_RET_IO,
    E27_D_RET_TIMEOUT,
    E27_D_RET_BUSY,
    E27_D_RET_NOT_READY,
    E27_D_RET_REJECTED,
    E27_D_RET_OVERFLOW,
    E27_D_RET_RESET
} e27_d_ret_t;

/* Last confirmed mode. UNKNOWN requires a fresh ENTER transaction. */
typedef enum
{
    E27_D_MODE_UNKNOWN = 0,
    E27_D_MODE_TRANSPARENT,
    E27_D_MODE_COMMAND,
    E27_D_MODE_REBOOTING
} e27_d_mode_t;

typedef enum
{
    E27_D_CMD_ENTER = 0,
    E27_D_CMD_EXIT,
    E27_D_CMD_AT,
    E27_D_CMD_SAVE,
    E27_D_CMD_RESTART,
    E27_D_CMD_CPIN,
    E27_D_CMD_CSQ,
    E27_D_CMD_IMEI,
    E27_D_CMD_ICCID,
    E27_D_CMD_IMSI,
    E27_D_CMD_DTUID_QUERY,
    E27_D_CMD_CONNECT_QUERY,
    E27_D_CMD_MODE_QUERY,
    E27_D_CMD_SERVER_QUERY,
    E27_D_CMD_ALL_QUERY,
    /* Existing board-verified command; absent from manual V1.0.1. */
    E27_D_CMD_DISABLE_SECURITY,
    E27_D_CMD_SET_MODE,
    E27_D_CMD_SET_MODBUS,
    E27_D_CMD_SET_HEX,
    E27_D_CMD_SET_DTUID,
    E27_D_CMD_SET_KEEPALIVE,
    E27_D_CMD_SET_SERVER,
    E27_D_CMD_SET_SECONDARY,
    E27_D_CMD_SET_AUTOSTATUS,
    E27_D_CMD_SET_AUTOATO,
    E27_D_CMD_COUNT
} e27_d_command_t;

typedef struct
{
    e27_d_command_t command;
    union
    {
        struct
        {
            uint8_t value;
            uint8_t channel;
        } channel;
        struct
        {
            uint8_t        mode;     /* DTUID: 0..3 */
            uint8_t        content;  /* DTUID: custom=0, IMEI=1, ICCID=2 */
            uint8_t        format;   /* ASCII=0, HEX=1 */
            uint8_t        channel;  /* 1..2 */
            uint16_t       interval; /* KEEPALIVE: seconds, 0 disables */
            const uint8_t *p_text;
            uint16_t       size; /* ASCII <=128, HEX <=256 and even */
        } packet;
        struct
        {
            uint8_t        channel; /* 1..2 */
            uint8_t        udp;     /* TCP=0, UDP=1 */
            const uint8_t *p_host;
            uint16_t       size; /* 1..128; empty address not supported */
            uint16_t       port; /* 1..65535 */
        } server;
        struct
        {
            uint8_t channel1;
            uint8_t channel2;
        } secondary;
        struct
        {
            uint8_t level;
            uint8_t boot_report;
        } autostatus;
        uint16_t autoato_seconds; /* 0 disables; otherwise 60..65535 */
    } args;
} e27_d_request_t;

typedef enum
{
    E27_D_EVENT_LINE = 0,
    E27_D_EVENT_COMPLETE,
    E27_D_EVENT_READY,
    E27_D_EVENT_LINK,
    E27_D_EVENT_REGISTRATION,
    E27_D_EVENT_IMEI,
    E27_D_EVENT_ICCID,
    E27_D_EVENT_IMSI,
    E27_D_EVENT_SIM_READY,
    E27_D_EVENT_SIGNAL,
    E27_D_EVENT_CONNECTIONS,
    E27_D_EVENT_DTUID,
    E27_D_EVENT_OVERFLOW
} e27_d_event_kind_t;

typedef struct
{
    e27_d_event_kind_t kind;
    e27_d_command_t    command; /* COUNT means unsolicited/no transaction */
    e27_d_ret_t        result;  /* COMPLETE only */
    const char        *p_text;
    uint16_t           size;
    uint16_t           value; /* RSSI, registration/link state, DTUID mode */
    uint16_t           auxiliary; /* BER, second connection, DTUID content */
    uint8_t            channel;
    uint8_t            format;
} e27_d_event_t;

typedef e27_d_ret_t (*e27_d_transmit_t)(void          *p_context,
                                        const uint8_t *p_data,
                                        uint16_t       size,
                                        uint32_t       timeout_ms);
typedef void (*e27_d_event_callback_t)(void                *p_context,
                                       const e27_d_event_t *p_event);
typedef struct
{
    e27_d_transmit_t       transmit;
    e27_d_event_callback_t event;
    void                  *p_context;
} e27_d_io_t;

/* Caller-owned, zero-initialize before init; one serialized owner per UART.
 * Parser storage belongs to the device; no secondary RX FIFO is allocated.
 */
typedef struct
{
    e27_d_io_t      io;
    char            line[E27_D_LINE_CAPACITY];
    uint32_t        start_ms;
    uint32_t        reply_timeout_ms;
    uint32_t        mode_start_ms;
    uint16_t        autoato_seconds;
    uint16_t        pending_autoato_seconds;
    uint16_t        line_size;
    e27_d_command_t command;
    e27_d_mode_t    mode;
    uint8_t         initialized;
    uint8_t         pending;
    uint8_t         discarding;
    uint8_t         processing;
} e27_d_device_t;

e27_d_ret_t e27_d_init(e27_d_device_t *p_device, const e27_d_io_t *p_io);
/* Synchronous bounded TX, asynchronous reply. Timeouts are supplied by the
 * caller, not specified by the manual; each must be 1..INT32_MAX ms.
 * now_ms is sampled before TX; the reply budget includes TX time.
 * ENTER is exactly "+++"; all other commands end with CR LF.
 * Request strings are consumed during this call and never retained.
 */
e27_d_ret_t e27_d_command_start(e27_d_device_t        *p_device,
                                const e27_d_request_t *p_request,
                                uint32_t               now_ms,
                                uint32_t               send_timeout_ms,
                                uint32_t               reply_timeout_ms);
/* Feed every received byte before tick. All bytes are consumed, including
 * URCs after a terminal reply. Callbacks must not reenter any driver API.
 * Event text is borrowed until callback return. Query events are provisional
 * until COMPLETE/OK; discard them after any other completion.
 * Transparent business payload must be routed by the owner, not fed blindly
 * to this text parser (payload may look like an AT status line).
 */
e27_d_ret_t e27_d_response_process(e27_d_device_t *p_device,
                                   const uint8_t  *p_data,
                                   uint16_t        size);
e27_d_ret_t e27_d_tick(e27_d_device_t *p_device, uint32_t now_ms);
/* Call after transport data loss/error; pending command fails and mode becomes
 * UNKNOWN. Owner must drain stale transport data before re-entering AT mode.
 */
e27_d_ret_t e27_d_transport_lost(e27_d_device_t *p_device);

#ifdef __cplusplus
}
#endif
#endif
