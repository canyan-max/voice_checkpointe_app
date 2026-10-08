/* add user code begin Header */
/**
  **************************************************************************
  * @file     usbh_user.c
  * @brief    usb user function
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

#include "usbh_user.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */

/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */

/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */

/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

/* add user code end 0 */

static usb_sts_type usbh_user_init(void);
static usb_sts_type usbh_user_reset(void);
static usb_sts_type usbh_user_attached(void);
static usb_sts_type usbh_user_disconnect(void);
static usb_sts_type usbh_user_speed(uint8_t speed);
static usb_sts_type usbh_user_mfc_string(void *string);
static usb_sts_type usbh_user_product_string(void *string);
static usb_sts_type usbh_user_serial_string(void *string);
static usb_sts_type usbh_user_enumeration_done(void);
static usb_sts_type usbh_user_application(void);
static usb_sts_type usbh_user_active_vbus(void *uhost, confirm_state state);
static usb_sts_type usbh_user_not_support(void);

usbh_user_handler_type usbh_user_handle =
{
  usbh_user_init,
  usbh_user_reset,
  usbh_user_attached,
  usbh_user_disconnect,
  usbh_user_speed,
  usbh_user_mfc_string,
  usbh_user_product_string,
  usbh_user_serial_string,
  usbh_user_enumeration_done,
  usbh_user_application,
  usbh_user_active_vbus,
  usbh_user_not_support,
};

/**
  * @brief  usb host init user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_init(void)
{
  /* add user code begin usbh_user_init 0 */

  /* add user code end usbh_user_init 0 */

  usb_sts_type status = USB_OK;

  /* add user code begin usbh_user_init 1 */

  /* add user code end usbh_user_init 1 */

  return status;
}

/**
  * @brief  usb host reset user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_reset(void)
{
  /* add user code begin usbh_user_reset 0 */

  /* add user code end usbh_user_reset 0 */

  usb_sts_type status = USB_OK;

  /* add user code begin usbh_user_reset 1 */

  /* add user code end usbh_user_reset 1 */

  return status;
}

/**
  * @brief  usb host check device attached
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_attached(void)
{
  /* add user code begin usbh_user_attached 0 */

  /* add user code end usbh_user_attached 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("USB Device Attached");

  /* add user code begin usbh_user_attached 1 */

  /* add user code end usbh_user_attached 1 */

  return status;
}

/**
  * @brief  usb host discconet user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_disconnect(void)
{
  /* add user code begin usbh_user_disconnect 0 */

  /* add user code end usbh_user_disconnect 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("Device Disconnect");

  /* add user code begin usbh_user_disconnect 1 */

  /* add user code end usbh_user_disconnect 1 */

  return status;
}

/**
  * @brief  usb host speed user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_speed(uint8_t speed)
{
  /* add user code begin usbh_user_speed 0 */

  /* add user code end usbh_user_speed 0 */

  usb_sts_type status = USB_OK;
  if(speed == USB_PRTSPD_FULL_SPEED)
  {
    USBH_DEBUG("This is a Full-Speed device");
  }
  else if(speed == USB_PRTSPD_LOW_SPEED)
  {
    USBH_DEBUG("This is a Low-Speed device");
  }
  else if(speed == USB_PRTSPD_HIGH_SPEED)
  {
    USBH_DEBUG("This is a High-Speed device");
  }

  /* add user code begin usbh_user_speed 0 */

  /* add user code end usbh_user_speed 0 */

  return status;
}

/**
  * @brief  usb host manufacturer string user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_mfc_string(void *string)
{
  /* add user code begin usbh_user_mfc_string 0 */

  /* add user code end usbh_user_mfc_string 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("Manufacturer: %s", (uint8_t *)string);

  /* add user code begin usbh_user_mfc_string 1 */

  /* add user code end usbh_user_mfc_string 1 */

  return status;
}

/**
  * @brief  usb host product string user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_product_string(void *string)
{
  /* add user code begin usbh_user_product_string 0 */

  /* add user code end usbh_user_product_string 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("Product: %s", (uint8_t *)string);

  /* add user code begin usbh_user_product_string 1 */

  /* add user code end usbh_user_product_string 1 */

  return status;
}

/**
  * @brief  usb host serial string user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_serial_string(void *string)
{
  /* add user code begin usbh_user_serial_string 0 */

  /* add user code end usbh_user_serial_string 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("Serial: %s", (uint8_t *)string);

  /* add user code begin usbh_user_serial_string 1 */

  /* add user code end usbh_user_serial_string 1 */

  return status;
}

/**
  * @brief  usb host enumeration done user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_enumeration_done(void)
{
  /* add user code begin usbh_user_enumeration_done 0 */

  /* add user code end usbh_user_enumeration_done 0 */

  usb_sts_type status = USB_OK;
  USBH_DEBUG("Enumeration done");

  /* add user code begin usbh_user_enumeration_done 1 */

  /* add user code end usbh_user_enumeration_done 1 */

  return status;
}

/**
  * @brief  usb host application user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_application(void)
{
  /* add user code begin usbh_user_application 0 */

  /* add user code end usbh_user_application 0 */

  usb_sts_type status = USB_OK;

  /* add user code begin usbh_user_application 1 */

  /* add user code end usbh_user_application 1 */

  return status;
}


/**
  * @brief  usb host active vbus user handler
  * @param  uhost: to the structure of usbh_core_type
  * @param  state: vbus state
            TRUE: active vbus
            FALSE: deactive vbus
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_active_vbus(void *uhost, confirm_state state)
{
  /* add user code begin usbh_user_active_vbus 0 */

  /* add user code end usbh_user_active_vbus 0 */

  usb_sts_type status = USB_OK;

  /* add user code begin usbh_user_active_vbus 1 */

  /* add user code end usbh_user_active_vbus 1 */

  return status;
}

/**
  * @brief  usb host not support user handler
  * @param  none
  * @retval usb_sts_type
  */
static usb_sts_type usbh_user_not_support(void)
{
  /* add user code begin usbh_user_not_support 0 */

  /* add user code end usbh_user_not_support 0 */

  usb_sts_type status = USB_OK;

  /* add user code begin usbh_user_not_support 1 */

  /* add user code end usbh_user_not_support 1 */

  return status;
}

/* add user code begin 1 */

/* add user code end 1 */

