/**
 ******************************************************************************
 *@file               :   bsp_audio_amplifier.c
 *@brief              :   Bind the board audio amplifier to TPA3116 GPIOs.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include "board_resources.h"
#include "bsp_audio_amplifier.h"
#include "plat_gpio.h"
#include "tpa3116.h"

/* variables ----------------------------------------------------------------*/
static tpa3116_device_t audio_amplifier_device;
static uint8_t audio_amplifier_is_initialized;

/* private functions --------------------------------------------------------*/
static tpa3116_ret_t bsp_audio_amplifier_pin_write(void          *p_context,
                                                   tpa3116_pin_t  pin,
                                                   uint8_t        level)
{
    plat_gpio_id_t gpio_id;
    plat_gpio_state_t state;

    (void)p_context;
    if(level > 1U)
    {
        return TPA3116_RET_PARAM;
    }

    switch(pin)
    {
        case TPA3116_PIN_SDZ:
            gpio_id = BOARD_GPIO_TPA_SDZ;
            break;
        case TPA3116_PIN_MUTE:
            gpio_id = BOARD_GPIO_TPA_MUTE;
            break;
        default:
            return TPA3116_RET_PARAM;
    }

    state = (0U == level) ? PLAT_GPIO_RESET : PLAT_GPIO_SET;
    return (PLATFORM_ERR_OK == plat_gpio_write(gpio_id, state)) ?
           TPA3116_RET_OK : TPA3116_RET_IO;
}

static tpa3116_ret_t bsp_audio_amplifier_pin_read(void          *p_context,
                                                  tpa3116_pin_t  pin,
                                                  uint8_t       *p_level)
{
    plat_gpio_state_t state;
    platform_err_t ret;

    (void)p_context;
    if((TPA3116_PIN_FAULTZ != pin) || (NULL == p_level))
    {
        return TPA3116_RET_PARAM;
    }

    ret = plat_gpio_read(BOARD_GPIO_TPA_FAULTZ, &state);
    if(PLATFORM_ERR_OK != ret)
    {
        return TPA3116_RET_IO;
    }
    *p_level = (PLAT_GPIO_RESET == state) ? 0U : 1U;
    return TPA3116_RET_OK;
}

static platform_err_t bsp_audio_amplifier_ret_map(tpa3116_ret_t ret)
{
    switch(ret)
    {
        case TPA3116_RET_OK:
            return PLATFORM_ERR_OK;
        case TPA3116_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case TPA3116_RET_IO:
        case TPA3116_RET_NOT_READY:
        default:
            return PLATFORM_ERR_HW;
    }
}

static platform_err_t bsp_audio_amplifier_ready_check(void)
{
    return (0U != audio_amplifier_is_initialized) ?
           PLATFORM_ERR_OK : PLATFORM_ERR_HW;
}

/* exported functions -------------------------------------------------------*/
platform_err_t bsp_audio_amplifier_init(void)
{
    static const tpa3116_io_t io =
    {
        bsp_audio_amplifier_pin_write,
        bsp_audio_amplifier_pin_read,
        NULL
    };
    tpa3116_ret_t ret;

    if(0U != audio_amplifier_is_initialized)
    {
        return PLATFORM_ERR_OK;
    }

    ret = tpa3116_init(&audio_amplifier_device, &io);
    if(TPA3116_RET_OK == ret)
    {
        ret = tpa3116_shutdown(&audio_amplifier_device);
    }
    if(TPA3116_RET_OK == ret)
    {
        audio_amplifier_is_initialized = 1U;
    }
    return bsp_audio_amplifier_ret_map(ret);
}

platform_err_t bsp_audio_amplifier_enable(void)
{
    if(PLATFORM_ERR_OK != bsp_audio_amplifier_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_audio_amplifier_ret_map(
        tpa3116_enable(&audio_amplifier_device));
}

platform_err_t bsp_audio_amplifier_shutdown(void)
{
    if(PLATFORM_ERR_OK != bsp_audio_amplifier_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_audio_amplifier_ret_map(
        tpa3116_shutdown(&audio_amplifier_device));
}

platform_err_t bsp_audio_amplifier_mute_set(uint8_t is_muted)
{
    if(PLATFORM_ERR_OK != bsp_audio_amplifier_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_audio_amplifier_ret_map(
        tpa3116_mute_set(&audio_amplifier_device, is_muted));
}

platform_err_t bsp_audio_amplifier_fault_get(uint8_t *p_is_fault)
{
    if(NULL == p_is_fault)
    {
        return PLATFORM_ERR_PARAM;
    }
    if(PLATFORM_ERR_OK != bsp_audio_amplifier_ready_check())
    {
        return PLATFORM_ERR_HW;
    }
    return bsp_audio_amplifier_ret_map(
        tpa3116_fault_get(&audio_amplifier_device, p_is_fault));
}

/* end of file --------------------------------------------------------------*/
