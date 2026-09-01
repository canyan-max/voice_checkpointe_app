/**
 ******************************************************************************
 *@file               :   tpa3116.h
 *@brief              :   Provide a portable TPA3116 control interface.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef TPA3116_H
#define TPA3116_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>

/* typedef ------------------------------------------------------------------*/
typedef enum TPA3116_RET_T
{
    TPA3116_RET_OK = 0U,
    TPA3116_RET_PARAM,
    TPA3116_RET_IO,
    TPA3116_RET_NOT_READY
} tpa3116_ret_t;

typedef enum TPA3116_PIN_T
{
    TPA3116_PIN_SDZ = 0U,
    TPA3116_PIN_MUTE,
    TPA3116_PIN_FAULTZ,
    TPA3116_PIN_NUM
} tpa3116_pin_t;

typedef tpa3116_ret_t (*tpa3116_pin_write_t)(void          *p_context,
                                             tpa3116_pin_t  pin,
                                             uint8_t        level);
typedef tpa3116_ret_t (*tpa3116_pin_read_t)(void          *p_context,
                                            tpa3116_pin_t  pin,
                                            uint8_t       *p_level);

typedef struct TPA3116_IO_T
{
    tpa3116_pin_write_t write;
    tpa3116_pin_read_t  read;
    void               *p_context;
} tpa3116_io_t;

typedef struct TPA3116_DEVICE_T
{
    tpa3116_io_t io;
    uint8_t      is_initialized;
} tpa3116_device_t;

/* function  ----------------------------------------------------------------*/
/**
  * @brief            : Bind the device to its board IO implementation.
  * @retval           : TPA3116_RET_OK or TPA3116_RET_PARAM.
  */
tpa3116_ret_t tpa3116_init(tpa3116_device_t   *p_device,
                           const tpa3116_io_t *p_io);

/**
  * @brief            : Enable the amplifier while keeping its outputs muted.
  * @retval           : TPA3116_RET_OK, TPA3116_RET_PARAM,
  *                     TPA3116_RET_IO or TPA3116_RET_NOT_READY.
  */
tpa3116_ret_t tpa3116_enable(tpa3116_device_t *p_device);

/**
  * @brief            : Mute the outputs and place the amplifier in shutdown.
  * @retval           : TPA3116_RET_OK, TPA3116_RET_PARAM,
  *                     TPA3116_RET_IO or TPA3116_RET_NOT_READY.
  */
tpa3116_ret_t tpa3116_shutdown(tpa3116_device_t *p_device);

/**
  * @brief            : Set the amplifier mute state.
  * @param[in]        : is_muted 0 enables outputs; 1 makes outputs high-Z.
  */
tpa3116_ret_t tpa3116_mute_set(tpa3116_device_t *p_device,
                               uint8_t           is_muted);

/**
  * @brief            : Read the active-low FAULTZ signal as a logical fault.
  * @param[out]       : p_is_fault Set to 1 on fault, otherwise 0.
  */
tpa3116_ret_t tpa3116_fault_get(tpa3116_device_t *p_device,
                                uint8_t          *p_is_fault);

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* TPA3116_H */
