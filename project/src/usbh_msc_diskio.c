/* add user code begin Header */
/**
  **************************************************************************
  * @file     usbh_msc_diskio.c
  * @brief    usb host disk driver
  **************************************************************************
  *
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

#include "fatfs_disk.h"
#include "usb_core.h"
#include "usbh_msc_class.h"

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

static DSTATUS usbh_msc_disk_initialize (BYTE lun);
static DSTATUS usbh_msc_disk_status (BYTE lun);
static DRESULT usbh_msc_disk_read (BYTE lun, BYTE *buff, LBA_t sector, UINT count);
static DRESULT usbh_msc_disk_write (BYTE lun, const BYTE *buff, LBA_t sector, UINT count);
static DRESULT usbh_msc_disk_ioctl (BYTE lun, BYTE cmd, void *buff);

extern otg_core_type otg_core_struct_fs2;
otg_core_type *otg_core_struct = &otg_core_struct_fs2;

disk_driver_type usbh_msc_disk = {
	usbh_msc_disk_initialize,
	usbh_msc_disk_status,
	usbh_msc_disk_read,
	usbh_msc_disk_write,
	usbh_msc_disk_ioctl
};

/* add user code begin 0 */

/* add user code end 0 */

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
static DSTATUS usbh_msc_disk_initialize (BYTE lun)
{
/* add user code begin msc_disk_initialize 0 */

/* add user code end msc_disk_initialize 0 */

  return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
static DSTATUS usbh_msc_disk_status (BYTE lun)
{
/* add user code begin msc_disk_status 0 */

/* add user code end msc_disk_status 0 */

  if(usbh_msc_is_ready(&otg_core_struct->host, lun) == MSC_OK)
  {
/* add user code begin msc_disk_status 1 */

/* add user code end msc_disk_status 1 */

    return RES_OK;
  }

/* add user code begin msc_disk_status 2 */

/* add user code end msc_disk_status 2 */

  return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
static DRESULT usbh_msc_disk_read (
  BYTE lun,
  BYTE *buff,    /* Data buffer to store read data */
  LBA_t sector,  /* Start sector in LBA */
  UINT count    /* Number of sectors to read */
)
{
/* add user code begin msc_disk_read 0 */

/* add user code end msc_disk_read 0 */

	if(usbh_msc_read(&otg_core_struct->host, sector, count, buff, lun) == USB_OK)
	{
/* add user code begin msc_disk_read 1 */

/* add user code end msc_disk_read 1 */

		return RES_OK;
	}

/* add user code begin msc_disk_read 2 */

/* add user code end msc_disk_read 2 */

  return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
static DRESULT usbh_msc_disk_write (
  BYTE lun,
  const BYTE *buff,  /* Data to be written */
  LBA_t sector,    /* Start sector in LBA */
  UINT count      /* Number of sectors to write */
)
{
/* add user code begin msc_disk_write 0 */

/* add user code end msc_disk_write 0 */

  if(usbh_msc_write(&otg_core_struct->host, sector, count, (uint8_t *)buff, lun) == USB_OK)
	{
/* add user code begin msc_disk_write 1 */

/* add user code end msc_disk_write 1 */

		return RES_OK;
	}

/* add user code begin msc_disk_write 2 */

/* add user code end msc_disk_write 2 */

  return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
static DRESULT usbh_msc_disk_ioctl (
  BYTE lun,
  BYTE cmd,    /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
/* add user code begin msc_disk_ioctl 0 */

/* add user code end msc_disk_ioctl 0 */

  usbh_core_type *host = (usbh_core_type *)&otg_core_struct->host;
  usbh_msc_type *pmsc = (usbh_msc_type *)host->class_handler->pdata;
  DRESULT res = RES_OK;
  switch(cmd)
  {
    case CTRL_SYNC:
      res = RES_OK;
      break;
    case GET_SECTOR_COUNT:
      *(DWORD*)buff = pmsc->l_unit_n[lun].capacity.blk_nbr;
      break;
    case GET_SECTOR_SIZE:
      *(DWORD*)buff = pmsc->l_unit_n[lun].capacity.blk_size;
      break;
    case GET_BLOCK_SIZE:
      *(DWORD*)buff = pmsc->l_unit_n[lun].capacity.blk_size;
      break;
    default:
      res = RES_PARERR;
  }

/* add user code begin msc_disk_ioctl 1 */

/* add user code end msc_disk_ioctl 1 */

  return res;
}

/* add user code begin 1 */

/* add user code end 1 */
