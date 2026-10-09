#include <stddef.h>
#include "sd_card_presence_app.h"
#include "sd_card_presence_service.h"
#include "plat_log.h"
#include "plat_sys.h"
#include "FreeRTOS.h"
#include "task.h"

/* Static context and error latch owned by the caller task. */
static sd_card_presence_service_t card_presence;
static uint8_t read_failed;
/* Only this snapshot is shared with other tasks; Service remains single-owner. */
static uint8_t published_known;
static uint8_t published_present;
static uint32_t published_generation;

platform_err_t sd_card_presence_app_init(void)
{
    read_failed = 0U;
    taskENTER_CRITICAL();
    published_known = 0U;
    published_present = 0U;
    published_generation = 0U;
    taskEXIT_CRITICAL();
    return sd_card_presence_service_init(&card_presence);
}

void sd_card_presence_app_poll(void)
{
    sd_card_presence_event_t event;
    uint8_t present;
    platform_err_t result;

    result = sd_card_presence_service_poll(&card_presence, plat_tick_get_ms(),
                                           &event, &present);
    if(PLATFORM_ERR_OK != result)
    {
        if(0U == read_failed)
        {
            plat_log_e("TF_CD_TEST read failed ret=%d", (int)result);
        }
        read_failed = 1U;
        return;
    }
    if(0U != read_failed)
    {
        plat_log_i("TF_CD_TEST read recovered");
        read_failed = 0U;
    }
    if(SD_CARD_PRESENCE_EVENT_NONE != event)
    {
        taskENTER_CRITICAL();
        if((0U != published_known) && (0U != published_present) && (0U == present))
        {
            published_generation++;
        }
        published_present = present;
        published_known = 1U;
        taskEXIT_CRITICAL();
        plat_log_i("TF_CD_TEST %s: card=%s stack_free=%lu words",
                   (SD_CARD_PRESENCE_EVENT_INITIAL == event) ? "initial" : "changed",
                   (0U != present) ? "inserted" : "absent",
                   (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    }
}

platform_err_t sd_card_presence_app_get(uint8_t *p_present)
{
    uint32_t generation;
    return sd_card_presence_app_snapshot_get(p_present, &generation);
}

platform_err_t sd_card_presence_app_snapshot_get(uint8_t *p_present,
                                                uint32_t *p_generation)
{
    uint8_t known;
    uint8_t present;
    uint32_t generation;

    if((NULL == p_present) || (NULL == p_generation))
    {
        return PLATFORM_ERR_PARAM;
    }
    taskENTER_CRITICAL();
    known = published_known;
    present = published_present;
    generation = published_generation;
    taskEXIT_CRITICAL();
    if(0U == known)
    {
        return PLATFORM_ERR_BUSY;
    }
    *p_present = present;
    *p_generation = generation;
    return PLATFORM_ERR_OK;
}
