/**
 ******************************************************************************
 *@file               :   plat_spi.h
 *@brief              :   MCU-independent blocking SPI master interface.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef PLAT_SPI_H
#define PLAT_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"
#include "plat_resource.h"

/**
 * @brief Write a command and then read data in one SPI transaction.
 * @param id Logical SPI bus ID.
 * @param p_command Command bytes transmitted before the read phase.
 * @param command_size Number of command bytes; valid range is 1..65535.
 * @param p_data Destination for bytes received during the read phase.
 * @param data_size Number of bytes to read; valid range is 1..65535.
 * @param dummy_byte Byte transmitted to generate each read clock.
 * @param timeout_ms Total transaction timeout in milliseconds.
 * @note Hardware CS remains active across both phases. Received bytes from
 *       the command phase are discarded. Callers sharing one bus must
 *       serialize access.
 */
platform_err_t plat_spi_write_read(plat_spi_id_t id,
                                   const uint8_t *p_command,
                                   uint16_t       command_size,
                                   uint8_t       *p_data,
                                   uint16_t       data_size,
                                   uint8_t        dummy_byte,
                                   uint32_t       timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_SPI_H */
