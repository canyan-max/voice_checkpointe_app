/**
 ******************************************************************************
 *@file               :   cs4344.c
 *@brief              :   Validate the portable CS4344 stream contract.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "cs4344.h"

/* define   -----------------------------------------------------------------*/
#define CS4344_SAMPLE_RATE_MIN_HZ       2000U
#define CS4344_SAMPLE_RATE_MAX_HZ       200000U
#define CS4344_MCLK_MIN_HZ              512000U
#define CS4344_MCLK_MAX_HZ              50000000U

/* Private  functions  ------------------------------------------------------*/
static uint8_t cs4344_rate_ratio_is_valid(uint32_t sample_rate_hz,
                                          uint16_t mclk_lrck_ratio)
{
    switch(mclk_lrck_ratio)
    {
        case 256U:
        case 384U:
            return (uint8_t)((sample_rate_hz <= 50000U) ||
                             ((sample_rate_hz >= 84000U) &&
                              (sample_rate_hz <= 134000U)));
        case 1024U:
            return (uint8_t)(sample_rate_hz <= 50000U);
        case 512U:
        case 768U:
            return (uint8_t)((sample_rate_hz >= 42000U) &&
                             (sample_rate_hz <= 67000U));
        case 1152U:
            return (uint8_t)((sample_rate_hz >= 30000U) &&
                             (sample_rate_hz <= 34000U));
        case 128U:
        case 192U:
            return (uint8_t)(((sample_rate_hz >= 50000U) &&
                              (sample_rate_hz <= 100000U)) ||
                             ((sample_rate_hz >= 168000U) &&
                              (sample_rate_hz <= 200000U)));
        case 64U:
        case 96U:
            return (uint8_t)((sample_rate_hz >= 100000U) &&
                             (sample_rate_hz <= 200000U));
        default:
            return 0U;
    }
}

/* Exported functions -------------------------------------------------------*/
cs4344_ret_t cs4344_stream_config_validate(
    const cs4344_stream_config_t *p_config)
{
    uint32_t mclk_hz;

    if(NULL == p_config)
    {
        return CS4344_RET_PARAM;
    }
    if((CS4344_SERIAL_FORMAT_I2S != p_config->serial_format) ||
       (2U != p_config->channel_count) ||
       (p_config->sample_bits < 16U) || (p_config->sample_bits > 24U) ||
       (p_config->sample_rate_hz < CS4344_SAMPLE_RATE_MIN_HZ) ||
       (p_config->sample_rate_hz > CS4344_SAMPLE_RATE_MAX_HZ) ||
       (0U == cs4344_rate_ratio_is_valid(p_config->sample_rate_hz,
                                         p_config->mclk_lrck_ratio)))
    {
        return CS4344_RET_UNSUPPORTED;
    }
    if(p_config->sample_rate_hz >
       (UINT32_MAX / (uint32_t)p_config->mclk_lrck_ratio))
    {
        return CS4344_RET_UNSUPPORTED;
    }
    mclk_hz = p_config->sample_rate_hz *
              (uint32_t)p_config->mclk_lrck_ratio;
    if((mclk_hz < CS4344_MCLK_MIN_HZ) ||
       (mclk_hz > CS4344_MCLK_MAX_HZ))
    {
        return CS4344_RET_UNSUPPORTED;
    }
    return CS4344_RET_OK;
}

/* end of file --------------------------------------------------------------*/
