/**
 ******************************************************************************
 *@file               :   cat5171.h
 *@brief              :   Portable CAT5171 digital potentiometer driver.
 *@version            :   V1.0
 ******************************************************************************
 */
#ifndef CAT5171_H
#define CAT5171_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum CAT5171_RET_T
{
    CAT5171_RET_OK = 0U,
    CAT5171_RET_PARAM,
    CAT5171_RET_IO,
    CAT5171_RET_BUSY,
    CAT5171_RET_TIMEOUT,
    CAT5171_RET_NOT_READY
} cat5171_ret_t;

typedef cat5171_ret_t (*cat5171_write_t)(void          *p_context,
                                         const uint8_t *p_data,
                                         uint16_t       size,
                                         uint32_t       timeout_ms);
typedef cat5171_ret_t (*cat5171_read_t)(void     *p_context,
                                        uint8_t  *p_data,
                                        uint16_t  size,
                                        uint32_t  timeout_ms);

typedef struct CAT5171_IO_T
{
    cat5171_write_t write;
    cat5171_read_t  read;
    void           *p_context;
} cat5171_io_t;

typedef struct CAT5171_DEVICE_T
{
    cat5171_io_t io;
    uint8_t      is_initialized;
} cat5171_device_t;

cat5171_ret_t cat5171_init(cat5171_device_t   *p_device,
                           const cat5171_io_t *p_io);

cat5171_ret_t cat5171_wiper_write(cat5171_device_t *p_device,
                                  uint8_t           position,
                                  uint32_t          timeout_ms);

cat5171_ret_t cat5171_wiper_read(cat5171_device_t *p_device,
                                 uint8_t          *p_position,
                                 uint32_t          timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* CAT5171_H */
