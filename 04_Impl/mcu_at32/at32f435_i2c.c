/**
 ******************************************************************************
 *@file               :   at32f435_i2c.c
 *@brief              :   AT32F435 polling I2C master implementation.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "plat_i2c.h"
#include "plat_sys.h"

#define AT32F435_I2C_MAX_TRANSFER_SIZE  255U
#define AT32F435_I2C_ERROR_FLAGS        \
    (I2C_ACKFAIL_FLAG | I2C_BUSERR_FLAG | I2C_ARLOST_FLAG | I2C_OUF_FLAG)

static i2c_type *const i2c_cfg[BOARD_I2C_RESOURCE_NUM] =
{
    [BOARD_I2C_SHARED] = BOARD_I2C_SHARED_INSTANCE
};

_Static_assert((sizeof(i2c_cfg) / sizeof(i2c_cfg[0])) ==
                   BOARD_I2C_RESOURCE_NUM,
               "I2C resource table size mismatch");

static void at32f435_i2c_ctrl2_reset(i2c_type *p_i2c)
{
    p_i2c->ctrl2_bit.saddr = 0U;
    p_i2c->ctrl2_bit.readh10 = 0U;
    p_i2c->ctrl2_bit.cnt = 0U;
    p_i2c->ctrl2_bit.rlden = 0U;
    p_i2c->ctrl2_bit.dir = 0U;
}

static platform_err_t at32f435_i2c_wait(i2c_type *p_i2c,
                                        uint32_t  flag,
                                        uint8_t   wait_for_reset,
                                        uint32_t  start_ms,
                                        uint32_t  timeout_ms)
{
    for(;;)
    {
        flag_status status = i2c_flag_get(p_i2c, flag);

        if(((0U == wait_for_reset) && (RESET != status)) ||
           ((0U != wait_for_reset) && (RESET == status)))
        {
            return PLATFORM_ERR_OK;
        }
        if(0U != (p_i2c->sts & AT32F435_I2C_ERROR_FLAGS))
        {
            return PLATFORM_ERR_HW;
        }
        if((plat_tick_get_ms() - start_ms) >= timeout_ms)
        {
            return PLATFORM_ERR_TIMEOUT;
        }
    }
}

static void at32f435_i2c_finish(i2c_type *p_i2c)
{
    if(RESET != i2c_flag_get(p_i2c, I2C_STOPF_FLAG))
    {
        i2c_flag_clear(p_i2c, I2C_STOPF_FLAG);
    }
    i2c_flag_clear(p_i2c, AT32F435_I2C_ERROR_FLAGS);
    at32f435_i2c_ctrl2_reset(p_i2c);
}

static void at32f435_i2c_abort(i2c_type *p_i2c,
                               uint32_t  start_ms,
                               uint32_t  timeout_ms)
{
    if(RESET != i2c_flag_get(p_i2c, I2C_BUSYF_FLAG))
    {
        i2c_stop_generate(p_i2c);
        while((RESET == i2c_flag_get(p_i2c, I2C_STOPF_FLAG)) &&
              ((plat_tick_get_ms() - start_ms) < timeout_ms))
        {
            /* Wait only within the caller's original deadline. */
        }
    }
    at32f435_i2c_finish(p_i2c);
}

static platform_err_t at32f435_i2c_validate(plat_i2c_id_t id,
                                             uint8_t       address_7bit,
                                             const void   *p_data,
                                             uint16_t      size,
                                             uint32_t      timeout_ms)
{
    if((id >= BOARD_I2C_RESOURCE_NUM) ||
       (address_7bit > 0x7FU) ||
       (NULL == p_data) ||
       (0U == size) ||
       (size > AT32F435_I2C_MAX_TRANSFER_SIZE) ||
       (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }
    return PLATFORM_ERR_OK;
}

platform_err_t plat_i2c_write(plat_i2c_id_t id,
                              uint8_t       address_7bit,
                              const uint8_t *p_data,
                              uint16_t      size,
                              uint32_t      timeout_ms)
{
    i2c_type *p_i2c;
    platform_err_t ret;
    uint32_t start_ms;
    uint16_t index;

    ret = at32f435_i2c_validate(id, address_7bit, p_data, size, timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_i2c = i2c_cfg[id];
    start_ms = plat_tick_get_ms();
    ret = at32f435_i2c_wait(p_i2c,
                            I2C_BUSYF_FLAG,
                            1U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    at32f435_i2c_finish(p_i2c);
    i2c_transmit_set(p_i2c,
                     (uint16_t)address_7bit << 1U,
                     (uint8_t)size,
                     I2C_AUTO_STOP_MODE,
                     I2C_GEN_START_WRITE);
    for(index = 0U; index < size; index++)
    {
        ret = at32f435_i2c_wait(p_i2c,
                                I2C_TDIS_FLAG,
                                0U,
                                start_ms,
                                timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_i2c_abort(p_i2c, start_ms, timeout_ms);
            return ret;
        }
        i2c_data_send(p_i2c, p_data[index]);
    }

    ret = at32f435_i2c_wait(p_i2c,
                            I2C_STOPF_FLAG,
                            0U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        at32f435_i2c_abort(p_i2c, start_ms, timeout_ms);
        return ret;
    }
    at32f435_i2c_finish(p_i2c);
    return PLATFORM_ERR_OK;
}

platform_err_t plat_i2c_read(plat_i2c_id_t id,
                             uint8_t       address_7bit,
                             uint8_t      *p_data,
                             uint16_t      size,
                             uint32_t      timeout_ms)
{
    i2c_type *p_i2c;
    platform_err_t ret;
    uint32_t start_ms;
    uint16_t index;

    ret = at32f435_i2c_validate(id, address_7bit, p_data, size, timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_i2c = i2c_cfg[id];
    start_ms = plat_tick_get_ms();
    ret = at32f435_i2c_wait(p_i2c,
                            I2C_BUSYF_FLAG,
                            1U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    at32f435_i2c_finish(p_i2c);
    i2c_transmit_set(p_i2c,
                     (uint16_t)address_7bit << 1U,
                     (uint8_t)size,
                     I2C_AUTO_STOP_MODE,
                     I2C_GEN_START_READ);
    for(index = 0U; index < size; index++)
    {
        ret = at32f435_i2c_wait(p_i2c,
                                I2C_RDBF_FLAG,
                                0U,
                                start_ms,
                                timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_i2c_abort(p_i2c, start_ms, timeout_ms);
            return ret;
        }
        p_data[index] = i2c_data_receive(p_i2c);
    }

    ret = at32f435_i2c_wait(p_i2c,
                            I2C_STOPF_FLAG,
                            0U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        at32f435_i2c_abort(p_i2c, start_ms, timeout_ms);
        return ret;
    }
    at32f435_i2c_finish(p_i2c);
    return PLATFORM_ERR_OK;
}
