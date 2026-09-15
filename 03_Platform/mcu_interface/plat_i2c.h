/**
 ******************************************************************************
 *@file               :   plat_i2c.h
 *@brief              :   MCU-independent blocking I2C master interface.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef PLAT_I2C_H
#define PLAT_I2C_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "plat_error.h"
#include "plat_resource.h"

/**
  * @brief Write one I2C frame in polling mode.
  * @param id Logical I2C bus ID.
  * @param address_7bit Unshifted 7-bit slave address (0x00..0x7F).
  * @param p_data Data bytes sent after the address.
  * @param size Number of bytes; valid range is 1..255.
  * @param timeout_ms Total transaction timeout in milliseconds.
  * @note Callers sharing one bus must serialize access.
  */
platform_err_t plat_i2c_write(plat_i2c_id_t id,
                              uint8_t       address_7bit,
                              const uint8_t *p_data,
                              uint16_t      size,
                              uint32_t      timeout_ms);

/**
  * @brief Read one I2C frame in polling mode.
  * @param id Logical I2C bus ID.
  * @param address_7bit Unshifted 7-bit slave address (0x00..0x7F).
  * @param p_data Destination buffer.
  * @param size Number of bytes; valid range is 1..255.
  * @param timeout_ms Total transaction timeout in milliseconds.
  * @note Callers sharing one bus must serialize access.
  */
platform_err_t plat_i2c_read(plat_i2c_id_t id,
                             uint8_t       address_7bit,
                             uint8_t      *p_data,
                             uint16_t      size,
                             uint32_t      timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* PLAT_I2C_H */
