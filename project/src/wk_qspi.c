/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_qspi.c
  * @brief    work bench config program
  **************************************************************************
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */
/* add user code end Header */

/* Includes ------------------------------------------------------------------*/
#include "wk_qspi.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init qspi1 function
  * @param  none
  * @retval none
  */
void wk_qspi1_init(void)
{
  /* add user code begin qspi1_init 0 */

  /* add user code end qspi1_init 0 */

  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin qspi1_init 1 */

  /* add user code end qspi1_init 1 */

  /* configure the SCK pin */
  gpio_pin_mux_config(QSPI1_SCK_PB1_GPIO_PORT, GPIO_PINS_SOURCE1, GPIO_MUX_9);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_SCK_PB1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_SCK_PB1_GPIO_PORT, &gpio_init_struct);

  /* configure the CS pin */
  gpio_pin_mux_config(QSPI1_WS_PC11_GPIO_PORT, GPIO_PINS_SOURCE11, GPIO_MUX_9);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_WS_PC11_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_WS_PC11_GPIO_PORT, &gpio_init_struct);

  /* configure the IO0 pin */
  gpio_pin_mux_config(QSPI1_SI0_PA6_GPIO_PORT, GPIO_PINS_SOURCE6, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_SI0_PA6_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_SI0_PA6_GPIO_PORT, &gpio_init_struct);

  /* configure the IO1 pin */
  gpio_pin_mux_config(QSPI1_SI1_PA7_GPIO_PORT, GPIO_PINS_SOURCE7, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_SI1_PA7_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_SI1_PA7_GPIO_PORT, &gpio_init_struct);

  /* configure the IO2 pin */
  gpio_pin_mux_config(QSPI1_SI2_PC4_GPIO_PORT, GPIO_PINS_SOURCE4, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_SI2_PC4_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_SI2_PC4_GPIO_PORT, &gpio_init_struct);

  /* configure the IO3 pin */
  gpio_pin_mux_config(QSPI1_SI3_PC5_GPIO_PORT, GPIO_PINS_SOURCE5, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = QSPI1_SI3_PC5_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(QSPI1_SI3_PC5_GPIO_PORT, &gpio_init_struct);

 /* configure param */
  qspi_xip_enable(QSPI1, FALSE);

  qspi_clk_division_set(QSPI1, QSPI_CLK_DIV_3);
  
  qspi_sck_mode_set(QSPI1, QSPI_SCK_MODE_0);

  qspi_busy_config(QSPI1, QSPI_BUSY_OFFSET_0);

  qspi_auto_ispc_enable(QSPI1);

  qspi_dma_rx_threshold_set(QSPI1, QSPI_DMA_FIFO_THOD_WORD08);

  qspi_dma_tx_threshold_set(QSPI1, QSPI_DMA_FIFO_THOD_WORD08);

  /* add user code begin qspi1_init 2 */

  /* add user code end qspi1_init 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
