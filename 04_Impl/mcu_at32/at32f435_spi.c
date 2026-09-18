/**
 ******************************************************************************
 *@file               :   at32f435_spi.c
 *@brief              :   AT32F435 polling SPI master implementation.
 *@version            :   V1.0
 ******************************************************************************
 */

#include <stddef.h>
#include "board_at32f435_binding.h"
#include "board_resources.h"
#include "plat_spi.h"
#include "plat_sys.h"

#define AT32F435_SPI_ERROR_FLAGS \
    (SPI_MMERR_FLAG | SPI_I2S_ROERR_FLAG | SPI_CSPAS_FLAG)

typedef struct AT32F435_SPI_CFG_T
{
    spi_type  *p_instance;
    gpio_type *p_cs_port;
    uint16_t   cs_pin;
} at32f435_spi_cfg_t;

static const at32f435_spi_cfg_t spi_cfg[BOARD_SPI_RESOURCE_NUM] =
{
    [BOARD_SPI_FONT_ROM] =
    {
        BOARD_SPI_FONT_ROM_INSTANCE,
        BOARD_SPI_FONT_ROM_CS_PORT,
        BOARD_SPI_FONT_ROM_CS_PIN
    }
};

_Static_assert((sizeof(spi_cfg) / sizeof(spi_cfg[0])) ==
                   BOARD_SPI_RESOURCE_NUM,
               "SPI resource table size mismatch");

static platform_err_t at32f435_spi_wait(spi_type *p_spi,
                                        uint32_t  flag,
                                        uint8_t   wait_for_reset,
                                        uint32_t  start_ms,
                                        uint32_t  timeout_ms)
{
    for(;;)
    {
        flag_status status = spi_i2s_flag_get(p_spi, flag);

        if(((0U == wait_for_reset) && (RESET != status)) ||
           ((0U != wait_for_reset) && (RESET == status)))
        {
            return PLATFORM_ERR_OK;
        }
        if(0U != (p_spi->sts & AT32F435_SPI_ERROR_FLAGS))
        {
            return PLATFORM_ERR_HW;
        }
        if((plat_tick_get_ms() - start_ms) >= timeout_ms)
        {
            return PLATFORM_ERR_TIMEOUT;
        }
    }
}

static void at32f435_spi_finish(const at32f435_spi_cfg_t *p_cfg)
{
    spi_type *p_spi = p_cfg->p_instance;

    gpio_bits_set(p_cfg->p_cs_port, p_cfg->cs_pin);
    spi_enable(p_spi, FALSE);

    if(RESET != spi_i2s_flag_get(p_spi, SPI_I2S_RDBF_FLAG))
    {
        (void)spi_i2s_data_receive(p_spi);
    }
    if(RESET != spi_i2s_flag_get(p_spi, SPI_MMERR_FLAG))
    {
        spi_i2s_flag_clear(p_spi, SPI_MMERR_FLAG);
    }
    if(RESET != spi_i2s_flag_get(p_spi, SPI_I2S_ROERR_FLAG))
    {
        spi_i2s_flag_clear(p_spi, SPI_I2S_ROERR_FLAG);
    }
    if(RESET != spi_i2s_flag_get(p_spi, SPI_CSPAS_FLAG))
    {
        spi_i2s_flag_clear(p_spi, SPI_CSPAS_FLAG);
    }
}

static platform_err_t at32f435_spi_byte_exchange(spi_type *p_spi,
                                                  uint8_t   tx_data,
                                                  uint8_t  *p_rx_data,
                                                  uint32_t  start_ms,
                                                  uint32_t  timeout_ms)
{
    platform_err_t ret;

    ret = at32f435_spi_wait(p_spi,
                            SPI_I2S_TDBE_FLAG,
                            0U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    spi_i2s_data_transmit(p_spi, tx_data);
    ret = at32f435_spi_wait(p_spi,
                            SPI_I2S_RDBF_FLAG,
                            0U,
                            start_ms,
                            timeout_ms);
    if(PLATFORM_ERR_OK != ret)
    {
        return ret;
    }

    if(NULL != p_rx_data)
    {
        *p_rx_data = (uint8_t)spi_i2s_data_receive(p_spi);
    }
    else
    {
        (void)spi_i2s_data_receive(p_spi);
    }
    return PLATFORM_ERR_OK;
}

platform_err_t plat_spi_write_read(plat_spi_id_t id,
                                   const uint8_t *p_command,
                                   uint16_t       command_size,
                                   uint8_t       *p_data,
                                   uint16_t       data_size,
                                   uint8_t        dummy_byte,
                                   uint32_t       timeout_ms)
{
    const at32f435_spi_cfg_t *p_cfg;
    spi_type *p_spi;
    platform_err_t ret;
    uint32_t start_ms;
    uint16_t index;

    if((id >= BOARD_SPI_RESOURCE_NUM) || (NULL == p_command) ||
       (0U == command_size) || (NULL == p_data) ||
       (0U == data_size) || (0U == timeout_ms))
    {
        return PLATFORM_ERR_PARAM;
    }

    p_cfg = &spi_cfg[id];
    p_spi = p_cfg->p_instance;
    at32f435_spi_finish(p_cfg);
    start_ms = plat_tick_get_ms();
    spi_enable(p_spi, TRUE);
    gpio_bits_reset(p_cfg->p_cs_port, p_cfg->cs_pin);

    for(index = 0U; index < command_size; index++)
    {
        ret = at32f435_spi_byte_exchange(p_spi,
                                         p_command[index],
                                         NULL,
                                         start_ms,
                                         timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_spi_finish(p_cfg);
            return ret;
        }
    }

    for(index = 0U; index < data_size; index++)
    {
        ret = at32f435_spi_byte_exchange(p_spi,
                                         dummy_byte,
                                         &p_data[index],
                                         start_ms,
                                         timeout_ms);
        if(PLATFORM_ERR_OK != ret)
        {
            at32f435_spi_finish(p_cfg);
            return ret;
        }
    }

    ret = at32f435_spi_wait(p_spi,
                            SPI_I2S_BF_FLAG,
                            1U,
                            start_ms,
                            timeout_ms);
    at32f435_spi_finish(p_cfg);
    return ret;
}
