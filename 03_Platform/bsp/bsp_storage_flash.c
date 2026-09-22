/**
 ******************************************************************************
 *@file               :   bsp_storage_flash.c
 *@brief              :   Bind the board QSPI storage Flash capability.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "board_resources.h"
#include "bsp_storage_flash.h"
#include "hg25q128b.h"
#include "plat_qspi.h"
#include "plat_sys.h"

static hg25q128b_device_t storage_flash_device;

_Static_assert(BSP_STORAGE_FLASH_CAPACITY_BYTES ==
                   HG25Q128B_CAPACITY_BYTES,
               "Storage Flash capacity mismatch");
_Static_assert(BSP_STORAGE_FLASH_PAGE_SIZE_BYTES ==
                   HG25Q128B_PAGE_SIZE_BYTES,
               "Storage Flash page size mismatch");
_Static_assert(BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES ==
                   HG25Q128B_SECTOR_SIZE_BYTES,
               "Storage Flash sector size mismatch");
_Static_assert(BSP_STORAGE_FLASH_JEDEC_ID_SIZE ==
                   HG25Q128B_JEDEC_ID_SIZE,
               "Storage Flash JEDEC ID size mismatch");

static hg25q128b_ret_t bsp_storage_flash_ret_from_platform(platform_err_t ret)
{
    switch(ret)
    {
        case PLATFORM_ERR_OK:
            return HG25Q128B_RET_OK;
        case PLATFORM_ERR_PARAM:
            return HG25Q128B_RET_PARAM;
        case PLATFORM_ERR_BUSY:
            return HG25Q128B_RET_BUSY;
        case PLATFORM_ERR_TIMEOUT:
            return HG25Q128B_RET_TIMEOUT;
        default:
            return HG25Q128B_RET_IO;
    }
}

static platform_err_t bsp_storage_flash_ret_to_platform(hg25q128b_ret_t ret)
{
    switch(ret)
    {
        case HG25Q128B_RET_OK:
            return PLATFORM_ERR_OK;
        case HG25Q128B_RET_PARAM:
            return PLATFORM_ERR_PARAM;
        case HG25Q128B_RET_BUSY:
            return PLATFORM_ERR_BUSY;
        case HG25Q128B_RET_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;
        default:
            return PLATFORM_ERR_HW;
    }
}

static hg25q128b_ret_t bsp_storage_flash_command_convert(
    const hg25q128b_command_t *p_source,
    plat_qspi_command_t       *p_target)
{
    if((NULL == p_source) || (NULL == p_target))
    {
        return HG25Q128B_RET_PARAM;
    }

    p_target->instruction = p_source->instruction;
    p_target->address = p_source->address;
    p_target->address_size = p_source->address_size;
    p_target->dummy_cycles = p_source->dummy_cycles;
    switch(p_source->transfer_mode)
    {
        case HG25Q128B_TRANSFER_1_1_1:
            p_target->transfer_mode = PLAT_QSPI_TRANSFER_1_1_1;
            break;
        case HG25Q128B_TRANSFER_1_1_4:
            p_target->transfer_mode = PLAT_QSPI_TRANSFER_1_1_4;
            break;
        default:
            return HG25Q128B_RET_PARAM;
    }
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t bsp_storage_flash_command(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    uint32_t                    timeout_ms)
{
    plat_qspi_command_t command;
    hg25q128b_ret_t ret;

    (void)p_context;
    ret = bsp_storage_flash_command_convert(p_command, &command);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    return bsp_storage_flash_ret_from_platform(
        plat_qspi_command(BOARD_QSPI_STORAGE, &command, timeout_ms));
}

static hg25q128b_ret_t bsp_storage_flash_qspi_read(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    uint8_t                    *p_data,
    uint32_t                    data_size,
    uint32_t                    timeout_ms)
{
    plat_qspi_command_t command;
    hg25q128b_ret_t ret;

    (void)p_context;
    ret = bsp_storage_flash_command_convert(p_command, &command);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    return bsp_storage_flash_ret_from_platform(
        plat_qspi_read(BOARD_QSPI_STORAGE,
                       &command,
                       p_data,
                       data_size,
                       timeout_ms));
}

static hg25q128b_ret_t bsp_storage_flash_qspi_write(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    const uint8_t              *p_data,
    uint32_t                    data_size,
    uint32_t                    timeout_ms)
{
    plat_qspi_command_t command;
    hg25q128b_ret_t ret;

    (void)p_context;
    ret = bsp_storage_flash_command_convert(p_command, &command);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    return bsp_storage_flash_ret_from_platform(
        plat_qspi_write(BOARD_QSPI_STORAGE,
                        &command,
                        p_data,
                        data_size,
                        timeout_ms));
}

static uint32_t bsp_storage_flash_tick_ms(void *p_context)
{
    (void)p_context;
    return plat_tick_get_ms();
}

static void bsp_storage_flash_delay_ms(void *p_context, uint32_t delay_ms)
{
    (void)p_context;
    plat_delay_ms(delay_ms);
}

platform_err_t bsp_storage_flash_init(uint32_t timeout_ms)
{
    static const hg25q128b_io_t io =
    {
        bsp_storage_flash_command,
        bsp_storage_flash_qspi_read,
        bsp_storage_flash_qspi_write,
        bsp_storage_flash_tick_ms,
        bsp_storage_flash_delay_ms,
        NULL
    };

    return bsp_storage_flash_ret_to_platform(
        hg25q128b_init(&storage_flash_device, &io, timeout_ms));
}

platform_err_t bsp_storage_flash_info_get(
    bsp_storage_flash_info_t *p_info)
{
    hg25q128b_ret_t ret;

    if(NULL == p_info)
    {
        return PLATFORM_ERR_PARAM;
    }
    p_info->capacity_bytes = 0U;
    p_info->page_size_bytes = 0U;
    p_info->sector_size_bytes = 0U;
    p_info->jedec_id[0] = 0U;
    p_info->jedec_id[1] = 0U;
    p_info->jedec_id[2] = 0U;
    ret = hg25q128b_jedec_id_get(&storage_flash_device,
                                 p_info->jedec_id,
                                 BSP_STORAGE_FLASH_JEDEC_ID_SIZE);
    if(HG25Q128B_RET_OK == ret)
    {
        p_info->capacity_bytes = BSP_STORAGE_FLASH_CAPACITY_BYTES;
        p_info->page_size_bytes = BSP_STORAGE_FLASH_PAGE_SIZE_BYTES;
        p_info->sector_size_bytes = BSP_STORAGE_FLASH_SECTOR_SIZE_BYTES;
    }
    return bsp_storage_flash_ret_to_platform(ret);
}

platform_err_t bsp_storage_flash_read(uint32_t address,
                                      uint8_t *p_data,
                                      uint32_t data_size,
                                      uint32_t timeout_ms)
{
    return bsp_storage_flash_ret_to_platform(
        hg25q128b_read(&storage_flash_device,
                       address,
                       p_data,
                       data_size,
                       timeout_ms));
}

platform_err_t bsp_storage_flash_program(uint32_t address,
                                         const uint8_t *p_data,
                                         uint32_t data_size,
                                         uint32_t timeout_ms)
{
    return bsp_storage_flash_ret_to_platform(
        hg25q128b_program(&storage_flash_device,
                          address,
                          p_data,
                          data_size,
                          timeout_ms));
}

platform_err_t bsp_storage_flash_sector_erase(uint32_t address,
                                              uint32_t timeout_ms)
{
    return bsp_storage_flash_ret_to_platform(
        hg25q128b_sector_erase(&storage_flash_device,
                               address,
                               timeout_ms));
}
