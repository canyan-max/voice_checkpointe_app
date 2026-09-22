/**
 ******************************************************************************
 *@file               :   plat_qspi.h
 *@brief              :   MCU-independent blocking QSPI command interface.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef PLAT_QSPI_H
#define PLAT_QSPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"
#include "plat_resource.h"

typedef enum PLAT_QSPI_TRANSFER_MODE_T
{
    PLAT_QSPI_TRANSFER_1_1_1 = 0,
    PLAT_QSPI_TRANSFER_1_1_2,
    PLAT_QSPI_TRANSFER_1_1_4,
    PLAT_QSPI_TRANSFER_1_2_2,
    PLAT_QSPI_TRANSFER_1_4_4,
    PLAT_QSPI_TRANSFER_2_2_2,
    PLAT_QSPI_TRANSFER_4_4_4,
    PLAT_QSPI_TRANSFER_MODE_NUM
} plat_qspi_transfer_mode_t;

typedef struct PLAT_QSPI_COMMAND_T
{
    uint8_t                   instruction;
    uint32_t                  address;
    uint8_t                   address_size;
    uint8_t                   dummy_cycles;
    plat_qspi_transfer_mode_t transfer_mode;
} plat_qspi_command_t;

/**
 * @brief Send a command without a data phase.
 * @note The QSPI peripheral and pins must already be initialized by the board
 *       startup code. Callers sharing one controller must serialize access.
 */
platform_err_t plat_qspi_command(plat_qspi_id_t            id,
                                 const plat_qspi_command_t *p_command,
                                 uint32_t                   timeout_ms);

/**
 * @brief Send a command and read its data phase using polling.
 * @param timeout_ms Total timeout for the complete transaction.
 */
platform_err_t plat_qspi_read(plat_qspi_id_t            id,
                              const plat_qspi_command_t *p_command,
                              uint8_t                   *p_data,
                              uint32_t                   data_size,
                              uint32_t                   timeout_ms);

/**
 * @brief Send a command followed by its data phase using polling.
 * @param timeout_ms Total timeout for the complete transaction.
 * @note Device-specific write-enable, page-boundary, busy-wait and protection
 *       handling remain the responsibility of the ExternalChip driver.
 */
platform_err_t plat_qspi_write(plat_qspi_id_t            id,
                               const plat_qspi_command_t *p_command,
                               const uint8_t             *p_data,
                               uint32_t                   data_size,
                               uint32_t                   timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_QSPI_H */
