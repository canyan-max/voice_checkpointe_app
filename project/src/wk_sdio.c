/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_sdio.c
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
#include "wk_sdio.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init sdio2 function
  * @param  none
  * @retval none
  */
void wk_sdio2_init(void)
{
  /* add user code begin sdio2_init 0 */

  /* add user code end sdio2_init 0 */

  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin sdio2_init 1 */

  /* add user code end sdio2_init 1 */

  /* configure the D0 pin */
  gpio_pin_mux_config(SDDIO2_TFD0_PC0_GPIO_PORT, GPIO_PINS_SOURCE0, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFD0_PC0_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFD0_PC0_GPIO_PORT, &gpio_init_struct);

  /* configure the D1 pin */
  gpio_pin_mux_config(SDDIO2_TFD1_PC1_GPIO_PORT, GPIO_PINS_SOURCE1, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFD1_PC1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFD1_PC1_GPIO_PORT, &gpio_init_struct);

  /* configure the D2 pin */
  gpio_pin_mux_config(SDDIO2_TFD2_PC2_GPIO_PORT, GPIO_PINS_SOURCE2, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFD2_PC2_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFD2_PC2_GPIO_PORT, &gpio_init_struct);

  /* configure the D3 pin */
  gpio_pin_mux_config(SDDIO2_TFD3_PC3_GPIO_PORT, GPIO_PINS_SOURCE3, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFD3_PC3_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFD3_PC3_GPIO_PORT, &gpio_init_struct);

  /* configure the CK pin */
  gpio_pin_mux_config(SDDIO2_TFCLX_PA2_GPIO_PORT, GPIO_PINS_SOURCE2, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFCLX_PA2_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFCLX_PA2_GPIO_PORT, &gpio_init_struct);

  /* configure the CMD pin */
  gpio_pin_mux_config(SDDIO2_TFCMD_PA3_GPIO_PORT, GPIO_PINS_SOURCE3, GPIO_MUX_10);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = SDDIO2_TFCMD_PA3_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDDIO2_TFCMD_PA3_GPIO_PORT, &gpio_init_struct);

  /* configure param */
  sdio_clock_config(SDIO2, 10, SDIO_CLOCK_EDGE_RISING);
  sdio_power_saving_mode_enable(SDIO2, FALSE);
  sdio_flow_control_enable(SDIO2, FALSE);
  sdio_clock_bypass(SDIO2, FALSE);
  sdio_bus_width_config(SDIO2, SDIO_BUS_WIDTH_D4);

  /* sdio power on */
  sdio_power_set(SDIO2, SDIO_POWER_ON);
  /* enable to output sdio_ck */
  sdio_clock_enable(SDIO2, TRUE);

  /* add user code begin sdio2_init 2 */

  /* add user code end sdio2_init 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
