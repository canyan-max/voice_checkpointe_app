/**
 ******************************************************************************
 *@file               :   hg25q128b.h
 *@brief              :   Portable HG25Q128B serial NOR Flash driver.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef HG25Q128B_H
#define HG25Q128B_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define HG25Q128B_CAPACITY_BYTES       (0x01000000UL)
#define HG25Q128B_PAGE_SIZE_BYTES      (256U)
#define HG25Q128B_SECTOR_SIZE_BYTES    (4096U)
#define HG25Q128B_JEDEC_ID_SIZE        (3U)

typedef enum HG25Q128B_RET_T
{
    HG25Q128B_RET_OK = 0U,
    HG25Q128B_RET_PARAM,
    HG25Q128B_RET_IO,
    HG25Q128B_RET_BUSY,
    HG25Q128B_RET_TIMEOUT,
    HG25Q128B_RET_NOT_READY,
    HG25Q128B_RET_DEVICE
} hg25q128b_ret_t;

typedef enum HG25Q128B_TRANSFER_MODE_T
{
    HG25Q128B_TRANSFER_1_1_1 = 0U,
    HG25Q128B_TRANSFER_1_1_4
} hg25q128b_transfer_mode_t;

typedef struct HG25Q128B_COMMAND_T
{
    uint8_t                     instruction;
    uint32_t                    address;
    uint8_t                     address_size;
    uint8_t                     dummy_cycles;
    hg25q128b_transfer_mode_t   transfer_mode;
} hg25q128b_command_t;

typedef hg25q128b_ret_t (*hg25q128b_command_fn_t)(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    uint32_t                    timeout_ms);

typedef hg25q128b_ret_t (*hg25q128b_read_t)(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    uint8_t                    *p_data,
    uint32_t                    data_size,
    uint32_t                    timeout_ms);

typedef hg25q128b_ret_t (*hg25q128b_write_t)(
    void                       *p_context,
    const hg25q128b_command_t  *p_command,
    const uint8_t              *p_data,
    uint32_t                    data_size,
    uint32_t                    timeout_ms);

typedef uint32_t (*hg25q128b_tick_ms_t)(void *p_context);
typedef void (*hg25q128b_delay_ms_t)(void *p_context, uint32_t delay_ms);

typedef struct HG25Q128B_IO_T
{
    hg25q128b_command_fn_t command;
    hg25q128b_read_t       read;
    hg25q128b_write_t      write;
    hg25q128b_tick_ms_t    tick_ms;
    hg25q128b_delay_ms_t   delay_ms;
    void                  *p_context;
} hg25q128b_io_t;

typedef struct HG25Q128B_DEVICE_T
{
    hg25q128b_io_t io;
    uint8_t        jedec_id[HG25Q128B_JEDEC_ID_SIZE];
    uint8_t        is_initialized;
} hg25q128b_device_t;

hg25q128b_ret_t hg25q128b_init(hg25q128b_device_t   *p_device,
                                const hg25q128b_io_t *p_io,
                                uint32_t               timeout_ms);

hg25q128b_ret_t hg25q128b_jedec_id_get(
    const hg25q128b_device_t *p_device,
    uint8_t                   *p_id,
    uint8_t                    id_size);

/**
 * @brief Read an arbitrary range using 0x6B, 8 dummy cycles and 1-1-4.
 */
hg25q128b_ret_t hg25q128b_read(hg25q128b_device_t *p_device,
                                uint32_t             address,
                                uint8_t             *p_data,
                                uint32_t             data_size,
                                uint32_t             timeout_ms);

/**
 * @brief Program an arbitrary range, splitting at 256-byte page boundaries.
 * @note The target range must have been erased by the caller. Programming can
 *       only change bits from 1 to 0.
 */
hg25q128b_ret_t hg25q128b_program(hg25q128b_device_t *p_device,
                                   uint32_t             address,
                                   const uint8_t       *p_data,
                                   uint32_t             data_size,
                                   uint32_t             timeout_ms);

/**
 * @brief Erase one aligned 4 KiB sector.
 */
hg25q128b_ret_t hg25q128b_sector_erase(hg25q128b_device_t *p_device,
                                        uint32_t             address,
                                        uint32_t             timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* HG25Q128B_H */
