/**
 ******************************************************************************
 *@file               :   comm_app.h
 *@brief              :   Declare the communication task and its injected transport.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 ******************************************************************************
 */

/* define to prevent recursive inclusion ------------------------------------*/
#ifndef COMM_APP_H
#define COMM_APP_H

/* includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "plat_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef struct COMM_APP_IO_T {
    platform_err_t (*pf_init)(void);
    platform_err_t (*pf_receive)(uint8_t  *p_data,
                                 uint16_t  capacity,
                                 uint16_t *p_size);
    platform_err_t (*pf_send)(const uint8_t *p_data,
                              uint16_t       size,
                              uint32_t       timeout_ms);
    /* Opaque progress token including bytes not yet returned by pf_receive. */
    platform_err_t (*pf_get_rx_progress)(uint16_t *p_progress);
} comm_app_io_t;

/* Synchronous callbacks on the COMM task. Do not block. The payload is
 * borrowed until the callback returns; copy it if it needs to be retained. */
typedef struct COMM_APP_ACTIONS_T {
    void *p_context;
    platform_err_t (*pf_voice_set)(void          *p_context,
                                   uint8_t        item_number,
                                   const uint8_t *p_text,
                                   uint16_t       text_size);
    platform_err_t (*pf_voice_repeat_set)(void   *p_context,
                                          uint8_t repeat_count);
    platform_err_t (*pf_display_repeat_set)(void   *p_context,
                                            uint8_t repeat_count);
    platform_err_t (*pf_display_set)(void          *p_context,
                                     uint8_t        item_number,
                                     const uint8_t *p_text,
                                     uint16_t       text_size);
    platform_err_t (*pf_play)(void *p_context, uint8_t item_number);
    platform_err_t (*pf_stop)(void *p_context);
    platform_err_t (*pf_pir_enable_set)(void *p_context, uint8_t enabled);
    platform_err_t (*pf_pir_item_set)(void *p_context, uint8_t item_number);
    void (*pf_pir_poll)(void *p_context);
    void (*pf_device_id_changed)(void *p_context, uint8_t device_id);
    /* Optional periodic work after handling received bytes and PIR. */
    void (*pf_config_poll)(void *p_context);
} comm_app_actions_t;

/* variables ----------------------------------------------------------------*/

/* function -----------------------------------------------------------------*/

/* Copies both tables. The referenced context must outlive the task. */
platform_err_t comm_app_init(const comm_app_io_t      *p_io,
                             const comm_app_actions_t *p_actions,
                             uint8_t initial_device_id);

#ifdef __cplusplus
}
#endif

#endif /* COMM_APP_H */

/* end of file --------------------------------------------------------------*/
