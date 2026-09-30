/** E27-D protocol framing and streaming response parser. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "e27_d.h"

#define E27_D_COMMAND_CAPACITY 320U
#define E27_D_TIME_MAX 0x7FFFFFFFUL

static const char *const fixed_commands[] = {"+++",
                                             "ATO\r\n",
                                             "AT\r\n",
                                             "AT&W\r\n",
                                             "AT+CFUN=1,1\r\n",
                                             "AT+CPIN\r\n",
                                             "AT+CSQ\r\n",
                                             "AT+GSN\r\n",
                                             "AT+ICCID\r\n",
                                             "AT+IMSI\r\n",
                                             "AT+DTUID?\r\n",
                                             "AT+ASKCONNECT?\r\n",
                                             "AT+DTUMODE?\r\n",
                                             "AT+DSCADDR?\r\n",
                                             "AT+ALL?\r\n",
                                             "AT+MQTTSSL=0,1\r\n"};
_Static_assert(sizeof(fixed_commands) / sizeof(fixed_commands[0]) ==
                   E27_D_CMD_SET_MODE,
               "E27-D fixed command table mismatch");
_Static_assert(E27_D_LINE_CAPACITY > E27_D_TEXT_MAX_SIZE + 32U,
               "E27-D line storage must fit a full HEX registration reply");

static e27_d_ret_t device_check(const e27_d_device_t *d)
{
    if(NULL == d)
    {
        return E27_D_RET_PARAM;
    }
    if((0U == d->initialized) || (NULL == d->io.transmit) ||
       (NULL == d->io.event))
    {
        return E27_D_RET_NOT_READY;
    }
    if(0U != d->processing)
    {
        return E27_D_RET_BUSY;
    }
    return E27_D_RET_OK;
}

static void emit(e27_d_device_t *d, e27_d_event_t *e)
{
    d->io.event(d->io.p_context, e);
}

static void complete(e27_d_device_t *d, e27_d_ret_t result)
{
    e27_d_event_t e = {0};
    e.kind          = E27_D_EVENT_COMPLETE;
    e.command       = d->command;
    e.result        = result;
    d->pending      = 0U;
    if(E27_D_RET_OK != result)
    {
        d->mode = E27_D_MODE_UNKNOWN;
    }
    emit(d, &e);
}

/* Reject embedded quote/CR/LF and non-printable ASCII before framing. */
static uint8_t text_valid(const uint8_t *text, uint16_t size, uint8_t hex)
{
    uint16_t i;
    if((hex > 1U) || (size > ((0U == hex) ? 128U : 256U)) ||
       ((0U != size) && (NULL == text)) || ((0U != hex) && (0U != (size & 1U))))
    {
        return 0U;
    }
    for(i = 0U; i < size; ++i)
    {
        uint8_t c = text[i];
        if(0U != hex)
        {
            if(!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) ||
                 ((c >= 'A') && (c <= 'F'))))
            {
                return 0U;
            }
        }
        else if((c < 0x20U) || (c > 0x7EU) || ('"' == c))
        {
            return 0U;
        }
    }
    return 1U;
}

static e27_d_ret_t
command_build(const e27_d_request_t *r, char *out, uint16_t *size)
{
    int         n = -1;
    const char *text;
    if((NULL == r) || ((unsigned int)r->command >= E27_D_CMD_COUNT))
    {
        return E27_D_RET_PARAM;
    }
    if(r->command < E27_D_CMD_SET_MODE)
    {
        *size = (uint16_t)strlen(fixed_commands[r->command]);
        memcpy(out, fixed_commands[r->command], *size);
        return E27_D_RET_OK;
    }
    switch(r->command)
    {
        case E27_D_CMD_SET_MODE:
        case E27_D_CMD_SET_MODBUS:
        case E27_D_CMD_SET_HEX:
            if((r->args.channel.value >
                ((E27_D_CMD_SET_MODE == r->command) ? 8U : 1U)) ||
               (r->args.channel.channel < 1U) ||
               (r->args.channel.channel >
                ((E27_D_CMD_SET_HEX == r->command) ? 3U : 2U)))
            {
                return E27_D_RET_PARAM;
            }
            text = (E27_D_CMD_SET_MODE == r->command)
                       ? "DTUMODE"
                       : ((E27_D_CMD_SET_MODBUS == r->command) ? "TCPMODBUS"
                                                               : "TCPHEX");
            n = snprintf(out, E27_D_COMMAND_CAPACITY, "AT+%s=%u,%u\r\n", text,
                         (unsigned int)r->args.channel.value,
                         (unsigned int)r->args.channel.channel);
            break;
        case E27_D_CMD_SET_DTUID:
        case E27_D_CMD_SET_KEEPALIVE:
            if((r->args.packet.channel < 1U) || (r->args.packet.channel > 2U) ||
               (0U == text_valid(r->args.packet.p_text, r->args.packet.size,
                                 r->args.packet.format)))
            {
                return E27_D_RET_PARAM;
            }
            text = (0U == r->args.packet.size)
                       ? ""
                       : (const char *)r->args.packet.p_text;
            if(E27_D_CMD_SET_DTUID == r->command)
            {
                if((r->args.packet.mode > 3U) || (r->args.packet.content > 2U))
                {
                    return E27_D_RET_PARAM;
                }
                n = snprintf(out, E27_D_COMMAND_CAPACITY,
                             "AT+DTUID=%u,%u,%u,\"%.*s\",%u\r\n",
                             (unsigned int)r->args.packet.mode,
                             (unsigned int)r->args.packet.content,
                             (unsigned int)r->args.packet.format,
                             (int)r->args.packet.size, text,
                             (unsigned int)r->args.packet.channel);
            }
            else
            {
                n = snprintf(out, E27_D_COMMAND_CAPACITY,
                             "AT+KEEPALIVE=%u,%u,\"%.*s\",%u\r\n",
                             (unsigned int)r->args.packet.interval,
                             (unsigned int)r->args.packet.format,
                             (int)r->args.packet.size, text,
                             (unsigned int)r->args.packet.channel);
            }
            break;
        case E27_D_CMD_SET_SERVER:
            if((r->args.server.channel < 1U) || (r->args.server.channel > 2U) ||
               (r->args.server.udp > 1U) || (0U == r->args.server.port) ||
               (0U == r->args.server.size) ||
               (0U ==
                text_valid(r->args.server.p_host, r->args.server.size, 0U)))
            {
                return E27_D_RET_PARAM;
            }
            n = snprintf(out, E27_D_COMMAND_CAPACITY,
                         "AT+DSCADDR=%u,\"%s\",\"%.*s\",%u\r\n",
                         (unsigned int)r->args.server.channel,
                         (0U != r->args.server.udp) ? "UDP" : "TCP",
                         (int)r->args.server.size,
                         (const char *)r->args.server.p_host,
                         (unsigned int)r->args.server.port);
            break;
        case E27_D_CMD_SET_SECONDARY:
            if((r->args.secondary.channel1 > 1U) ||
               (r->args.secondary.channel2 > 1U))
            {
                return E27_D_RET_PARAM;
            }
            n = snprintf(out, E27_D_COMMAND_CAPACITY, "AT+SECSERVER=%u,%u\r\n",
                         (unsigned int)r->args.secondary.channel1,
                         (unsigned int)r->args.secondary.channel2);
            break;
        case E27_D_CMD_SET_AUTOSTATUS:
            if((r->args.autostatus.level > 3U) ||
               (r->args.autostatus.boot_report > 1U))
            {
                return E27_D_RET_PARAM;
            }
            n = snprintf(out, E27_D_COMMAND_CAPACITY, "AT+AUTOSTATUS=%u,%u\r\n",
                         (unsigned int)r->args.autostatus.level,
                         (unsigned int)r->args.autostatus.boot_report);
            break;
        case E27_D_CMD_SET_AUTOATO:
            if((0U != r->args.autoato_seconds) &&
               (r->args.autoato_seconds < 60U))
            {
                return E27_D_RET_PARAM;
            }
            n = snprintf(out, E27_D_COMMAND_CAPACITY, "AT+AUTOATO=%u\r\n",
                         (unsigned int)r->args.autoato_seconds);
            break;
        default:
            return E27_D_RET_PARAM;
    }
    if((n < 0) || ((unsigned int)n >= E27_D_COMMAND_CAPACITY))
    {
        return E27_D_RET_OVERFLOW;
    }
    *size = (uint16_t)n;
    return E27_D_RET_OK;
}

static uint8_t number(const char **cursor, uint16_t max, uint16_t *value)
{
    const char *p = *cursor;
    uint32_t    n = 0U;
    if((*p < '0') || (*p > '9'))
    {
        return 0U;
    }
    while((*p >= '0') && (*p <= '9'))
    {
        n = n * 10U + (uint32_t)(*p - '0');
        if(n > max)
        {
            return 0U;
        }
        ++p;
    }
    *cursor = p;
    *value  = (uint16_t)n;
    return 1U;
}

static uint8_t digits(const char *p, uint16_t size)
{
    uint16_t i;
    if(strlen(p) != size)
    {
        return 0U;
    }
    for(i = 0U; i < size; ++i)
    {
        if((p[i] < '0') || (p[i] > '9'))
        {
            return 0U;
        }
    }
    return 1U;
}

static uint8_t pair(const char *p, uint16_t max, e27_d_event_t *e)
{
    if((0U == number(&p, max, &e->value)) || (',' != *p))
    {
        return 0U;
    }
    ++p;
    return (uint8_t)((0U != number(&p, max, &e->auxiliary)) && ('\0' == *p));
}

static uint8_t dtuid(const char *p, e27_d_event_t *e)
{
    uint16_t    format, channel;
    const char *end;
    if((0U == number(&p, 3U, &e->value)) || (',' != *p))
    {
        return 0U;
    }
    ++p;
    if((0U == number(&p, 2U, &e->auxiliary)) || (',' != *p))
    {
        return 0U;
    }
    ++p;
    if((0U == number(&p, 1U, &format)) || (',' != *p))
    {
        return 0U;
    }
    ++p;
    if('"' != *p)
    {
        return 0U;
    }
    e->p_text = ++p;
    end       = strchr(p, '"');
    if(NULL == end)
    {
        return 0U;
    }
    e->size = (uint16_t)(end - p);
    if(0U == text_valid((const uint8_t *)p, e->size, (uint8_t)format))
    {
        return 0U;
    }
    p = end + 1;
    if(',' != *p)
    {
        return 0U;
    }
    ++p;
    if((0U == number(&p, 2U, &channel)) || (0U == channel) || ('\0' != *p))
    {
        return 0U;
    }
    e->channel = (uint8_t)channel;
    e->format  = (uint8_t)format;
    e->kind    = E27_D_EVENT_DTUID;
    return 1U;
}

static void line_process(e27_d_device_t *d)
{
    e27_d_event_t e = {0};
    const char   *p = d->line;
    if(0U == d->line_size)
    {
        return;
    }
    d->line[d->line_size] = '\0';
    e.command             = (0U != d->pending) ? d->command : E27_D_CMD_COUNT;
    e.p_text              = p;
    e.size                = d->line_size;
    if(0 == strcmp(p, "AT Ready"))
    {
        if(0U != d->pending)
        {
            /* Boot notification cannot substitute for command OK. */
            complete(d, E27_D_RET_RESET);
        }
        d->mode = E27_D_MODE_TRANSPARENT;
        e.kind  = E27_D_EVENT_READY;
    }
    else if((0U != d->pending) && (0 == strcmp(p, "OK")))
    {
        if(E27_D_CMD_ENTER == d->command)
        {
            /* Manual only promises OK for repeated +++; it does not promise
             * that repeated entry restarts the automatic exit countdown. */
            if(E27_D_MODE_COMMAND != d->mode)
            {
                d->mode_start_ms = d->start_ms;
            }
            d->mode = E27_D_MODE_COMMAND;
        }
        else if(E27_D_CMD_EXIT == d->command)
        {
            d->mode = E27_D_MODE_TRANSPARENT;
        }
        else if(E27_D_CMD_RESTART == d->command)
        {
            d->mode = E27_D_MODE_REBOOTING;
        }
        else if(E27_D_CMD_SET_AUTOATO == d->command)
        {
            d->autoato_seconds = d->pending_autoato_seconds;
        }
        complete(d, E27_D_RET_OK);
        return;
    }
    else if((0U != d->pending) &&
            ((0 == strcmp(p, "ERROR")) ||
             ((0 == strncmp(p, "+CME ERROR", 10U)) &&
              (('\0' == p[10]) || (':' == p[10]) || (' ' == p[10]))) ||
             ((0 == strncmp(p, "+CMS ERROR", 10U)) &&
              (('\0' == p[10]) || (':' == p[10]) || (' ' == p[10])))))
    {
        complete(d, E27_D_RET_REJECTED);
        return;
    }
    else if(0 == strncmp(p, "+STATUS: ", 9U))
    {
        uint16_t    channel;
        const char *cursor = p + 9U;
        if((0U != number(&cursor, 2U, &channel)) && (0U != channel) &&
           (0 == strncmp(cursor, ", ", 2U)))
        {
            cursor += 2U;
            if((0 == strcmp(cursor, "CONNECTED")) ||
               (0 == strcmp(cursor, "CLOSED")))
            {
                e.kind    = E27_D_EVENT_LINK;
                e.channel = (uint8_t)channel;
                e.value   = (uint16_t)(0 == strcmp(cursor, "CONNECTED"));
            }
        }
    }
    else if((0 == strcmp(p, "+STATUS:NET STATE REGISTERED")) ||
            (0 == strcmp(p, "+STATUS:NET STATE UNREGISTER")))
    {
        e.kind  = E27_D_EVENT_REGISTRATION;
        e.value = (uint16_t)(0 == strcmp(p, "+STATUS:NET STATE REGISTERED"));
    }
    else if(0U != d->pending)
    {
        uint8_t all = (uint8_t)(E27_D_CMD_ALL_QUERY == d->command);
        if((E27_D_CMD_IMEI == d->command) && (0U != digits(p, 15U)))
        {
            e.kind = E27_D_EVENT_IMEI;
        }
        else if((0U != all) && (0 == strncmp(p, "+IMEI: ", 7U)) &&
                (0U != digits(p + 7U, 15U)))
        {
            e.kind   = E27_D_EVENT_IMEI;
            e.p_text = p + 7U;
            e.size   = 15U;
        }
        else if(((E27_D_CMD_ICCID == d->command) || (0U != all)) &&
                (0 == strncmp(p, "+ICCID: ", 8U)) &&
                (0U != digits(p + 8U, 20U)))
        {
            e.kind   = E27_D_EVENT_ICCID;
            e.p_text = p + 8U;
            e.size   = 20U;
        }
        else if(((E27_D_CMD_IMSI == d->command) || (0U != all)) &&
                (0 == strncmp(p, "+IMSI: ", 7U)) && (0U != digits(p + 7U, 15U)))
        {
            e.kind   = E27_D_EVENT_IMSI;
            e.p_text = p + 7U;
            e.size   = 15U;
        }
        else if((E27_D_CMD_CPIN == d->command) &&
                (0 == strcmp(p, "+CPIN: READY")))
        {
            e.kind  = E27_D_EVENT_SIM_READY;
            e.value = 1U;
        }
        else if((E27_D_CMD_CSQ == d->command) &&
                (0 == strncmp(p, "+CSQ: ", 6U)))
        {
            if((0U != pair(p + 6U, 99U, &e)) &&
               ((e.value <= 31U) || (99U == e.value)))
            {
                e.kind = E27_D_EVENT_SIGNAL;
            }
        }
        else if((E27_D_CMD_CONNECT_QUERY == d->command) &&
                (0 == strncmp(p, "+ASKCONNECT: ", 13U)))
        {
            if(0U != pair(p + 13U, 1U, &e))
            {
                e.kind = E27_D_EVENT_CONNECTIONS;
            }
        }
        else if(((E27_D_CMD_DTUID_QUERY == d->command) || (0U != all)) &&
                (0 == strncmp(p, "+DTUID: ", 8U)))
        {
            if(0U == dtuid(p + 8U, &e))
            {
                e.p_text = p;
                e.size   = d->line_size;
            }
        }
    }
    emit(d, &e);
}

e27_d_ret_t e27_d_init(e27_d_device_t *d, const e27_d_io_t *io)
{
    e27_d_io_t copy;
    if((NULL == d) || (NULL == io) || (NULL == io->transmit) ||
       (NULL == io->event))
    {
        return E27_D_RET_PARAM;
    }
    if((0U != d->initialized) && ((0U != d->pending) || (0U != d->processing)))
    {
        return E27_D_RET_BUSY;
    }
    copy = *io;
    memset(d, 0, sizeof(*d));
    d->io      = copy;
    d->command = E27_D_CMD_COUNT;
    /* Minimum allowed nonzero AUTOATO interval, conservative until explicitly
     * set. The manual's factory default is 120 s, but persisted settings vary.
     */
    d->autoato_seconds = 60U;
    d->initialized     = 1U;
    return E27_D_RET_OK;
}

e27_d_ret_t e27_d_tick(e27_d_device_t *d, uint32_t now_ms)
{
    e27_d_ret_t ret = device_check(d);
    if(E27_D_RET_OK != ret)
    {
        return ret;
    }
    d->processing = 1U;
    if((E27_D_MODE_COMMAND == d->mode) && (0U != d->autoato_seconds) &&
       ((now_ms - d->mode_start_ms) >= (uint32_t)d->autoato_seconds * 1000U))
    {
        d->mode = E27_D_MODE_UNKNOWN;
        if(0U != d->pending)
        {
            complete(d, E27_D_RET_TIMEOUT);
            ret = E27_D_RET_TIMEOUT;
        }
    }
    if((0U != d->pending) && ((now_ms - d->start_ms) >= d->reply_timeout_ms))
    {
        complete(d, E27_D_RET_TIMEOUT);
        ret = E27_D_RET_TIMEOUT;
    }
    if(E27_D_RET_TIMEOUT == ret)
    {
        d->line_size  = 0U;
        d->discarding = 1U;
    }
    d->processing = 0U;
    return ret;
}

e27_d_ret_t e27_d_command_start(e27_d_device_t        *d,
                                const e27_d_request_t *r,
                                uint32_t               now_ms,
                                uint32_t               tx_ms,
                                uint32_t               reply_ms)
{
    char        command[E27_D_COMMAND_CAPACITY];
    uint16_t    size;
    e27_d_ret_t ret = device_check(d);
    if(E27_D_RET_OK != ret)
    {
        return ret;
    }
    if((0U != d->pending) || (0U != d->line_size))
    {
        return E27_D_RET_BUSY;
    }
    if((0U == tx_ms) || (tx_ms > E27_D_TIME_MAX) || (0U == reply_ms) ||
       (reply_ms > E27_D_TIME_MAX) || (reply_ms < tx_ms))
    {
        return E27_D_RET_PARAM;
    }
    ret = command_build(r, command, &size);
    if(E27_D_RET_OK != ret)
    {
        return ret;
    }
    (void)e27_d_tick(d, now_ms);
    if((E27_D_CMD_ENTER != r->command) && (E27_D_MODE_COMMAND != d->mode))
    {
        return E27_D_RET_NOT_READY;
    }
    d->line_size               = 0U;
    d->discarding              = 0U;
    d->command                 = r->command;
    d->start_ms                = now_ms;
    d->reply_timeout_ms        = reply_ms;
    d->pending_autoato_seconds = (E27_D_CMD_SET_AUTOATO == r->command)
                                     ? r->args.autoato_seconds
                                     : 0U;
    d->processing              = 1U;
    ret = d->io.transmit(d->io.p_context, (const uint8_t *)command, size,
                         tx_ms);
    d->processing = 0U;
    if(E27_D_RET_OK != ret)
    {
        d->mode = E27_D_MODE_UNKNOWN;
        return ret;
    }
    d->pending = 1U;
    return E27_D_RET_OK;
}

e27_d_ret_t
e27_d_response_process(e27_d_device_t *d, const uint8_t *data, uint16_t size)
{
    uint16_t    i;
    e27_d_ret_t ret = device_check(d);
    if(E27_D_RET_OK != ret)
    {
        return ret;
    }
    if((NULL == data) && (0U != size))
    {
        return E27_D_RET_PARAM;
    }
    d->processing = 1U;
    for(i = 0U; i < size; ++i)
    {
        uint8_t c = data[i];
        if(('\r' == c) || ('\n' == c))
        {
            if(0U == d->discarding)
            {
                line_process(d);
            }
            d->line_size  = 0U;
            d->discarding = 0U;
        }
        else if(0U != d->discarding)
        {
            continue;
        }
        else if((c >= 0x20U) && (c <= 0x7EU) &&
                (d->line_size < (sizeof(d->line) - 1U)))
        {
            d->line[d->line_size++] = (char)c;
        }
        else
        {
            e27_d_event_t e = {0};
            e.kind          = E27_D_EVENT_OVERFLOW;
            e.command       = (0U != d->pending) ? d->command : E27_D_CMD_COUNT;
            e.result        = E27_D_RET_OVERFLOW;
            d->line_size    = 0U;
            d->discarding   = 1U;
            if(0U != d->pending)
            {
                complete(d, E27_D_RET_OVERFLOW);
            }
            emit(d, &e);
            ret = E27_D_RET_OVERFLOW;
        }
    }
    d->processing = 0U;
    return ret;
}

e27_d_ret_t e27_d_transport_lost(e27_d_device_t *d)
{
    e27_d_ret_t ret = device_check(d);
    if(E27_D_RET_OK != ret)
    {
        return ret;
    }
    d->processing = 1U;
    d->mode       = E27_D_MODE_UNKNOWN;
    d->line_size  = 0U;
    d->discarding = 1U;
    if(0U != d->pending)
    {
        complete(d, E27_D_RET_IO);
    }
    d->processing = 0U;
    return E27_D_RET_OK;
}
