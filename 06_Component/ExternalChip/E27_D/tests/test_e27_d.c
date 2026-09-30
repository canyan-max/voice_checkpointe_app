/* Host tests: gcc -std=c11 -Wall -Wextra -Werror -pedantic
 * ../e27_d.c test_e27_d.c -o test_e27_d.exe */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../e27_d.h"

typedef struct
{
    e27_d_device_t device;
    char           tx[384];
    uint16_t       tx_size;
    e27_d_ret_t    tx_result;
    unsigned int completed, ready, link, overflow, identity, connections, dtuid;
    e27_d_ret_t  last_result;
    char         text[257];
    uint16_t     value, auxiliary;
    uint8_t      channel;
} fixture_t;

static e27_d_ret_t send_mock(void          *context,
                             const uint8_t *data,
                             uint16_t       size,
                             uint32_t       timeout_ms)
{
    fixture_t *f = context;
    assert(timeout_ms > 0U);
    assert(size < sizeof(f->tx));
    memcpy(f->tx, data, size);
    f->tx[size] = '\0';
    f->tx_size  = size;
    return f->tx_result;
}

static void event_mock(void *context, const e27_d_event_t *event)
{
    fixture_t *f = context;
    /* Sending from callbacks would associate trailing bytes with a new
     * command; all mutating APIs must reject reentrant access. */
    assert(E27_D_RET_BUSY == e27_d_tick(&f->device, 0U));
    switch(event->kind)
    {
        case E27_D_EVENT_COMPLETE:
            ++f->completed;
            f->last_result = event->result;
            break;
        case E27_D_EVENT_READY:
            ++f->ready;
            break;
        case E27_D_EVENT_LINK:
            ++f->link;
            f->value   = event->value;
            f->channel = event->channel;
            break;
        case E27_D_EVENT_OVERFLOW:
            ++f->overflow;
            break;
        case E27_D_EVENT_IMEI:
        case E27_D_EVENT_ICCID:
        case E27_D_EVENT_IMSI:
            ++f->identity;
            assert(event->size < sizeof(f->text));
            memcpy(f->text, event->p_text, event->size);
            f->text[event->size] = '\0';
            break;
        case E27_D_EVENT_CONNECTIONS:
            ++f->connections;
            f->value     = event->value;
            f->auxiliary = event->auxiliary;
            break;
        case E27_D_EVENT_DTUID:
            ++f->dtuid;
            f->channel = event->channel;
            break;
        default:
            break;
    }
}

static void init(fixture_t *f)
{
    e27_d_io_t io = {send_mock, event_mock, f};
    memset(f, 0, sizeof(*f));
    assert(E27_D_RET_OK == e27_d_init(&f->device, &io));
}

static e27_d_ret_t start(fixture_t *f, e27_d_command_t command, uint32_t now)
{
    e27_d_request_t r = {0};
    r.command         = command;
    return e27_d_command_start(&f->device, &r, now, 10U, 100U);
}

static void feed(fixture_t *f, const char *text)
{
    assert(E27_D_RET_OK == e27_d_response_process(&f->device,
                                                  (const uint8_t *)text,
                                                  (uint16_t)strlen(text)));
}

static void enter(fixture_t *f)
{
    assert(E27_D_RET_OK == start(f, E27_D_CMD_ENTER, 0U));
    assert(3U == f->tx_size);
    assert(0 == strcmp(f->tx, "+++"));
    feed(f, "\r\nOK\r\n");
    assert(E27_D_MODE_COMMAND == f->device.mode);
}

static void framing_and_parameters(void)
{
    fixture_t       f;
    e27_d_request_t r = {0};
    uint8_t         payload[256];
    init(&f);
    assert(E27_D_RET_NOT_READY == start(&f, E27_D_CMD_ICCID, 0U));
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_CPIN, 1U));
    assert(0 == strcmp(f.tx, "AT+CPIN\r\n"));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_ICCID, 2U));
    assert(0 == strcmp(f.tx, "AT+ICCID\r\n"));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_IMEI, 3U));
    assert(0 == strcmp(f.tx, "AT+GSN\r\n"));
    feed(&f, "OK\r\n");
    r.command             = E27_D_CMD_SET_DTUID;
    r.args.packet.mode    = 1U;
    r.args.packet.channel = 1U;
    r.args.packet.p_text  = (const uint8_t *)"kvt9dxr84qrryr1z";
    r.args.packet.size    = 16U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 4U, 10U, 100U));
    assert(0 == strcmp(f.tx, "AT+DTUID=1,0,0,\"kvt9dxr84qrryr1z\",1\r\n"));
    feed(&f, "OK\r\n");
    r.args.packet.p_text = (const uint8_t *)"x\"\r\nAT&W";
    r.args.packet.size   = 9U;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 5U, 10U, 100U));
    memset(payload, 'A', sizeof(payload));
    r.args.packet.p_text = payload;
    r.args.packet.size   = 256U;
    r.args.packet.format = 1U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 6U, 10U, 100U));
    assert(0 == memcmp(f.tx, "AT+DTUID=1,0,1,\"", 16U));
    feed(&f, "OK\r\n");
    r.args.packet.size = 255U;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 7U, 10U, 100U));
    r.command = E27_D_CMD_COUNT;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 7U, 10U, 100U));
    r.command = E27_D_CMD_AT;
    assert(E27_D_RET_PARAM == e27_d_command_start(&f.device, &r, 7U, 0U, 100U));
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 7U, 101U, 100U));
}

static void streaming_and_events(void)
{
    const char
          *reply = "\r\n865501042107814\r\nOK\r\n+STATUS: 2, CONNECTED\r\n";
    size_t split;
    for(split = 0U; split <= strlen(reply); ++split)
    {
        fixture_t f;
        init(&f);
        enter(&f);
        assert(E27_D_RET_OK == start(&f, E27_D_CMD_IMEI, 1U));
        assert(E27_D_RET_BUSY == start(&f, E27_D_CMD_EXIT, 2U));
        assert(E27_D_RET_OK == e27_d_response_process(&f.device,
                                                      (const uint8_t *)reply,
                                                      (uint16_t)split));
        feed(&f, reply + split);
        assert(2U == f.completed && E27_D_RET_OK == f.last_result);
        assert(1U == f.identity && 0 == strcmp(f.text, "865501042107814"));
        assert(1U == f.link && 2U == f.channel && 1U == f.value);
    }
    {
        fixture_t f;
        init(&f);
        enter(&f);
        assert(E27_D_RET_OK == start(&f, E27_D_CMD_CONNECT_QUERY, 1U));
        feed(&f, "+ASKCONNECT: 0,1\r\nOK\r\n");
        assert(1U == f.connections && 0U == f.value && 1U == f.auxiliary);
        assert(E27_D_RET_OK == start(&f, E27_D_CMD_DTUID_QUERY, 2U));
        feed(&f,
             "+DTUID: 1,0,0,\"id1\",1\r\n+DTUID: 1,0,1,\"4142\",2\r\nOK\r\n");
        assert(2U == f.dtuid && 2U == f.channel);
        assert(E27_D_RET_OK == start(&f, E27_D_CMD_EXIT, 3U));
        feed(&f, "OK\r\n+STATUS: 1, CLOSED\r\n");
        assert(E27_D_MODE_TRANSPARENT == f.device.mode && 1U == f.link);
    }
}

static void failure_and_recovery(void)
{
    fixture_t f;
    uint8_t   oversized[E27_D_LINE_CAPACITY + 10U];
    init(&f);
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_CPIN, 1U));
    feed(&f, "+CME ERROR (no SIM)\r\n");
    assert(E27_D_RET_REJECTED == f.last_result);
    assert(E27_D_MODE_UNKNOWN == f.device.mode);
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_RESTART, 1U));
    feed(&f, "AT Ready\r\n");
    assert(E27_D_RET_RESET == f.last_result && 1U == f.ready);
    assert(E27_D_MODE_TRANSPARENT == f.device.mode);
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_ICCID, 1U));
    feed(&f, "AT Ready\r\n");
    assert(E27_D_RET_RESET == f.last_result && 2U == f.ready);
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_ALL_QUERY, 1U));
    memset(oversized, 'A', sizeof(oversized));
    assert(E27_D_RET_OVERFLOW ==
           e27_d_response_process(&f.device, oversized, sizeof(oversized)));
    feed(&f, "OK\r\n+STATUS: 1, CONNECTED\r\n");
    assert(1U == f.overflow && E27_D_RET_OVERFLOW == f.last_result &&
           1U == f.link);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_ENTER, 0xFFFFFFE0U));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_AT, 0xFFFFFFF0U));
    assert(E27_D_RET_OK == e27_d_tick(&f.device, 0x53U));
    assert(E27_D_RET_TIMEOUT == e27_d_tick(&f.device, 0x54U));
    assert(E27_D_RET_TIMEOUT == f.last_result);
    feed(&f, "OK\r\n"); /* late reply cannot complete the failed transaction */
    f.tx_result = E27_D_RET_IO;
    assert(E27_D_RET_IO == start(&f, E27_D_CMD_ENTER, 1U));
    assert(0U == f.device.pending && E27_D_MODE_UNKNOWN == f.device.mode);
    f.tx_result = E27_D_RET_OK;
    enter(&f);
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_CPIN, 1U));
    assert(E27_D_RET_OK == e27_d_transport_lost(&f.device));
    assert(E27_D_RET_IO == f.last_result &&
           E27_D_MODE_UNKNOWN == f.device.mode);
}

static void settings_and_expiry(void)
{
    fixture_t       f;
    e27_d_request_t r = {0};
    e27_d_io_t      io;
    init(&f);
    enter(&f);
    r.command             = E27_D_CMD_SET_SERVER;
    r.args.server.channel = 1U;
    r.args.server.p_host  = (const uint8_t *)"example.com";
    r.args.server.size    = 11U;
    r.args.server.port    = 15000U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    assert(0 == strcmp(f.tx, "AT+DSCADDR=1,\"TCP\",\"example.com\",15000\r\n"));
    io = f.device.io;
    assert(E27_D_RET_BUSY == e27_d_init(&f.device, &io));
    feed(&f, "OK\r\n");
    r.args.server.port = 0U;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    r.command              = E27_D_CMD_SET_MODE;
    r.args.channel.value   = 9U;
    r.args.channel.channel = 1U;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    r.args.channel.value = 8U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    assert(0 == strcmp(f.tx, "AT+DTUMODE=8,1\r\n"));
    feed(&f, "OK\r\n");
    r.command              = E27_D_CMD_SET_HEX;
    r.args.channel.value   = 1U;
    r.args.channel.channel = 3U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    assert(0 == strcmp(f.tx, "AT+TCPHEX=1,3\r\n"));
    feed(&f, "OK\r\n");
    r.command              = E27_D_CMD_SET_AUTOATO;
    r.args.autoato_seconds = 59U;
    assert(E27_D_RET_PARAM ==
           e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    r.args.autoato_seconds = 120U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == start(&f, E27_D_CMD_ENTER, 119000U));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == e27_d_tick(&f.device, 120000U));
    assert(E27_D_MODE_UNKNOWN ==
           f.device.mode); /* repeated +++ did not refresh */
    enter(&f);
    feed(&f, "+STATUS: 1, CONNE");
    assert(E27_D_RET_BUSY == start(&f, E27_D_CMD_AT, 1U));
    feed(&f, "CTED\r\n");
    assert(1U == f.link);
    r.args.autoato_seconds = 0U;
    assert(E27_D_RET_OK == e27_d_command_start(&f.device, &r, 1U, 10U, 100U));
    feed(&f, "OK\r\n");
    assert(E27_D_RET_OK == e27_d_tick(&f.device, 300000U));
    assert(E27_D_MODE_COMMAND == f.device.mode);
}

int main(void)
{
    framing_and_parameters();
    streaming_and_events();
    failure_and_recovery();
    settings_and_expiry();
    puts("E27-D host tests passed");
    return 0;
}
