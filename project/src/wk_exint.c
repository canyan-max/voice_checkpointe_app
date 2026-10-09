/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_exint.c
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
#include "wk_exint.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init exint function.
  * @param  none
  * @retval none
  */
void wk_exint_config(void)
{
  /* add user code begin exint_config 0 */

  /* add user code end exint_config 0 */

  gpio_init_type gpio_init_struct;
  exint_init_type exint_init_struct;

  /* add user code begin exint_config 1 */

  /* add user code end exint_config 1 */

  /* configure the EXINT7 */
  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = ALARM_IN_PD7_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(ALARM_IN_PD7_GPIO_PORT, &gpio_init_struct);

  scfg_exint_line_config(SCFG_PORT_SOURCE_GPIOD, SCFG_PINS_SOURCE7);

  exint_default_para_init(&exint_init_struct);
  exint_init_struct.line_enable = TRUE;
  exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
  exint_init_struct.line_select = EXINT_LINE_7;
  exint_init_struct.line_polarity = EXINT_TRIGGER_FALLING_EDGE;
  exint_init(&exint_init_struct);

  /* configure the EXINT10 */
  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pins = PIR_IN_PC10_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init(PIR_IN_PC10_GPIO_PORT, &gpio_init_struct);

  scfg_exint_line_config(SCFG_PORT_SOURCE_GPIOC, SCFG_PINS_SOURCE10);

  exint_default_para_init(&exint_init_struct);
  exint_init_struct.line_enable = TRUE;
  exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
  exint_init_struct.line_select = EXINT_LINE_10;
  exint_init_struct.line_polarity = EXINT_TRIGGER_FALLING_EDGE;
  exint_init(&exint_init_struct);

  /* add user code begin exint_config 2 */

  /* add user code end exint_config 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
