/* add user code begin Header */
/**
  **************************************************************************
  * @file     sd_diskio.c
  * @brief    sd disk driver
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

#include "fatfs_disk.h"      /* Obtains integer types */
#include "at32_sdio.h"

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

#define BLOCK_SIZE        512

static DSTATUS sd_disk_initialize (BYTE lun);
static DSTATUS sd_disk_status (BYTE lun);
static DRESULT sd_disk_read (BYTE lun, BYTE *buff, LBA_t sector, UINT count);
static DRESULT sd_disk_write (BYTE lun, const BYTE *buff, LBA_t sector, UINT count);
static DRESULT sd_disk_ioctl (BYTE lun, BYTE cmd, void *buff);

disk_driver_type sd_disk = {
	sd_disk_initialize,
	sd_disk_status,
	sd_disk_read,
	sd_disk_write,
	sd_disk_ioctl
};

/* add user code begin 0 */

/* add user code end 0 */

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
static DSTATUS sd_disk_initialize (BYTE lun)
{
  DSTATUS stat = STA_NOINIT;

/* add user code begin sd_disk_initialize 0 */

/* add user code end sd_disk_initialize 0 */

  if(sd_init() == SD_OK)
	{
/* add user code begin sd_disk_initialize 1 */

/* add user code end sd_disk_initialize 1 */

		stat = RES_OK;
	}

/* add user code begin sd_disk_initialize 2 */

/* add user code end sd_disk_initialize 2 */

  return stat;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
static DSTATUS sd_disk_status (BYTE lun)
{
/* add user code begin sd_disk_status 0 */

/* add user code end sd_disk_status 0 */

  DSTATUS stat = 0;

/* add user code begin sd_disk_status 1 */

/* add user code end sd_disk_status 1 */

  return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
static DRESULT sd_disk_read (
  BYTE lun,
  BYTE *buff,    /* Data buffer to store read data */
  LBA_t sector,  /* Start sector in LBA */
  UINT count    /* Number of sectors to read */
)
{
  DRESULT res = RES_OK;

/* add user code begin sd_disk_read 0 */

/* add user code end sd_disk_read 0 */
	
	if(count > 1)
	{
		if(sd_mult_blocks_read(buff, (sector << 9), BLOCK_SIZE, count) != SD_OK)
		{
/* add user code begin sd_disk_read 1 */

/* add user code end sd_disk_read 1 */

			res = RES_ERROR;
		}
	}
	else
	{
	  if(sd_block_read(buff, (sector << 9), BLOCK_SIZE) != SD_OK)
		{
/* add user code begin sd_disk_read 2 */

/* add user code end sd_disk_read 2 */

			res = RES_ERROR;
		}
	}

/* add user code begin sd_disk_read 3 */

/* add user code end sd_disk_read 3 */

  return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
static DRESULT sd_disk_write (
  BYTE lun,      /* Physical drive nmuber to identify the drive */
  const BYTE *buff,  /* Data to be written */
  LBA_t sector,    /* Start sector in LBA */
  UINT count      /* Number of sectors to write */
)
{
  DRESULT res = RES_OK;

/* add user code begin sd_disk_write 0 */

/* add user code end sd_disk_write 0 */

	if(count > 1)
	{
		if(sd_mult_blocks_write(buff, (sector << 9), BLOCK_SIZE, count) != SD_OK)
		{
/* add user code begin sd_disk_write 1 */

/* add user code end sd_disk_write 1 */

			res = RES_ERROR;
		}
	}
	else
	{
	  if(sd_block_write(buff, (sector << 9), BLOCK_SIZE) != SD_OK)
		{
/* add user code begin sd_disk_write 2 */

/* add user code end sd_disk_write 2 */

			res = RES_ERROR;
		}
	}

/* add user code begin sd_disk_write 3 */

/* add user code end sd_disk_write 3 */

  return res;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
static DRESULT sd_disk_ioctl (
  BYTE lun,    /* Physical drive nmuber (0..) */
  BYTE cmd,    /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
/* add user code begin sd_disk_ioctl 0 */

/* add user code end sd_disk_ioctl 0 */

  DRESULT res = RES_OK;;
  switch(cmd)
	{
		case CTRL_SYNC:
		  break;
		case GET_SECTOR_SIZE:
			*(DWORD*)buff = BLOCK_SIZE;
		  break;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = sd_card_info.card_capacity/BLOCK_SIZE;
		  break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = sd_card_info.card_blk_size;
			break;
		default:
			res = RES_PARERR;
		  break;
	}

/* add user code begin sd_disk_ioctl 1 */

/* add user code end sd_disk_ioctl 1 */

  return res;
}

/* add user code begin 1 */

/* add user code end 1 */
