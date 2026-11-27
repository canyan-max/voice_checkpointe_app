/**
 **************************************************************************************************
 *@file               :   usb_upgrade_port.c
 *@brief              :   Connect USB Host and FatFs to the firmware upgrade App.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!
 **************************************************************************************************
 */

/* includes -------------------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "fatfs_disk.h"
#include "usb_core.h"
#include "usbh_msc_class.h"
#include "usb_upgrade_port.h"
#include "firmware_upgrade_app.h"
#include "plat_log.h"
#include "plat_sys.h"
/* define   -------------------------------------------------------------------------------------*/

#define USB_UPGRADE_DRIVE_INDEX (1U)
#define USB_UPGRADE_SECTOR_BYTES (512U)
#define USB_UPGRADE_DRIVE _T("1:")
#define USB_UPGRADE_IMAGE_PATH _T("1:/UPDATE.BIN")

/* typedef --------------------------------------------------------------------------------------*/

/* variables ------------------------------------------------------------------------------------*/

extern otg_core_type    otg_core_struct_fs2;
extern disk_driver_type usbh_msc_disk;

/* USB drive and its single file are owned by the USB Host polling task. */
static FATFS    s_fs;
static FIL      s_file;
static uint8_t  s_file_open;
static uint8_t  s_mounted;
static uint8_t  s_attached;
static uint8_t  s_ready_reported;
static uint8_t  s_active;
static uint32_t s_size;
static uint32_t s_crc32;
static uint32_t s_started_ms;

/* private  functions  --------------------------------------------------------------------------*/

static uint8_t usb_upgrade_port_has_device(const usbh_core_type *p_host) {
    return 0U != p_host->conn_sts && USBH_CLASS == p_host->global_state &&
           NULL != p_host->class_handler && NULL != p_host->class_handler->pdata;
}

static uint8_t usb_upgrade_port_disk_ready(usbh_core_type *p_host) {
    return usb_upgrade_port_has_device(p_host) && USBH_MSC_IDLE == usbh_msc.state &&
           USBH_MSC_IDLE == usbh_msc.l_unit_n[0U].state && MSC_OK == usbh_msc_is_ready(p_host, 0U);
}

static void usb_upgrade_port_finish(const char *p_error, unsigned int detail) {
    FRESULT close_ret   = FR_OK;
    FRESULT unmount_ret = FR_OK;
    if (0U != s_file_open) {
        /* The mount is discarded below if the device is gone. */
        if (usb_upgrade_port_has_device(&otg_core_struct_fs2.host)) {
            close_ret = f_close(&s_file);
        } else {
            close_ret = FR_NOT_READY;
        }
        s_file_open = 0U;
    }
    if (0U != s_mounted) {
        unmount_ret = f_mount(NULL, USB_UPGRADE_DRIVE, 0U);
        s_mounted   = 0U;
    }
    fatfs_disk.is_initialized[USB_UPGRADE_DRIVE_INDEX] = 0U;
    s_active                                           = 0U;

    if (NULL != p_error) {
        plat_log_w("USB_OTA failed: %s ret=%u", p_error, detail);
    } else if (FR_OK != close_ret || FR_OK != unmount_ret) {
        plat_log_w("USB_OTA cleanup failed: close=%u unmount=%u", (unsigned int)close_ret,
                   (unsigned int)unmount_ret);
    } else {
        plat_log_i("USB_OTA verified: bytes=%lu crc=%08lX took=%lu ms stack_free=%lu words",
                   (unsigned long)s_size, (unsigned long)s_crc32,
                   (unsigned long)(plat_tick_get_ms() - s_started_ms),
                   (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    }
}

static platform_err_t usb_upgrade_port_read(void *p_context, uint8_t *p_data, uint32_t size) {
    (void)p_context;
    UINT          received = 0U;
    const FRESULT ret      = f_read(&s_file, p_data, size, &received);
    if (FR_OK != ret || received != size) {
        plat_log_w("USB_OTA UPDATE.BIN read failed: ret=%u got=%u expected=%lu", (unsigned int)ret,
                   received, (unsigned long)size);
        return PLATFORM_ERR_HW;
    }
    return PLATFORM_ERR_OK;
}

/* exported functions ---------------------------------------------------------------------------*/

void usb_upgrade_port_request(void) {
    static const firmware_upgrade_io_t s_io = {
        .pf_read   = usb_upgrade_port_read,
        .p_context = NULL,
    };

    if (0U != s_active) {
        plat_log_w("USB_OTA staging in progress");
        return;
    }
    if (!usb_upgrade_port_disk_ready(&otg_core_struct_fs2.host)) {
        plat_log_w("USB_OTA drive not ready");
        return;
    }
    if (&usbh_msc_disk != fatfs_disk.disk[USB_UPGRADE_DRIVE_INDEX] ||
        0U != fatfs_disk.lun[USB_UPGRADE_DRIVE_INDEX] ||
        USB_UPGRADE_SECTOR_BYTES != usbh_msc.l_unit_n[0U].capacity.blk_size) {
        plat_log_w("USB_OTA requires USB LUN0 on drive 1 with 512-byte sectors");
        return;
    }

    s_started_ms                                       = plat_tick_get_ms();
    fatfs_disk.is_initialized[USB_UPGRADE_DRIVE_INDEX] = 0U;
    /* FatFs registers the object even if immediate mounting fails. */
    s_mounted   = 1U;
    FRESULT ret = f_mount(&s_fs, USB_UPGRADE_DRIVE, 1U);
    if (FR_OK != ret) {
        usb_upgrade_port_finish("mount", (unsigned int)ret);
        return;
    }
    ret = f_open(&s_file, USB_UPGRADE_IMAGE_PATH, FA_READ);
    if (FR_OK != ret) {
        usb_upgrade_port_finish("open UPDATE.BIN", (unsigned int)ret);
        return;
    }
    s_file_open             = 1U;
    const FSIZE_t  size     = f_size(&s_file);
    const uint32_t capacity = firmware_upgrade_app_capacity();
    if (0U == size || size > capacity) {
        plat_log_w("USB_OTA invalid size: bytes=%lu max=%lu", (unsigned long)size,
                   (unsigned long)capacity);
        usb_upgrade_port_finish("size", 0U);
        return;
    }

    const platform_err_t start_ret = firmware_upgrade_app_start((uint32_t)size, &s_io);
    if (PLATFORM_ERR_OK != start_ret) {
        usb_upgrade_port_finish("start", (unsigned int)start_ret);
        return;
    }
    s_size   = (uint32_t)size;
    s_active = 1U;
    plat_log_i("USB_OTA staging UPDATE.BIN: bytes=%lu", (unsigned long)s_size);
}

void usb_upgrade_port_poll(void) {
    usbh_core_type *p_host = &otg_core_struct_fs2.host;
    if (0U == p_host->conn_sts) {
        s_attached = 0U;
    } else if (0U == s_attached) {
        s_attached = 1U;
        plat_log_i("USB_OTA attached, waiting for MSC ready");
    }
    if (!usb_upgrade_port_has_device(p_host)) {
        s_ready_reported = 0U;
        if (0U != s_active) {
            firmware_upgrade_app_cancel();
            usb_upgrade_port_finish("USB disconnected or class lost", 0U);
        }
        return;
    }
    if (0U != s_active) {
        const firmware_upgrade_progress_t progress = firmware_upgrade_app_poll(&s_crc32);
        if (FIRMWARE_UPGRADE_PROGRESS_DONE == progress) {
            usb_upgrade_port_finish(NULL, 0U);
        } else if (FIRMWARE_UPGRADE_PROGRESS_WORKING != progress) {
            usb_upgrade_port_finish("staging or verification", (unsigned int)progress);
        }
    } else if (0U == s_ready_reported && usb_upgrade_port_disk_ready(p_host)) {
        s_ready_reported = 1U;
        plat_log_i("USB_OTA drive ready: press SW2 to stage UPDATE.BIN");
    }
}

uint8_t usb_upgrade_port_is_active(void) {
    return s_active;
}

/* end of file ----------------------------------------------------------------------------------*/
