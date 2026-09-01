/**
 ******************************************************************************
 *@file               :   at32f435_log.c
 *@brief              :   Adapt Platform logging to EasyLogger.
 *@version            :   V1.0 
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "plat_log.h"
#ifdef USE_DEBUG_LOG
#include "elog.h"
#endif
/* define   -----------------------------------------------------------------*/
#define AT32F435_LOG_MESSAGE_SIZE  256U

/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/
#ifdef USE_DEBUG_LOG
static const uint8_t log_level_map[PLAT_LOG_LEVEL_NUM] =
{
    [PLAT_LOG_LEVEL_ERROR]   = ELOG_LVL_ERROR,
    [PLAT_LOG_LEVEL_WARN]    = ELOG_LVL_WARN,
    [PLAT_LOG_LEVEL_INFO]    = ELOG_LVL_INFO,
    [PLAT_LOG_LEVEL_DEBUG]   = ELOG_LVL_DEBUG,
    [PLAT_LOG_LEVEL_VERBOSE] = ELOG_LVL_VERBOSE
};

static const char *log_level_tag[PLAT_LOG_LEVEL_NUM] =
{
    [PLAT_LOG_LEVEL_ERROR]   = "E",
    [PLAT_LOG_LEVEL_WARN]    = "W",
    [PLAT_LOG_LEVEL_INFO]    = "I",
    [PLAT_LOG_LEVEL_DEBUG]   = "D",
    [PLAT_LOG_LEVEL_VERBOSE] = "V"
};

_Static_assert((sizeof(log_level_map) / sizeof(log_level_map[0])) ==
                   PLAT_LOG_LEVEL_NUM,
               "Log level map size mismatch");
_Static_assert((sizeof(log_level_tag) / sizeof(log_level_tag[0])) ==
                   PLAT_LOG_LEVEL_NUM,
               "Log level tag size mismatch");
#endif

/* private  functions  ------------------------------------------------------*/

/* exported functions -------------------------------------------------------*/
platform_err_t plat_log_init(void)
{
#ifdef USE_DEBUG_LOG
    if(ELOG_NO_ERR != elog_init())
    {
        return PLATFORM_ERR_HW;
    }
    elog_start();
#endif
    return PLATFORM_ERR_OK;
}

void plat_log_output(plat_log_level_t level,
                     const char      *p_file,
                     const char      *p_func,
                     uint32_t         line,
                     const char      *p_format,
                     ...)
{
#ifdef USE_DEBUG_LOG
    char message[AT32F435_LOG_MESSAGE_SIZE];
    va_list args;
    int32_t format_ret;

    if(((uint32_t)level >= (uint32_t)PLAT_LOG_LEVEL_NUM) ||
       (NULL == p_file) ||
       (NULL == p_func) ||
       (NULL == p_format))
    {
        return;
    }

    va_start(args, p_format);
    format_ret = (int32_t)vsnprintf(message,
                                    sizeof(message),
                                    p_format,
                                    args);
    va_end(args);
    if(format_ret < 0)
    {
        return;
    }

    elog_output(log_level_map[level],
                log_level_tag[level],
                p_file,
                p_func,
                (long)line,
                "%s",
                message);
#else
    (void)level;
    (void)p_file;
    (void)p_func;
    (void)line;
    (void)p_format;
#endif
}

void plat_log_hexdump_output(const char *p_name,
                             uint8_t     width,
                             const void *p_buf,
                             uint16_t    size)
{
#ifdef USE_DEBUG_LOG
    if((NULL == p_name) ||
       (0U == width) ||
       (NULL == p_buf) ||
       (0U == size))
    {
        return;
    }

    elog_hexdump(p_name, width, p_buf, size);
#else
    (void)p_name;
    (void)width;
    (void)p_buf;
    (void)size;
#endif
}
/* end of  file -------------------------------------------------------------*/
