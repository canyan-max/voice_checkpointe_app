/**
 ******************************************************************************
 *@file               :   comm_app.c
 *@brief              :   Run the communication task and dispatch checkpoint requests.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* includes -----------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "comm_app.h"
#include "plat_log.h"
#include "plat_sys.h"
#include "proto_checkpoint.h"

/* define -------------------------------------------------------------------*/
#define COMM_STACK_WORDS   (768U)
#define COMM_POLL_MS       (5U)
#define COMM_RX_CHUNK      (64U)
#define COMM_PAYLOAD_BYTES (512U)
#define COMM_RX_GAP_MS     (100U)
#define COMM_TX_TIMEOUT_MS (50U)
#define COMM_QUERY_ID      (0xFFU)
#define COMM_ACK_OK        (0xFFU)
#define COMM_ACK_FAILED    (0x00U)

/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/
static comm_app_io_t             g_comm_io;
static comm_app_actions_t        g_comm_actions;
static TaskHandle_t              g_comm_task;
static StaticTask_t              g_comm_task_control;
static StackType_t               g_comm_stack[COMM_STACK_WORDS];
static proto_checkpoint_parser_t g_comm_parser;
static uint8_t                   g_comm_payload[COMM_PAYLOAD_BYTES];

/* private functions --------------------------------------------------------*/
/* Called only after checksum and payload validation. */
static uint8_t
comm_actions_handle(const proto_checkpoint_request_t *p_request) {
    uint8_t item_number  = 1U;
    uint8_t display_text = 0U;

    switch (p_request->command) {
        case PROTO_CHECKPOINT_VOICE_1:
            break;
        case PROTO_CHECKPOINT_VOICE_2:
            item_number = 2U;
            break;
        case PROTO_CHECKPOINT_VOICE_3:
            item_number = 3U;
            break;
        case PROTO_CHECKPOINT_DISPLAY_1:
        case PROTO_CHECKPOINT_DISPLAY_2:
        case PROTO_CHECKPOINT_DISPLAY_3:
            display_text = 1U;
            if (PROTO_CHECKPOINT_DISPLAY_2 == p_request->command) {
                item_number = 2U;
            } else if (PROTO_CHECKPOINT_DISPLAY_3 == p_request->command) {
                item_number = 3U;
            }
            break;
        case PROTO_CHECKPOINT_VOICE_REPEAT: {
            platform_err_t
                ret = g_comm_actions
                          .pf_voice_repeat_set(g_comm_actions.p_context,
                                               p_request->p_payload[0U]);
            return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
        }
        case PROTO_CHECKPOINT_DISPLAY_REPEAT: {
            platform_err_t
                ret = g_comm_actions
                          .pf_display_repeat_set(g_comm_actions.p_context,
                                                 p_request->p_payload[0U]);
            return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
        }
        case PROTO_CHECKPOINT_PIR_ENABLE: {
            const uint8_t        enabled = (0xFFU == p_request->p_payload[0U]) ? 1U : 0U;
            const platform_err_t ret = g_comm_actions.pf_pir_enable_set(g_comm_actions.p_context,
                                                                        enabled);
            return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
        }
        case PROTO_CHECKPOINT_PIR_ITEM: {
            const platform_err_t ret = g_comm_actions.pf_pir_item_set(g_comm_actions.p_context,
                                                                      p_request->p_payload[0U]);
            return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
        }
        case PROTO_CHECKPOINT_PLAY: {
            item_number = p_request->p_payload[0U];
            platform_err_t
                ret = (0U == item_number)
                          ? g_comm_actions.pf_stop(g_comm_actions.p_context)
                          : g_comm_actions.pf_play(g_comm_actions.p_context,
                                                   item_number);
            plat_log_i("COMM voice enqueue: item=%u ret=%d",
                       (unsigned int)item_number, (int)ret);
            return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
        }
        default:
            /* Other validated commands retain their protocol test replies. */
            return COMM_ACK_OK;
    }

    platform_err_t
        ret = (0U != display_text)
                  ? g_comm_actions.pf_display_set(g_comm_actions.p_context,
                                                  item_number,
                                                  p_request->p_payload,
                                                  p_request->length)
                  : g_comm_actions.pf_voice_set(g_comm_actions.p_context,
                                                item_number,
                                                p_request->p_payload,
                                                p_request->length);
    return (PLATFORM_ERR_OK == ret) ? COMM_ACK_OK : COMM_ACK_FAILED;
}

static void comm_handle_request(const proto_checkpoint_request_t *p_request,
                                uint8_t                           checksum_ok) {
    uint8_t reply[PROTO_CHECKPOINT_REPLY_SIZE];

    uint8_t status    = COMM_ACK_FAILED;
    uint8_t update_id = 0U;
    if ((0U != checksum_ok) &&
        (0U != proto_checkpoint_payload_valid(p_request))) {
        status = COMM_ACK_OK;
        if (PROTO_CHECKPOINT_DEVICE_ID == p_request->command) {
            if ((COMM_QUERY_ID == p_request->id) &&
                (COMM_QUERY_ID == p_request->p_payload[0U])) {
                status = g_comm_parser.device_id;
            } else {
                update_id = (uint8_t)(g_comm_parser.device_id != p_request->p_payload[0U]);
            }
        } else {
            status = comm_actions_handle(p_request);
        }
    }

    size_t size = proto_checkpoint_encode_reply(reply, sizeof(reply),
                                                p_request->id,
                                                p_request->command, status);
    if (0U == size) {
        plat_log_e("COMM reply encode failed: cmd=%02X",
                   (unsigned int)p_request->command);
        return;
    }

    platform_err_t ret = g_comm_io.pf_send(reply, (uint16_t)size,
                                           COMM_TX_TIMEOUT_MS);
    if (PLATFORM_ERR_OK != ret) {
        plat_log_e("COMM reply send failed: cmd=%02X ret=%d",
                   (unsigned int)p_request->command, (int)ret);
        return;
    }

    if (0U != update_id) {
        /* The reply used the old ID. Switch RX matching after it is sent. */
        g_comm_parser.device_id = p_request->p_payload[0U];
        if (NULL != g_comm_actions.pf_device_id_changed) {
            g_comm_actions.pf_device_id_changed(g_comm_actions.p_context, g_comm_parser.device_id);
        }
        plat_log_i("COMM ID changed: %02X -> %02X", (unsigned int)p_request->id,
                   (unsigned int)g_comm_parser.device_id);
    }

    plat_log_i(
        "COMM reply sent: id=%02X cmd=%02X data=%02X stack_free=%lu words",
        (unsigned int)p_request->id, (unsigned int)p_request->command,
        (unsigned int)status, (unsigned long)uxTaskGetStackHighWaterMark(NULL));
}

static void comm_feed_bytes(const uint8_t *p_data, uint16_t size) {
    proto_checkpoint_request_t request = {0};
    uint8_t                    invalid = 0U;

    for (uint16_t i = 0U; i < size; i++) {
        proto_checkpoint_result_t result = proto_checkpoint_feed(&g_comm_parser,
                                                                 p_data[i],
                                                                 &request);
        if (PROTO_CHECKPOINT_FRAME == result) {
            comm_handle_request(&request, 1U);
            plat_log_i("COMM RX valid: id=%02X cmd=%02X len=%u checksum=OK",
                       (unsigned int)request.id, (unsigned int)request.command,
                       (unsigned int)request.length);
        } else if (PROTO_CHECKPOINT_BAD_CHECKSUM == result) {
            comm_handle_request(&request, 0U);
            plat_log_w("COMM RX bad checksum: id=%02X cmd=%02X",
                       (unsigned int)request.id, (unsigned int)request.command);
        } else if (PROTO_CHECKPOINT_INVALID == result) {
            invalid = 1U;
        }
    }
    if (0U != invalid) {
        plat_log_w("COMM invalid request");
    }
}

static void comm_task(void *p_parameter) {
    uint8_t rx[COMM_RX_CHUNK];
    (void)p_parameter;

    plat_log_i("COMM ready: id=%02X",
               (unsigned int)g_comm_parser.device_id);
    uint32_t last_rx_ms = plat_tick_get_ms();
    for (;;) {
        uint16_t       size   = 0U;
        platform_err_t ret    = g_comm_io.pf_receive(rx, sizeof(rx), &size);
        uint32_t       now_ms = plat_tick_get_ms();

        if (PLATFORM_ERR_OK != ret) {
            proto_checkpoint_reset(&g_comm_parser);
            plat_log_w("COMM RX discarded: ret=%d", (int)ret);
        } else {
            if (COMM_RX_GAP_MS <= (uint32_t)(now_ms - last_rx_ms)) {
                proto_checkpoint_reset(&g_comm_parser);
            }
            if (0U != size) {
                last_rx_ms = now_ms;
                comm_feed_bytes(rx, size);
            }
        }
        g_comm_actions.pf_pir_poll(g_comm_actions.p_context);
        if (NULL != g_comm_actions.pf_config_poll) {
            g_comm_actions.pf_config_poll(g_comm_actions.p_context);
        }
        vTaskDelay(pdMS_TO_TICKS(COMM_POLL_MS));
    }
}

/* exported functions -------------------------------------------------------*/
platform_err_t comm_app_init(const comm_app_io_t      *p_io,
                             const comm_app_actions_t *p_actions,
                             uint8_t initial_device_id) {
    if ((NULL == p_io) || (NULL == p_io->pf_init) ||
        (NULL == p_io->pf_receive) || (NULL == p_io->pf_send) ||
        (NULL == p_actions) || (NULL == p_actions->pf_voice_set) ||
        (NULL == p_actions->pf_voice_repeat_set) ||
        (NULL == p_actions->pf_display_repeat_set) ||
        (NULL == p_actions->pf_display_set) || (NULL == p_actions->pf_play) ||
        (NULL == p_actions->pf_stop) || (NULL == p_actions->pf_pir_enable_set) ||
        (NULL == p_actions->pf_pir_item_set) || (NULL == p_actions->pf_pir_poll)) {
        return PLATFORM_ERR_PARAM;
    }
    if (NULL != g_comm_task) {
        return PLATFORM_ERR_OK;
    }

    if (0U == proto_checkpoint_init(&g_comm_parser, initial_device_id,
                                    g_comm_payload, sizeof(g_comm_payload))) {
        return PLATFORM_ERR_PARAM;
    }
    platform_err_t ret = p_io->pf_init();
    if (PLATFORM_ERR_OK != ret) {
        return ret;
    }
    g_comm_io      = *p_io;
    g_comm_actions = *p_actions;
    g_comm_task = xTaskCreateStatic(comm_task, "COMM", COMM_STACK_WORDS, NULL,
                                    tskIDLE_PRIORITY, g_comm_stack,
                                    &g_comm_task_control);
    return (NULL != g_comm_task) ? PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}

/* end of file --------------------------------------------------------------*/
