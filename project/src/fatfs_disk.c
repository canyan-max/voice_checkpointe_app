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

uint8_t sd_ret;
FATFS sd_fatfs;
uint8_t sd_path[4];
extern disk_driver_type sd_disk;
fatfs_disk_type fatfs_disk = {0, {0}, {0}, {0}};;

/* add user code begin 0 */

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

/* add user code end disk_init 1 */

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
