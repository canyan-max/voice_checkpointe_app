/**
 ******************************************************************************
 *@file               :   kfifo.h
 * 
 *@brief              :   Provide the HAL APIs of description.
 * 
 *@version            :   V1.0 
 * 
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef KFIFO_H
#define KFIFO_H

/* Includes -----------------------------------------------------------------*/
#include <stdint.h> /* stdint lib header file. */
#include <stddef.h> /* stdint lib header file. */

/* define -------------------------------------------------------------------*/

/* typedef ------------------------------------------------------------------*/
typedef struct KFIFO_T
{
    uint8_t *buffer;
    uint32_t size;
    uint32_t in;
    uint32_t out;
} kfifo_t;
/* Exported types -----------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
/**
 * @brief            :  [kfifo_init]
 * @retval           :  [0 - success, 1 - error]
 * @param[in]        :  [kfifo_t* fifo,uint8_t* buffer,uint32_t size]
 */
uint8_t kfifo_init(kfifo_t *fifo, uint8_t *buffer, uint32_t size);

/**
 * @brief            :  [kfifo_put]
 * @retval           :  [number of bytes written]
 * @param[in]        :  [kfifo_t* fifo,const uint8_t* data,uint32_t len]
 */
uint32_t kfifo_put(kfifo_t *fifo, const uint8_t *data, uint32_t len);

/**
 * @brief            :  [kfifo_get]
 * @retval           :  [number of bytes read]
 * @param[in]        :  [kfifo_t* fifo,uint8_t* data,uint32_t len]
 */
uint32_t kfifo_get(kfifo_t *fifo, uint8_t *data, uint32_t len);

/**
 * @brief            :  [kfifo_advance_in]
 * @retval           :  [number of bytes read]
 * @param[in]        :  [kfifo_t *fifo, uint32_t len)]
 */
uint8_t kfifo_advance_in(kfifo_t *fifo, uint32_t len);

/**
 * @brief            :  [kfifo_get_buffer_size]
 * @retval           :  [0 successful]
 * @param[in]        :  [kfifo_t *fifo uint32_t *buffer_size]
 */
uint8_t kfifo_get_buffer_size(kfifo_t *fifo , uint32_t *buffer_size);

/**
 * @brief            :  [kfifo_len]
 * @retval           :  [number of bytes in the fifo]
 * @param[in]        :  [kfifo_t* fifo]
 */
static inline uint32_t kfifo_len(const kfifo_t *fifo)
{
    return fifo->in - fifo->out;
}

/**
 * @brief            :  [kfifo_avail]
 * @retval           :  [number of bytes available in the fifo]
 * @param[in]        :  [kfifo_t* fifo]
 */
static inline uint32_t kfifo_avail(const kfifo_t *fifo)
{
    return fifo->size - kfifo_len(fifo);
}

/**
 * @brief            :  [kfifo_contig_write_space]
 * @retval           :  [number of bytes available for writing]
 * @param[in]        :  [kfifo_t* fifo]
 */
static inline uint32_t kfifo_contig_write_space(const kfifo_t *fifo)
{
    return fifo->size - (fifo->in & (fifo->size - 1));
}

/**
 * @brief            :  [kfifo_contig_read_data]
 * @retval           :  [number of bytes available for reading]
 * @param[in]        :  [kfifo_t* fifo]
 */
static inline uint32_t kfifo_contig_read_data(const kfifo_t *fifo)
{
    return fifo->size - (fifo->out & (fifo->size - 1));
}

/**
 * @brief            :  [kfifo_advance_out]
 * @retval           :  [0 is successful]
 * @param[in]        :  [kfifo_t *fifo, uint32_t len]
 */
uint8_t kfifo_advance_out(kfifo_t *fifo, uint32_t len);

#endif /* KFIFO_H */
