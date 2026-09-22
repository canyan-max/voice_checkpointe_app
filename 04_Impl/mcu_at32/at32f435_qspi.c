/**
 ******************************************************************************
 *@file               :   at32f435_qspi.c
 *@brief              :   AT32F435 polling QSPI command-port implementation.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include <string.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "plat_qspi.h"
#include "plat_sys.h"

#define AT32F435_QSPI_FIFO_DEPTH_BYTES    128U

typedef struct AT32F435_QSPI_CFG_T
{
    qspi_type *p_instance;
} at32f435_qspi_cfg_t;

static const at32f435_qspi_cfg_t qspi_cfg[BOARD_QSPI_RESOURCE_NUM] =
{
    [BOARD_QSPI_STORAGE] = {BOARD_QSPI_STORAGE_INSTANCE}
};

_Static_assert((sizeof(qspi_cfg) / sizeof(qspi_cfg[0])) ==
                   BOARD_QSPI_RESOURCE_NUM,
               "QSPI resource table size mismatch");

static platform_err_t at32f435_qspi_wait(qspi_type *p_qspi,
                                         uint32_t   flag,
                                         uint32_t   start_ms,
                                         uint32_t   timeout_ms)
{
    while(RESET == qspi_flag_get(p_qspi, flag))
    {
        if((plat_tick_get_ms() - start_ms) >= timeout_ms)
        {
            return PLATFORM_ERR_TIMEOUT;
        }
    }
    return PLATFORM_ERR_OK;
}

static platform_err_t at32f435_qspi_mode_get(
    plat_qspi_transfer_mode_t mode,
    qspi_operate_mode_type   *p_at32_mode)
{
    static const qspi_operate_mode_type mode_map[PLAT_QSPI_TRANSFER_MODE_NUM] =
    {
        QSPI_OPERATE_MODE_111,
        QSPI_OPERATE_MODE_112,
        QSPI_OPERATE_MODE_114,
        QSPI_OPERATE_MODE_122,
        QSPI_OPERATE_MODE_144,
        QSPI_OPERATE_MODE_222,
        QSPI_OPERATE_MODE_444
    };

    if(((uint32_t)mode >= (uint32_t)PLAT_QSPI_TRANSFER_MODE_NUM) ||
       (NULL == p_at32_mode))
    {
        return PLATFORM_ERR_PARAM;
    }

    *p_at32_mode = mode_map[mode];
    return PLATFORM_ERR_OK;
}

static platform_err_t at32f435_qspi_command_build(
    const plat_qspi_command_t *p_command,
    uint32_t                   data_size,
    confirm_state              write_enable,
    qspi_cmd_type             *p_at32_command)
{
    platform_err_t ret;

    if((NULL == p_command) || (NULL == p_at32_command) ||
       (p_command->address_size > 4U) ||
       (p_command->dummy_cycles > 32U))
    {
        return PLATFORM_ERR_PARAM;
    }

    (void)memset(p_at32_command, 0, sizeof(*p_at32_command));
    ret = at32f435_qspi_mode_get(p_command->transfer_mode,
                                 &p_at32_command->operation_mode);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_at32_command->instruction_code = p_command->instruction;
    p_at32_command->instruction_length = QSPI_CMD_INSLEN_1_BYTE;
    p_at32_command->address_code = p_command->address;
    p_at32_command->address_length =
        (qspi_cmd_adrlen_type)p_command->address_size;
    p_at32_command->data_counter = data_size;
    p_at32_command->second_dummy_cycle_num = p_command->dummy_cycles;
    p_at32_command->read_status_config = QSPI_RSTSC_SW_ONCE;
    p_at32_command->read_status_enable = FALSE;
    p_at32_command->write_data_enable = write_enable;
    return PLATFORM_ERR_OK;
}

static void at32f435_qspi_command_flag_clear(qspi_type *p_qspi)
{
    if(RESET != qspi_flag_get(p_qspi, QSPI_CMDSTS_FLAG))
    {
        qspi_flag_clear(p_qspi, QSPI_CMDSTS_FLAG);
    }
}

platform_err_t plat_qspi_command(plat_qspi_id_t            id,
                                 const plat_qspi_command_t *p_command,
                                 uint32_t                   timeout_ms)
{
    qspi_cmd_type at32_command;
    qspi_type *p_qspi;
    platform_err_t ret;
    uint32_t start_ms;

    if((id >= BOARD_QSPI_RESOURCE_NUM) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = at32f435_qspi_command_build(p_command,
                                      0U,
                                      TRUE,
                                      &at32_command);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_qspi = qspi_cfg[id].p_instance;
    at32f435_qspi_command_flag_clear(p_qspi);
    start_ms = plat_tick_get_ms();
    qspi_cmd_operation_kick(p_qspi, &at32_command);
    ret = at32f435_qspi_wait(p_qspi,
                             QSPI_CMDSTS_FLAG,
                             start_ms,
                             timeout_ms);
    at32f435_qspi_command_flag_clear(p_qspi);
    return ret;
}

platform_err_t plat_qspi_read(plat_qspi_id_t            id,
                              const plat_qspi_command_t *p_command,
                              uint8_t                   *p_data,
                              uint32_t                   data_size,
                              uint32_t                   timeout_ms)
{
    qspi_cmd_type at32_command;
    qspi_type *p_qspi;
    platform_err_t ret;
    uint32_t start_ms;
    uint32_t remaining;
    uint32_t chunk_size;
    uint32_t index;

    if((id >= BOARD_QSPI_RESOURCE_NUM) || (NULL == p_data) ||
       (0U == data_size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = at32f435_qspi_command_build(p_command,
                                      data_size,
                                      FALSE,
                                      &at32_command);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_qspi = qspi_cfg[id].p_instance;
    at32f435_qspi_command_flag_clear(p_qspi);
    start_ms = plat_tick_get_ms();
    qspi_cmd_operation_kick(p_qspi, &at32_command);
    remaining = data_size;

    while(0U != remaining)
    {
        chunk_size = remaining;
        if(chunk_size > AT32F435_QSPI_FIFO_DEPTH_BYTES)
        {
            chunk_size = AT32F435_QSPI_FIFO_DEPTH_BYTES;
        }

        ret = at32f435_qspi_wait(p_qspi,
                                 QSPI_RXFIFORDY_FLAG,
                                 start_ms,
                                 timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_qspi_command_flag_clear(p_qspi);
            return ret;
        }

        for(index = 0U; index < chunk_size; index++)
        {
            *p_data++ = qspi_byte_read(p_qspi);
        }
        remaining -= chunk_size;
    }

    ret = at32f435_qspi_wait(p_qspi,
                             QSPI_CMDSTS_FLAG,
                             start_ms,
                             timeout_ms);
    at32f435_qspi_command_flag_clear(p_qspi);
    return ret;
}

platform_err_t plat_qspi_write(plat_qspi_id_t            id,
                               const plat_qspi_command_t *p_command,
                               const uint8_t             *p_data,
                               uint32_t                   data_size,
                               uint32_t                   timeout_ms)
{
    qspi_cmd_type at32_command;
    qspi_type *p_qspi;
    platform_err_t ret;
    uint32_t start_ms;
    uint32_t index;

    if((id >= BOARD_QSPI_RESOURCE_NUM) || (NULL == p_data) ||
       (0U == data_size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    ret = at32f435_qspi_command_build(p_command,
                                      data_size,
                                      TRUE,
                                      &at32_command);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    p_qspi = qspi_cfg[id].p_instance;
    at32f435_qspi_command_flag_clear(p_qspi);
    start_ms = plat_tick_get_ms();
    qspi_cmd_operation_kick(p_qspi, &at32_command);

    for(index = 0U; index < data_size; index++)
    {
        ret = at32f435_qspi_wait(p_qspi,
                                 QSPI_TXFIFORDY_FLAG,
                                 start_ms,
                                 timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_qspi_command_flag_clear(p_qspi);
            return ret;
        }
        qspi_byte_write(p_qspi, p_data[index]);
    }

    ret = at32f435_qspi_wait(p_qspi,
                             QSPI_CMDSTS_FLAG,
                             start_ms,
                             timeout_ms);
    at32f435_qspi_command_flag_clear(p_qspi);
    return ret;
}
