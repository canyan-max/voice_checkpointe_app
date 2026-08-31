/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_usb_otgfs.c
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
#include "wk_usb_otgfs.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init usb_otgfs2 function
  * @param  none
  * @retval none
  */
void wk_usb_otgfs2_init(void)
{
  /* add user code begin usb_otgfs2_init 0 */

  /* add user code end usb_otgfs2_init 0 */
  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin usb_otgfs2_init 1 */

  /* add user code end usb_otgfs2_init 1 */

  /* configure the VBUS pin */
  gpio_pin_mux_config(USB_VBUS_PB13_GPIO_PORT, GPIO_PINS_SOURCE13, GPIO_MUX_12);
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = USB_VBUS_PB13_PIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(USB_VBUS_PB13_GPIO_PORT, &gpio_init_struct);

  crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_HICK);

  /* add user code begin usb_otgfs2_init 2 */

  /* add user code end usb_otgfs2_init 2 */
}

/* add user code begin 1 */

/* add user code end 1 */
