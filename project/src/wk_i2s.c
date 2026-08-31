/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_i2s.c
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
#include "wk_i2s.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init i2s1 function.
  * @param  none
  * @retval none
  */
void wk_i2s1_init(void)
{
  /* add user code begin i2s1_init 0 */

  /* add user code end i2s1_init 0 */

  gpio_init_type gpio_init_struct;
  i2s_init_type i2s_init_struct;

  gpio_default_para_init(&gpio_init_struct);
  i2s_default_para_init(&i2s_init_struct);

  /* add user code begin i2s1_init 1 */

  /* add user code end i2s1_init 1 */

  /* configure the WS pin */
  gpio_pin_mux_config(IIS_LR_WS_PA4_GPIO_PORT, GPIO_PINS_SOURCE4, GPIO_MUX_5);
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = IIS_LR_WS_PA4_PIN;
  gpio_init(IIS_LR_WS_PA4_GPIO_PORT, &gpio_init_struct);

  /* configure the CK pin */
  gpio_pin_mux_config(IIS_CLK_PB3_GPIO_PORT, GPIO_PINS_SOURCE3, GPIO_MUX_5);
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = IIS_CLK_PB3_PIN;
  gpio_init(IIS_CLK_PB3_GPIO_PORT, &gpio_init_struct);

  /* configure the SD pin */
  gpio_pin_mux_config(IIS_SD_PB5_GPIO_PORT, GPIO_PINS_SOURCE5, GPIO_MUX_5);
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = IIS_SD_PB5_PIN;
  gpio_init(IIS_SD_PB5_GPIO_PORT, &gpio_init_struct);

  /* configure the MCK pin */
  gpio_pin_mux_config(IIS_MCK_PB6_GPIO_PORT, GPIO_PINS_SOURCE6, GPIO_MUX_5);
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = IIS_MCK_PB6_PIN;
  gpio_init(IIS_MCK_PB6_GPIO_PORT, &gpio_init_struct);

  /* configure the i2s_init_struct */
  i2s_init_struct.operation_mode = I2S_MODE_MASTER_TX;
  i2s_init_struct.audio_protocol = I2S_AUDIO_PROTOCOL_PHILLIPS;
  i2s_init_struct.data_channel_format = I2S_DATA_16BIT_CHANNEL_16BIT;
  i2s_init_struct.audio_sampling_freq = I2S_AUDIO_FREQUENCY_48K;
  i2s_init_struct.clock_polarity = I2S_CLOCK_POLARITY_LOW;
  i2s_init_struct.mclk_output_enable = TRUE;
  i2s_init(SPI1, &i2s_init_struct);

  /* add user code begin i2s1_init 2 */

  /* add user code end i2s1_init 2 */

  i2s_enable(SPI1, TRUE);

  /* add user code begin i2s1_init 3 */

  /* add user code end i2s1_init 3 */
}

/* add user code begin 1 */

/* add user code end 1 */
