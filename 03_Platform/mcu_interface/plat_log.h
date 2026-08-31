/**
 ******************************************************************************
 *@file               :   plat_log.h
 *@brief              :   Provide backend-independent Platform log APIs.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef PLAT_LOG_H
#define PLAT_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h" /* platform error code header file. */
/* typedef ------------------------------------------------------------------*/
typedef enum PLAT_LOG_LEVEL_T
{
    PLAT_LOG_LEVEL_ERROR = 0U,
    PLAT_LOG_LEVEL_WARN,
    PLAT_LOG_LEVEL_INFO,
    PLAT_LOG_LEVEL_DEBUG,
    PLAT_LOG_LEVEL_VERBOSE,
    PLAT_LOG_LEVEL_NUM
} plat_log_level_t;

/* variables ----------------------------------------------------------------*/

/* functions ----------------------------------------------------------------*/
/**
  * @brief            : Initialize and start the selected log backend.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_HW.
  * @note             : Call once before using the log output APIs.
  */
platform_err_t plat_log_init(void);

/**
  * @brief            : Format and output one Platform log message.
  * @param[in]        : level Platform log level.
  * @param[in]        : p_file Source file name supplied by the log macro.
  * @param[in]        : p_func Source function name supplied by the log macro.
  * @param[in]        : line Source line supplied by the log macro.
  * @param[in]        : p_format printf-style format string.
  * @note             : Call from task context. Concurrent output is serialized
  *                     by the selected backend. Long messages may be truncated.
  */
void plat_log_output(plat_log_level_t level,
                     const char      *p_file,
                     const char      *p_func,
                     uint32_t         line,
                     const char      *p_format,
                     ...);

/**
  * @brief            : Output a buffer in hexadecimal form.
  * @param[in]        : p_name Display name for the buffer.
  * @param[in]        : width Number of bytes displayed per line.
  * @param[in]        : p_buf Buffer to display.
  * @param[in]        : size Buffer size in bytes.
  * @note             : Call from task context.
  */
void plat_log_hexdump_output(const char *p_name,
                             uint8_t     width,
                             const void *p_buf,
                             uint16_t    size);

/* define -------------------------------------------------------------------*/
#ifdef USE_DEBUG_LOG
#define plat_log_v(...) \
    plat_log_output(PLAT_LOG_LEVEL_VERBOSE, \
                    __FILE__, __func__, (uint32_t)__LINE__, __VA_ARGS__)
#define plat_log_d(...) \
    plat_log_output(PLAT_LOG_LEVEL_DEBUG, \
                    __FILE__, __func__, (uint32_t)__LINE__, __VA_ARGS__)
#define plat_log_i(...) \
    plat_log_output(PLAT_LOG_LEVEL_INFO, \
                    __FILE__, __func__, (uint32_t)__LINE__, __VA_ARGS__)
#define plat_log_w(...) \
    plat_log_output(PLAT_LOG_LEVEL_WARN, \
                    __FILE__, __func__, (uint32_t)__LINE__, __VA_ARGS__)
#define plat_log_e(...) \
    plat_log_output(PLAT_LOG_LEVEL_ERROR, \
                    __FILE__, __func__, (uint32_t)__LINE__, __VA_ARGS__)
#define plat_log_hexdump(name, width, buf, size) \
    plat_log_hexdump_output(name, width, buf, size)
#else
#define plat_log_v(...)  ((void)0)
#define plat_log_d(...)  ((void)0)
#define plat_log_i(...)  ((void)0)
#define plat_log_w(...)  ((void)0)
#define plat_log_e(...)  ((void)0)
#define plat_log_hexdump(name, width, buf, size) ((void)0)
#endif /* USE_DEBUG_LOG */

#ifdef __cplusplus
}
#endif

#endif /* PLAT_LOG_H */
