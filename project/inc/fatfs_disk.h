/* add user code begin Header */
/**
  **************************************************************************
  * @file     fatfs_disk.h
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

/* define to prevent recursive inclusion -------------------------------------*/
#ifndef __FATFS_DISK_H
#define __FATFS_DISK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "diskio.h"
#include "stdint.h"
#include "at32f435_437.h"

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

/* add user code begin 0 */

/* add user code end 0 */

typedef struct
{
	DSTATUS (*disk_initialize) (BYTE lun);
	DSTATUS (*disk_status)     (BYTE lun);
	DRESULT (*disk_read)       (BYTE lun, BYTE *buff, LBA_t sector, UINT count);
	DRESULT (*disk_write)      (BYTE lun, const BYTE *buff, LBA_t sector, UINT count);
	DRESULT (*disk_ioctl)      (BYTE lun, BYTE cmd, void *buff);
}disk_driver_type;

typedef struct
{
	uint8_t nbr;
	uint8_t is_initialized[FF_VOLUMES];
	uint8_t lun[FF_VOLUMES];
	disk_driver_type *disk[FF_VOLUMES];
}fatfs_disk_type;

extern fatfs_disk_type fatfs_disk;

/* add user code begin 1 */

/* add user code end 1 */

void fatfs_disk_init(void);
uint8_t fatfs_disk_add(disk_driver_type *driver, uint8_t *path, uint8_t lun);
uint8_t fatfs_disk_remove(uint8_t *path, uint8_t lun);

/* add user code begin 2 */

/* add user code end 2 */
#ifdef __cplusplus
}
#endif

#endif
