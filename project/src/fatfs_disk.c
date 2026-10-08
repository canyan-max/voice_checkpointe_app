/* add user code begin Header */
/**
  **************************************************************************
  * @file     fatfs_disk.c
  * @brief    fatfs disk application
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

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include <stddef.h>
#include <string.h>

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

#include "usb_core.h"
uint8_t usbh_msc_ret;
FATFS usbh_msc_fatfs;
uint8_t usbh_path[4];
extern disk_driver_type usbh_msc_disk;
uint8_t sd_ret;
FATFS sd_fatfs;
uint8_t sd_path[4];
extern disk_driver_type sd_disk;
fatfs_disk_type fatfs_disk = {0, {0}, {0}, {0}};;

/* add user code begin 0 */
#if FF_VOLUMES < 2
#error "Fixed SD/USB mapping requires FF_VOLUMES >= 2"
#endif

/* Startup registration only: SD is always drive 0, USB is always drive 1. */
static uint8_t fatfs_disk_add_fixed(disk_driver_type *driver,
                                  uint8_t *path, uint8_t lun)
{
  uint8_t slot;
  if(driver == NULL || path == NULL || driver->disk_initialize == NULL ||
     driver->disk_status == NULL || driver->disk_read == NULL ||
     driver->disk_ioctl == NULL)
    return 1;
#if FF_FS_READONLY == 0
  if(driver->disk_write == NULL)
    return 1;
#endif
  if(driver == &sd_disk)
    slot = 0;
  else if(driver == &usbh_msc_disk)
    slot = 1;
  else
    return 1;

  if(slot >= FF_VOLUMES)
    return 1;
  if(fatfs_disk.disk[slot] != NULL)
  {
    if(fatfs_disk.disk[slot] != driver || fatfs_disk.lun[slot] != lun)
      return 1;
  }
  else
  {
    fatfs_disk.disk[slot] = driver;
    fatfs_disk.lun[slot] = lun;
    fatfs_disk.is_initialized[slot] = 0;
    fatfs_disk.nbr++;
  }
  path[0] = (uint8_t)('0' + slot);
  path[1] = ':';
  path[2] = '/';
  path[3] = 0;
  return 0;
}

/* Unmount and stop all disk users before removing a driver. */
static uint8_t fatfs_disk_remove_fixed(uint8_t *path, uint8_t lun)
{
  uint8_t slot;
  if(path == NULL || (path[0] != '0' && path[0] != '1'))
    return 1;
  if(path[1] != ':' || (path[2] != 0 &&
     (path[2] != '/' || path[3] != 0)))
    return 1;
  slot = (uint8_t)(path[0] - '0');
  if(slot >= FF_VOLUMES || fatfs_disk.nbr == 0 ||
     fatfs_disk.disk[slot] == NULL || fatfs_disk.lun[slot] != lun)
    return 1;
  fatfs_disk.disk[slot] = NULL;
  fatfs_disk.lun[slot] = 0;
  fatfs_disk.is_initialized[slot] = 0;
  fatfs_disk.nbr--;
  return 0;
}

/* add user code end 0 */

/**
  * @brief  fatfs init
  * @param  none
  * @retval none
  */
void fatfs_disk_init(void)
{
/* add user code begin disk_init 0 */

/* add user code end disk_init 0 */

	fatfs_disk.nbr = 0;

/* add user code begin disk_init 1 */
  /* Startup only, before mounting any filesystem. */
  memset(&fatfs_disk, 0, sizeof(fatfs_disk));

/* add user code end disk_init 1 */

	/* fatfs add usb host msc disk driver */
	usbh_msc_ret = fatfs_disk_add(&usbh_msc_disk, usbh_path, 0);
	/* fatfs add sd disk driver */
	sd_ret = fatfs_disk_add(&sd_disk, sd_path, 0);
/* add user code begin disk_init 2 */

/* add user code end disk_init 2 */
}

/**
  * @brief  add a new disk driver
  * @param  driver: pointer to the disk driver
  * @param  path: pointer to the logical path
  * @param  lun: fatfs lun
  * @retval returns 0 is success, otherwise 1.
  */
uint8_t fatfs_disk_add(disk_driver_type *driver, uint8_t *path, uint8_t lun)
{
/* add user code begin disk_add 0 */
  return fatfs_disk_add_fixed(driver, path, lun);

/* add user code end disk_add 0 */

	uint8_t ret = 1;

/* add user code begin disk_add 1 */

/* add user code end disk_add 1 */

	if(fatfs_disk.nbr < FF_VOLUMES)
	{
		path[0] = fatfs_disk.nbr + '0';
		path[1] = ':';
		path[2] = '/';
		path[3] = 0;

		fatfs_disk.is_initialized[fatfs_disk.nbr] = 0;
		fatfs_disk.disk[fatfs_disk.nbr] = driver;
		fatfs_disk.lun[fatfs_disk.nbr] = lun;
		fatfs_disk.nbr ++;
		ret = 0;
	}

/* add user code begin disk_add 2 */

/* add user code end disk_add 2 */
	
	return ret;
}

/**
  * @brief  remove a disk driver
  * @param  driver: pointer to the disk driver
  * @param  lun: fatfs lun
  * @retval returns 0 is success, otherwise 1.
  */
uint8_t fatfs_disk_remove(uint8_t *path, uint8_t lun)
{
/* add user code begin disk_remove 0 */
  return fatfs_disk_remove_fixed(path, lun);

/* add user code end disk_remove 0 */

	uint8_t ret = 1;
	uint8_t nbr;

/* add user code begin disk_remove 1 */

/* add user code end disk_remove 1 */

	if(fatfs_disk.nbr < 1)
		return ret;
	
	nbr = path[0] - '0';
	if(fatfs_disk.disk[nbr] != 0)
	{
		fatfs_disk.disk[nbr] = 0;
		fatfs_disk.lun[nbr] = 0;
		fatfs_disk.nbr --;
		ret = 0;
	}

/* add user code begin disk_remove 2 */

/* add user code end disk_remove 2 */

	return ret;
}

/* add user code begin 1 */

/* add user code end 1 */
