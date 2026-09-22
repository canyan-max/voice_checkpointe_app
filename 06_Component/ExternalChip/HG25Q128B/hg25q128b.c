/**
 ******************************************************************************
 *@file               :   hg25q128b.c
 *@brief              :   Implement HG25Q128B serial NOR Flash operations.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "hg25q128b.h"

#define HG25Q128B_COMMAND_WREN             (0x06U)
#define HG25Q128B_COMMAND_RDSR             (0x05U)
#define HG25Q128B_COMMAND_WRSR             (0x01U)
#define HG25Q128B_COMMAND_RDID             (0x9FU)
#define HG25Q128B_COMMAND_QUAD_READ        (0x6BU)
#define HG25Q128B_COMMAND_PAGE_PROGRAM     (0x02U)
#define HG25Q128B_COMMAND_SECTOR_ERASE     (0x20U)
#define HG25Q128B_ADDRESS_SIZE             (3U)
#define HG25Q128B_QUAD_READ_DUMMY_CYCLES   (8U)
#define HG25Q128B_STATUS_WIP_MASK          (0x01U)
#define HG25Q128B_STATUS_WEL_MASK          (0x02U)
#define HG25Q128B_STATUS_QE_MASK           (0x40U)
#define HG25Q128B_MANUFACTURER_ID          (0xC2U)
#define HG25Q128B_MEMORY_TYPE              (0x20U)
#define HG25Q128B_MEMORY_DENSITY           (0x18U)
#define HG25Q128B_POWER_UP_DELAY_MS        (1U)
#define HG25Q128B_POLL_DELAY_MS            (1U)

static hg25q128b_ret_t hg25q128b_io_validate(const hg25q128b_io_t *p_io)
{
    if((NULL == p_io) || (NULL == p_io->command) ||
       (NULL == p_io->read) || (NULL == p_io->write) ||
       (NULL == p_io->tick_ms) || (NULL == p_io->delay_ms))
    {
        return HG25Q128B_RET_PARAM;
    }
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t hg25q128b_validate(
    const hg25q128b_device_t *p_device)
{
    if(NULL == p_device)
    {
        return HG25Q128B_RET_PARAM;
    }
    if((0U == p_device->is_initialized) ||
       (HG25Q128B_RET_OK != hg25q128b_io_validate(&p_device->io)))
    {
        return HG25Q128B_RET_NOT_READY;
    }
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t hg25q128b_range_validate(uint32_t address,
                                                 uint32_t data_size)
{
    if((0U == data_size) || (address >= HG25Q128B_CAPACITY_BYTES) ||
       (data_size > (HG25Q128B_CAPACITY_BYTES - address)))
    {
        return HG25Q128B_RET_PARAM;
    }
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t hg25q128b_timeout_remaining(
    hg25q128b_device_t *p_device,
    uint32_t             start_ms,
    uint32_t             timeout_ms,
    uint32_t            *p_remaining_ms)
{
    uint32_t elapsed_ms;

    if((NULL == p_remaining_ms) || (0U == timeout_ms))
    {
        return HG25Q128B_RET_PARAM;
    }
    elapsed_ms = p_device->io.tick_ms(p_device->io.p_context) - start_ms;
    if(elapsed_ms >= timeout_ms)
    {
        return HG25Q128B_RET_TIMEOUT;
    }
    *p_remaining_ms = timeout_ms - elapsed_ms;
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t hg25q128b_status_read_raw(
    hg25q128b_device_t *p_device,
    uint8_t            *p_status,
    uint32_t            timeout_ms)
{
    const hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_RDSR,
        0U,
        0U,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };

    return p_device->io.read(p_device->io.p_context,
                             &command,
                             p_status,
                             1U,
                             timeout_ms);
}

static hg25q128b_ret_t hg25q128b_wait_ready(
    hg25q128b_device_t *p_device,
    uint32_t             start_ms,
    uint32_t             timeout_ms)
{
    hg25q128b_ret_t ret;
    uint32_t remaining_ms;
    uint8_t status;

    for(;;)
    {
        ret = hg25q128b_timeout_remaining(p_device,
                                          start_ms,
                                          timeout_ms,
                                          &remaining_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        ret = hg25q128b_status_read_raw(p_device,
                                        &status,
                                        remaining_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        if(0U == (status & HG25Q128B_STATUS_WIP_MASK))
        {
            return HG25Q128B_RET_OK;
        }
        p_device->io.delay_ms(p_device->io.p_context,
                              HG25Q128B_POLL_DELAY_MS);
    }
}

static hg25q128b_ret_t hg25q128b_write_enable(
    hg25q128b_device_t *p_device,
    uint32_t             start_ms,
    uint32_t             timeout_ms)
{
    const hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_WREN,
        0U,
        0U,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };
    hg25q128b_ret_t ret;
    uint32_t remaining_ms;
    uint8_t status;

    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = p_device->io.command(p_device->io.p_context,
                               &command,
                               remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_status_read_raw(p_device, &status, remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    if(0U == (status & HG25Q128B_STATUS_WEL_MASK))
    {
        return HG25Q128B_RET_IO;
    }
    return HG25Q128B_RET_OK;
}

static hg25q128b_ret_t hg25q128b_quad_enable(
    hg25q128b_device_t *p_device,
    uint32_t             start_ms,
    uint32_t             timeout_ms)
{
    const hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_WRSR,
        0U,
        0U,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };
    hg25q128b_ret_t ret;
    uint32_t remaining_ms;
    uint8_t status;
    uint8_t new_status;

    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_status_read_raw(p_device, &status, remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    if(0U != (status & HG25Q128B_STATUS_QE_MASK))
    {
        return HG25Q128B_RET_OK;
    }

    ret = hg25q128b_write_enable(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    new_status = status | HG25Q128B_STATUS_QE_MASK;
    ret = p_device->io.write(p_device->io.p_context,
                             &command,
                             &new_status,
                             1U,
                             remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_status_read_raw(p_device, &status, remaining_ms);
    if((HG25Q128B_RET_OK == ret) &&
       (0U == (status & HG25Q128B_STATUS_QE_MASK)))
    {
        ret = HG25Q128B_RET_IO;
    }
    return ret;
}

hg25q128b_ret_t hg25q128b_init(hg25q128b_device_t   *p_device,
                                const hg25q128b_io_t *p_io,
                                uint32_t               timeout_ms)
{
    const hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_RDID,
        0U,
        0U,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };
    hg25q128b_ret_t ret;
    uint32_t start_ms;
    uint32_t remaining_ms;

    if((NULL == p_device) || (0U == timeout_ms) ||
       (HG25Q128B_RET_OK != hg25q128b_io_validate(p_io)))
    {
        return HG25Q128B_RET_PARAM;
    }

    p_device->io = *p_io;
    p_device->is_initialized = 0U;
    p_device->io.delay_ms(p_device->io.p_context,
                          HG25Q128B_POWER_UP_DELAY_MS);
    start_ms = p_device->io.tick_ms(p_device->io.p_context);
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK == ret)
    {
        ret = p_device->io.read(p_device->io.p_context,
                                &command,
                                p_device->jedec_id,
                                HG25Q128B_JEDEC_ID_SIZE,
                                remaining_ms);
    }
    if((HG25Q128B_RET_OK == ret) &&
       ((HG25Q128B_MANUFACTURER_ID != p_device->jedec_id[0]) ||
        (HG25Q128B_MEMORY_TYPE != p_device->jedec_id[1]) ||
        (HG25Q128B_MEMORY_DENSITY != p_device->jedec_id[2])))
    {
        ret = HG25Q128B_RET_DEVICE;
    }
    if(HG25Q128B_RET_OK == ret)
    {
        ret = hg25q128b_quad_enable(p_device, start_ms, timeout_ms);
    }
    if(HG25Q128B_RET_OK == ret)
    {
        p_device->is_initialized = 1U;
    }
    return ret;
}

hg25q128b_ret_t hg25q128b_jedec_id_get(
    const hg25q128b_device_t *p_device,
    uint8_t                   *p_id,
    uint8_t                    id_size)
{
    hg25q128b_ret_t ret;
    uint8_t index;

    if((NULL == p_id) || (HG25Q128B_JEDEC_ID_SIZE != id_size))
    {
        return HG25Q128B_RET_PARAM;
    }
    ret = hg25q128b_validate(p_device);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    for(index = 0U; index < HG25Q128B_JEDEC_ID_SIZE; index++)
    {
        p_id[index] = p_device->jedec_id[index];
    }
    return HG25Q128B_RET_OK;
}

hg25q128b_ret_t hg25q128b_read(hg25q128b_device_t *p_device,
                                uint32_t             address,
                                uint8_t             *p_data,
                                uint32_t             data_size,
                                uint32_t             timeout_ms)
{
    hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_QUAD_READ,
        address,
        HG25Q128B_ADDRESS_SIZE,
        HG25Q128B_QUAD_READ_DUMMY_CYCLES,
        HG25Q128B_TRANSFER_1_1_4
    };
    hg25q128b_ret_t ret;
    uint32_t start_ms;
    uint32_t remaining_ms;

    if((NULL == p_data) || (0U == timeout_ms))
    {
        return HG25Q128B_RET_PARAM;
    }
    ret = hg25q128b_validate(p_device);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_range_validate(address, data_size);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    start_ms = p_device->io.tick_ms(p_device->io.p_context);
    ret = hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    return p_device->io.read(p_device->io.p_context,
                             &command,
                             p_data,
                             data_size,
                             remaining_ms);
}

hg25q128b_ret_t hg25q128b_program(hg25q128b_device_t *p_device,
                                   uint32_t             address,
                                   const uint8_t       *p_data,
                                   uint32_t             data_size,
                                   uint32_t             timeout_ms)
{
    hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_PAGE_PROGRAM,
        address,
        HG25Q128B_ADDRESS_SIZE,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };
    hg25q128b_ret_t ret;
    uint32_t start_ms;
    uint32_t remaining_ms;
    uint32_t page_remaining;
    uint32_t chunk_size;

    if((NULL == p_data) || (0U == timeout_ms))
    {
        return HG25Q128B_RET_PARAM;
    }
    ret = hg25q128b_validate(p_device);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_range_validate(address, data_size);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }

    start_ms = p_device->io.tick_ms(p_device->io.p_context);
    ret = hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    while(0U != data_size)
    {
        page_remaining = HG25Q128B_PAGE_SIZE_BYTES -
                         (address % HG25Q128B_PAGE_SIZE_BYTES);
        chunk_size = (data_size < page_remaining) ? data_size : page_remaining;
        ret = hg25q128b_write_enable(p_device, start_ms, timeout_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        ret = hg25q128b_timeout_remaining(p_device,
                                          start_ms,
                                          timeout_ms,
                                          &remaining_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        command.address = address;
        ret = p_device->io.write(p_device->io.p_context,
                                 &command,
                                 p_data,
                                 chunk_size,
                                 remaining_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        ret = hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
        if(HG25Q128B_RET_OK != ret)
        {
            return ret;
        }
        address += chunk_size;
        p_data += chunk_size;
        data_size -= chunk_size;
    }
    return HG25Q128B_RET_OK;
}

hg25q128b_ret_t hg25q128b_sector_erase(hg25q128b_device_t *p_device,
                                        uint32_t             address,
                                        uint32_t             timeout_ms)
{
    hg25q128b_command_t command =
    {
        HG25Q128B_COMMAND_SECTOR_ERASE,
        address,
        HG25Q128B_ADDRESS_SIZE,
        0U,
        HG25Q128B_TRANSFER_1_1_1
    };
    hg25q128b_ret_t ret;
    uint32_t start_ms;
    uint32_t remaining_ms;

    if((0U == timeout_ms) ||
       (0U != (address % HG25Q128B_SECTOR_SIZE_BYTES)) ||
       (address >= HG25Q128B_CAPACITY_BYTES))
    {
        return HG25Q128B_RET_PARAM;
    }
    ret = hg25q128b_validate(p_device);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }

    start_ms = p_device->io.tick_ms(p_device->io.p_context);
    ret = hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_write_enable(p_device, start_ms, timeout_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = hg25q128b_timeout_remaining(p_device,
                                      start_ms,
                                      timeout_ms,
                                      &remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    ret = p_device->io.command(p_device->io.p_context,
                               &command,
                               remaining_ms);
    if(HG25Q128B_RET_OK != ret)
    {
        return ret;
    }
    return hg25q128b_wait_ready(p_device, start_ms, timeout_ms);
}
