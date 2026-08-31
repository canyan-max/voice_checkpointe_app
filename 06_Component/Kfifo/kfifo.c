/**
 ******************************************************************************
 *@file               :   kfifo.c
 * 
 *@brief              :   Provide the HAL APIs of description.
 * 
 *@version            :   V1.0 
 * 
 *@note               :   1 tab == 4 spaces!  2026
 * 
 *@pardependencies    :   kfifo.c
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stdbool.h>
#include <string.h> /* stdint lib header file. */
#include "kfifo.h"  /* kfifo lib header file. */
/* define   -----------------------------------------------------------------*/
// #define MIN(a, b) \
//     ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* Private  functions  ------------------------------------------------------*/
/**
 * @brief            :  [check_fifo_size_pow_of_two
                        Because the value of a pow of 2 .
                        has only one bit set to 1 in binary.]
 * @retval           :  [true - success, false - error]
 * @param[in]        :  [uint32_t size]
 */
static inline bool check_fifo_size_pow_of_two(uint32_t size)
{
    return (size != 0) && ((size & (size - 1)) == 0);
}

/**
 * @brief            :  [min_u32]
 * @retval           :  [minimum of a and b]
 * @param[in]        :  [uint32_t a, uint32_t b]
 */
static inline uint32_t min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/* Exported functions -------------------------------------------------------*/
/**
 * @brief            :  [kfifo_init]
 * @retval           :  [0 - success, 1 - error]
 * @param[in]        :  [kfifo_t* fifo,uint8_t* buffer,uint32_t size]
 */
uint8_t kfifo_init(kfifo_t *fifo, uint8_t *buffer, uint32_t size)
{
    if(!check_fifo_size_pow_of_two(size))
    {
        return 1; /* Error: size is not a pow of 2 */
    }
    fifo->buffer = buffer;
    fifo->size   = size;
    fifo->in     = 0U;
    fifo->out    = 0U;
    return 0;
}

/**
 * @brief            :  [kfifo_put]
 * @retval           :  [number of bytes written]
 * @param[in]        :  [kfifo_t* fifo,const uint8_t* data,uint32_t len]
 */
uint32_t kfifo_put(kfifo_t *fifo, const uint8_t *data, uint32_t len)
{
    uint32_t l;
    len = min_u32(len, fifo->size - fifo->in + fifo->out);
    l   = min_u32(len, fifo->size - (fifo->in & (fifo->size - 1)));

    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), data, l);
    memcpy(fifo->buffer, data + l, len - l);
    fifo->in += len;
    return len;
}

/**
 * @brief            :  [kfifo_get]
 * @retval           :  [number of bytes read]
 * @param[in]        :  [kfifo_t* fifo,uint8_t* data,uint32_t len]
 */
uint32_t kfifo_get(kfifo_t *fifo, uint8_t *data, uint32_t len)
{
    uint32_t l;

    len = min_u32(len, fifo->in - fifo->out);
    l   = min_u32(len, fifo->size - (fifo->out & (fifo->size - 1)));

    memcpy(data, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
    memcpy(data + l, fifo->buffer, len - l);

    fifo->out += len;
    return len;
}

/**
 * @brief            :  [kfifo_peek]
 * @retval           :  [peek fifo data only head]
 * @param[in]        :  [kfifo_t* fifo,uint8_t* data]
 */
uint8_t kfifo_peek_out(kfifo_t *fifo, uint8_t *data)
{
    uint8_t offset = 0x00;
    if(NULL == fifo || NULL == data)
    {
        return 1U;
    }
    if(fifo->in == fifo->out)
    {
        return 2U;
    }
    offset = fifo->out &(fifo->size - 1);
    *data = offset ;
    return 0U;
}


/**
 * @brief            :  [kfifo_advance_in]
 * @retval           :  [0 is successful]
 * @param[in]        :  [kfifo_t *fifo, uint32_t len)]
 */
uint8_t kfifo_advance_in(kfifo_t *fifo, uint32_t len)
{

    if(NULL == fifo || 0 == len)
    {
        return 1U;
    }

    fifo->in += len;
    return 0U;
}

/**
 * @brief            :  [kfifo_get_buffer_size]
 * @retval           :  [0 successful]
 * @param[in]        :  [kfifo_t *fifo uint32_t *buffer_size]
 */
uint8_t kfifo_get_buffer_size(kfifo_t *fifo , uint32_t *buffer_size)
{

    if(NULL == fifo)
    {
        return 1U;
    }
    *buffer_size=fifo->size;
    return 0U;
}


/**
 * @brief            :  [kfifo_advance_out]
 * @retval           :  [0 is successful]
 * @param[in]        :  [kfifo_t *fifo, uint32_t len]
 */
uint8_t kfifo_advance_out(kfifo_t *fifo, uint32_t len)
{
    if (NULL == fifo || 0 == len)
    {
        return 1U;
    }
    fifo->out += len;
    return 0U;
}

/* end of  file -------------------------------------------------------------*/
