#include <stddef.h>
#include "sd_card_presence_service.h"
#include "bsp_sd_card.h"

#define SD_CARD_DEBOUNCE_MS       50U
#define SD_CARD_MAX_SAMPLE_GAP_MS 100U

platform_err_t sd_card_presence_service_init(sd_card_presence_service_t *p_service)
{
    if(NULL == p_service)
    {
        return PLATFORM_ERR_PARAM;
    }
    *p_service = (sd_card_presence_service_t){0};
    p_service->initialized = 1U;
    return PLATFORM_ERR_OK;
}

platform_err_t sd_card_presence_service_poll(sd_card_presence_service_t *p_service,
                                            uint32_t now_ms,
                                            sd_card_presence_event_t *p_event,
                                            uint8_t *p_present)
{
    uint8_t sample;
    platform_err_t result;

    if(NULL != p_event)
    {
        *p_event = SD_CARD_PRESENCE_EVENT_NONE;
    }
    if((NULL == p_service) || (NULL == p_event) || (NULL == p_present) ||
       (0U == p_service->initialized))
    {
        return PLATFORM_ERR_PARAM;
    }
    result = bsp_sd_card_is_present(&sample);
    if(PLATFORM_ERR_OK != result)
    {
        p_service->have_candidate = 0U;
        return result;
    }
    if((0U == p_service->have_candidate) || (sample != p_service->candidate) ||
       ((uint32_t)(now_ms - p_service->last_sample) > SD_CARD_MAX_SAMPLE_GAP_MS))
    {
        p_service->candidate = sample;
        p_service->candidate_since = now_ms;
        p_service->have_candidate = 1U;
    }
    p_service->last_sample = now_ms;
    if(((uint32_t)(now_ms - p_service->candidate_since) >= SD_CARD_DEBOUNCE_MS) &&
       ((0U == p_service->have_confirmed) ||
        (p_service->confirmed != p_service->candidate)))
    {
        *p_event = (0U == p_service->have_confirmed) ?
                  SD_CARD_PRESENCE_EVENT_INITIAL : SD_CARD_PRESENCE_EVENT_CHANGED;
        p_service->confirmed = p_service->candidate;
        p_service->have_confirmed = 1U;
        *p_present = p_service->confirmed;
    }
    return PLATFORM_ERR_OK;
}

platform_err_t sd_card_presence_service_get(const sd_card_presence_service_t *p_service,
                                           uint8_t *p_present)
{
    if((NULL == p_service) || (NULL == p_present) ||
       (0U == p_service->initialized))
    {
        return PLATFORM_ERR_PARAM;
    }
    if(0U == p_service->have_confirmed)
    {
        return PLATFORM_ERR_BUSY;
    }
    *p_present = p_service->confirmed;
    return PLATFORM_ERR_OK;
}
