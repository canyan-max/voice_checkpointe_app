/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_gpio.c
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
#include "wk_gpio.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init gpio_input/gpio_output/gpio_analog/eventout function.
  * @param  none
  * @retval none
  */
void wk_gpio_config(void)
{
  /* add user code begin gpio_config 0 */

  /* add user code end gpio_config 0 */

  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin gpio_config 1 */

  /* add user code end gpio_config 1 */

  /* gpio input config */
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = KEY3_PE2_PIN | KEY4_PE3_PIN | TPA_FAULTZ_PE4_PIN | KEY1_PE0_PIN | KEY2_PE1_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(GPIOE, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = SDIO2_TFCD_PC13_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(SDIO2_TFCD_PC13_GPIO_PORT, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = PIR_IN_PC10_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(PIR_IN_PC10_GPIO_PORT, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = ALARM_IN_PD7_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(ALARM_IN_PD7_GPIO_PORT, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = TTS_R_B_PB8_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(TTS_R_B_PB8_GPIO_PORT, &gpio_init_struct);

  /* gpio output config */
  gpio_bits_reset(GPIOE, TPA_SDZ_PE5_PIN | E27_RELOAD_PE7_PIN | E27_RESET_PE8_PIN | DEBUG_SCOPE_PE9_PIN);
  gpio_bits_set(GPIOE, TPA_MUTE_PE6_PIN | LED1_REMOTE_PE12_PIN | LED2_UPDATA_PE13_PIN | LED3_BRCAT_PE14_PIN | LED4_RUN_PE15_PIN);
  gpio_bits_reset(GPIOA, PIR_POWER_PA5_PIN | E27_POWEREN_PA11_PIN | RS485_RXEN_PA12_PIN);
  gpio_bits_reset(GPIOD, HUB_R1_PD8_PIN | HUB_G1_PD9_PIN | HUB_B1_PD10_PIN | HUB_R2_PD11_PIN | HUB_G2_PD12_PIN | 
                  HUB_B2_PD13_PIN | HUB_SCK_PD15_PIN | HUB_A_PD0_PIN | HUB_B_PD1_PIN | HUB_C_PD2_PIN | 
                  HUB_D_PD3_PIN | HUB_E_PD4_PIN | HUB_LAT_PD5_PIN);
  gpio_bits_reset(GPIOC, LED_OUT_B_PC7_PIN | LED_OUT_R_PC8_PIN | LED_POWER_CS_PC9_PIN);
  gpio_bits_set(HUB_OE245_PD6_GPIO_PORT, HUB_OE245_PD6_PIN);
  gpio_bits_set(GPIOB, ALARM_OUT_PB7_PIN | TTS_RST_PB9_PIN);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = TPA_SDZ_PE5_PIN | TPA_MUTE_PE6_PIN | E27_RELOAD_PE7_PIN | E27_RESET_PE8_PIN | DEBUG_SCOPE_PE9_PIN | 
                               LED1_REMOTE_PE12_PIN | LED2_UPDATA_PE13_PIN | LED3_BRCAT_PE14_PIN | LED4_RUN_PE15_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOE, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = PIR_POWER_PA5_PIN | E27_POWEREN_PA11_PIN | RS485_RXEN_PA12_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = HUB_R1_PD8_PIN | HUB_G1_PD9_PIN | HUB_B1_PD10_PIN | HUB_R2_PD11_PIN | HUB_G2_PD12_PIN | 
                               HUB_B2_PD13_PIN | HUB_SCK_PD15_PIN | HUB_A_PD0_PIN | HUB_B_PD1_PIN | HUB_C_PD2_PIN | 
                               HUB_D_PD3_PIN | HUB_E_PD4_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = LED_OUT_B_PC7_PIN | LED_OUT_R_PC8_PIN | LED_POWER_CS_PC9_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOC, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = HUB_LAT_PD5_PIN | HUB_OE245_PD6_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = ALARM_OUT_PB7_PIN | TTS_RST_PB9_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOB, &gpio_init_struct);

  /* gpio analog config */
  gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
  gpio_init_struct.gpio_pins = GPIO_PINS_3;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOH, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
  gpio_init_struct.gpio_pins = GPIO_PINS_14;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);

  gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
  gpio_init_struct.gpio_pins = GPIO_PINS_6;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOC, &gpio_init_struct);

  /* add user code begin gpio_config 2 */

  /* add user code end gpio_config 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
