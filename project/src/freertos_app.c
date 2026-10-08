/* add user code begin Header */
/**
  ******************************************************************************
  * File Name          : freertos_app.c
  * Description        : Code for freertos applications
  */
/* add user code end Header */

/* Includes ------------------------------------------------------------------*/
#include "freertos_app.h"
#include "usb_app.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "plat_log.h"
#include "plat_gpio.h"
#include "plat_sys.h"
#include "board_resources.h"
#include "audio_playback_app.h"
#include "audio_volume_service.h"
#include "button.h"
#include "voice_presentation_app.h"
#include "remote_modem_app.h"
#include "fatfs_disk.h"
#include <string.h>
#include "usb_core.h"
#include "usbh_msc_class.h"
/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */
#define START_TEST_TASK_STACK_WORDS      1536U
#define START_TEST_KEY_POLL_MS             10U
#define START_TEST_KEY_DEBOUNCE_MS         30U
#define START_TEST_KEY_LONG_PRESS_MS     1000U
#define START_TEST_KEY_DOUBLE_CLICK_MS    300U
#define START_TEST_BUTTON_COUNT             4U
#define START_TEST_VOLUME_STEP              5U
#define START_TEST_I2C_TIMEOUT_MS          100U

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

/* Temporary board test. USB host polling and file I/O share this task. */
extern otg_core_type otg_core_struct_fs2;
extern disk_driver_type usbh_msc_disk;
static FATFS usb_test_fs;
static DIR usb_test_dir;
static FIL usb_test_file;
static FILINFO usb_test_info;
static TCHAR usb_test_path[FF_MAX_LFN + 4U];
static uint8_t usb_test_data[128];
static char usb_test_name[96];

/* UTF-16 names to bounded UTF-8 for the UART log. */
static void usb_test_name_log(const TCHAR *name)
{
  unsigned int in = 0U, out = 0U;
  while(name[in] != 0 && out + 4U < sizeof(usb_test_name))
  {
    uint32_t c = (uint32_t)name[in++];
    if(c >= 0xD800U && c <= 0xDFFFU) c = '?';
    if(c < 0x80U)
      usb_test_name[out++] = (char)((c < 0x20U || c == 0x7FU) ? '?' : c);
    else if(c < 0x800U)
    {
      usb_test_name[out++] = (char)(0xC0U | (c >> 6));
      usb_test_name[out++] = (char)(0x80U | (c & 0x3FU));
    }
    else
    {
      usb_test_name[out++] = (char)(0xE0U | (c >> 12));
      usb_test_name[out++] = (char)(0x80U | ((c >> 6) & 0x3FU));
      usb_test_name[out++] = (char)(0x80U | (c & 0x3FU));
    }
  }
  if(name[in] != 0) usb_test_name[out++] = '~';
  usb_test_name[out] = 0;
}


/* Create a new file; never truncate an existing USB file. */
static uint8_t usb_test_write_verify(void)
{
  static const char content[] =
      "Voice checkpoint USB write test.\r\n"
      "SD=0, USB=1. Write and read-back verification.\r\n";
  static TCHAR path[] = _T("1:/usb_test_00.txt");
  FRESULT result, sync_result, close_result;
  UINT written = 0U, received = 0U;
  unsigned int index;
  uint8_t match;
  for(index = 0U; index < 100U; index++)
  {
    path[12] = (TCHAR)('0' + index / 10U);
    path[13] = (TCHAR)('0' + index % 10U);
    result = f_open(&usb_test_file, path, FA_WRITE | FA_CREATE_NEW);
    if(result != FR_EXIST) break;
  }
  if(index == 100U)
  {
    plat_log_w("USB_TEST write skipped: usb_test_00..99.txt already exist");
    return 0U;
  }
  usb_test_name_log(path);
  plat_log_i("USB_TEST create %s ret=%u", usb_test_name, (unsigned int)result);
  if(result != FR_OK) return 0U;
  result = f_write(&usb_test_file, content, sizeof(content) - 1U, &written);
  sync_result = (result == FR_OK && written == sizeof(content) - 1U) ?
      f_sync(&usb_test_file) : FR_DISK_ERR;
  close_result = f_close(&usb_test_file);
  plat_log_i("USB_TEST write ret=%u bytes=%u expected=%u sync=%u close=%u",
      (unsigned int)result, (unsigned int)written,
      (unsigned int)(sizeof(content) - 1U),
      (unsigned int)sync_result, (unsigned int)close_result);
  if(result != FR_OK || written != sizeof(content) - 1U ||
     sync_result != FR_OK || close_result != FR_OK)
    return 0U;
  result = f_open(&usb_test_file, path, FA_READ);
  plat_log_i("USB_TEST reopen ret=%u", (unsigned int)result);
  if(result != FR_OK) return 0U;
  match = (uint8_t)(f_size(&usb_test_file) == sizeof(content) - 1U);
  result = f_read(&usb_test_file, usb_test_data, sizeof(usb_test_data), &received);
  close_result = f_close(&usb_test_file);
  match = (uint8_t)(match && result == FR_OK && close_result == FR_OK &&
      received == sizeof(content) - 1U &&
      memcmp(usb_test_data, content, sizeof(content) - 1U) == 0);
  plat_log_i("USB_TEST verify ret=%u bytes=%u close=%u match=%u",
      (unsigned int)result, (unsigned int)received,
      (unsigned int)close_result, (unsigned int)match);
  if(match)
  {
    usb_test_data[received] = 0U;
    plat_log_i("USB_TEST TXT content: %s", (char *)usb_test_data);
  }
  return match;
}

static void usb_test_run(void)
{
  FRESULT result, close_result;
  UINT bytes = 0U;
  uint32_t started = plat_tick_get_ms();
  unsigned int entries = 0U, i, j;
  uint8_t have_file = 0U, dir_open = 0U, read_ok = 0U;
  uint8_t list_ok = 0U, write_ok = 0U;
  char hex[49], ascii[17];
  static const char digits[] = "0123456789ABCDEF";
  usb_test_path[0] = 0;
  fatfs_disk.is_initialized[1] = 0U;
  plat_log_i("USB_TEST ready: drive=1:/ blocks=%lu sector=%lu",
      (unsigned long)usbh_msc.l_unit_n[0].capacity.blk_nbr,
      (unsigned long)usbh_msc.l_unit_n[0].capacity.blk_size);
  if(usbh_msc.l_unit_n[0].capacity.blk_size != 512U)
  {
    plat_log_e("USB_TEST unsupported sector size (requires 512)");
    return;
  }
  result = f_mount(&usb_test_fs, _T("1:"), 1U);
  plat_log_i("USB_TEST mount ret=%u", (unsigned int)result);
  if(result != FR_OK) goto cleanup;
  result = f_opendir(&usb_test_dir, _T("1:/"));
  if(result != FR_OK)
  {
    plat_log_e("USB_TEST opendir ret=%u", (unsigned int)result);
    goto cleanup;
  }
  dir_open = 1U;
  while(entries < 32U)
  {
    if(otg_core_struct_fs2.host.conn_sts == 0U ||
       (uint32_t)(plat_tick_get_ms() - started) >= 5000U)
    {
      plat_log_w("USB_TEST listing stopped: disconnect or 5s limit");
      goto cleanup;
    }
    result = f_readdir(&usb_test_dir, &usb_test_info);
    if(result != FR_OK)
    {
      plat_log_e("USB_TEST readdir ret=%u", (unsigned int)result);
      goto cleanup;
    }
    if(usb_test_info.fname[0] == 0) break;
    entries++;
    usb_test_name_log(usb_test_info.fname);
    plat_log_i("USB_TEST %s %s size=%lu",
        (usb_test_info.fattrib & AM_DIR) ? "DIR" : "FILE",
        usb_test_name, (unsigned long)usb_test_info.fsize);
    if(!have_file && !(usb_test_info.fattrib & AM_DIR))
    {
      usb_test_path[0] = '1'; usb_test_path[1] = ':';
      usb_test_path[2] = '/';
      for(i = 0U; i < FF_MAX_LFN && usb_test_info.fname[i] != 0; i++)
        usb_test_path[i + 3U] = usb_test_info.fname[i];
      usb_test_path[i + 3U] = 0;
      have_file = 1U;
    }
    vTaskDelay(pdMS_TO_TICKS(1U));
  }
  list_ok = 1U;
  plat_log_i("USB_TEST root entries=%u%s", entries,
      entries == 32U ? " (limit reached)" : "");
  close_result = f_closedir(&usb_test_dir);
  dir_open = 0U;
  if(close_result != FR_OK)
  {
    plat_log_e("USB_TEST closedir ret=%u", (unsigned int)close_result);
    goto cleanup;
  }
  if(!have_file)
  {
    plat_log_w("USB_TEST no root file to preview");
    goto write_test;
  }
  if(otg_core_struct_fs2.host.conn_sts == 0U) goto cleanup;
  usb_test_name_log(usb_test_path);
  result = f_open(&usb_test_file, usb_test_path, FA_READ);
  plat_log_i("USB_TEST open %s ret=%u", usb_test_name, (unsigned int)result);
  if(result != FR_OK) goto cleanup;
  result = f_read(&usb_test_file, usb_test_data, sizeof(usb_test_data), &bytes);
  plat_log_i("USB_TEST read ret=%u bytes=%u", (unsigned int)result, (unsigned int)bytes);
  close_result = f_close(&usb_test_file);
  if(close_result != FR_OK)
    plat_log_e("USB_TEST close ret=%u", (unsigned int)close_result);
  if(result != FR_OK || close_result != FR_OK || bytes > sizeof(usb_test_data))
    goto cleanup;
  read_ok = 1U;
  for(i = 0U; i < bytes; i += 16U)
  {
    for(j = 0U; j < 16U && i + j < bytes; j++)
    {
      uint8_t c = usb_test_data[i + j];
      hex[j * 3U] = digits[c >> 4];
      hex[j * 3U + 1U] = digits[c & 15U];
      hex[j * 3U + 2U] = ' ';
      ascii[j] = (c >= 32U && c < 127U) ? (char)c : '.';
    }
    hex[j * 3U] = 0; ascii[j] = 0;
    plat_log_i("USB_TEST %03u: %s |%s|", i, hex, ascii);
  }
write_test:
  if(otg_core_struct_fs2.host.conn_sts != 0U)
    write_ok = usb_test_write_verify();
cleanup:
  if(dir_open) (void)f_closedir(&usb_test_dir);
  close_result = f_mount(NULL, _T("1:"), 0U);
  fatfs_disk.is_initialized[1] = 0U;
  plat_log_i("USB_TEST done: list_ok=%u read_ok=%u write_ok=%u unmount=%u stack_free=%lu words",
      (unsigned int)list_ok, (unsigned int)read_ok, (unsigned int)write_ok,
      (unsigned int)close_result,
      (unsigned long)uxTaskGetStackHighWaterMark(NULL));
}

static void usb_test_poll(void)
{
  static uint8_t connected = 0U, tested = 0U;
  usbh_core_type *host = &otg_core_struct_fs2.host;
  if(host->conn_sts == 0U)
  {
    if(connected) plat_log_i("USB_TEST disconnected; reinsert to repeat");
    connected = 0U;
    tested = 0U;
    fatfs_disk.is_initialized[1] = 0U;
    return;
  }
  if(!connected)
  {
    connected = 1U;
    plat_log_i("USB_TEST attached, waiting for MSC ready");
  }
  if(tested || host->global_state != USBH_CLASS ||
     host->class_handler == NULL || host->class_handler->pdata == NULL ||
     usbh_msc.state != USBH_MSC_IDLE ||
     usbh_msc.l_unit_n[0].state != USBH_MSC_IDLE ||
     usbh_msc_is_ready(host, 0U) != MSC_OK)
    return;
  tested = 1U;
  if(fatfs_disk.disk[1] != &usbh_msc_disk || fatfs_disk.lun[1] != 0U)
  {
    plat_log_e("USB_TEST drive 1 not registered as USB LUN0");
    return;
  }
  usb_test_run();
}

/* add user code end 0 */

/* task handler */
TaskHandle_t start_test_tasks_handle;

/* Idle task control block and stack */
static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];
static StackType_t timer_task_stack[configTIMER_TASK_STACK_DEPTH];

static StaticTask_t idle_task_tcb;
static StaticTask_t timer_task_tcb;

/* External Idle and Timer task static memory allocation functions */
extern void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
extern void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, uint32_t * pulTimerTaskStackSize );

/*
  vApplicationGetIdleTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &idle_task_tcb;
  *ppxIdleTaskStackBuffer = &idle_task_stack[0];
  *pulIdleTaskStackSize = (uint32_t)configMINIMAL_STACK_SIZE;
}
/*
  vApplicationGetTimerTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, uint32_t * pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &timer_task_tcb;
  *ppxTimerTaskStackBuffer = &timer_task_stack[0];
  *pulTimerTaskStackSize = (uint32_t)configTIMER_TASK_STACK_DEPTH;
}

void vApplicationIdleHook( void )
{
  /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
   
/* add user code begin vApplicationIdleHook */

/* add user code end vApplicationIdleHook */
}

void vApplicationTickHook( void )
{
  /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */

/* add user code begin vApplicationTickHook */

/* add user code end vApplicationTickHook */
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */

/* add user code begin vApplicationStackOverflowHook */

/* add user code end vApplicationStackOverflowHook */
}

/* add user code begin 1 */

/* add user code end 1 */

/**
  * @brief  initializes all task.
  * @param  none
  * @retval none
  */
void freertos_task_create(void)
{
  /* create start_test_tasks task */
  xTaskCreate(start_or_test_f,
              "start_test_tasks",
              START_TEST_TASK_STACK_WORDS,
              NULL,
              0,
              &start_test_tasks_handle);
}

/**
  * @brief  freertos init and begin run.
  * @param  none
  * @retval none
  */
void wk_freertos_init(void)
{
  /* add user code begin freertos_init 0 */

  /* add user code end freertos_init 0 */

  /* enter critical */
  taskENTER_CRITICAL();

  freertos_task_create();
	
  /* add user code begin freertos_init 1 */

  /* add user code end freertos_init 1 */

  /* exit critical */
  taskEXIT_CRITICAL();

  /* start scheduler */
  vTaskStartScheduler();
}

/**
  * @brief start_test_tasks function.
  * @param  none
  * @retval none
  */
void start_or_test_f(void *pvParameters)
{
  /* add user code begin start_or_test_f 0 */
  platform_err_t log_ret;
  platform_err_t app_ret;
  platform_err_t gpio_ret;
  platform_err_t play_ret;
  platform_err_t presentation_ret;
  platform_err_t volume_ret;
  button_status_t button_ret;
  button_event_t button_event;
  plat_gpio_state_t key_sample;
  uint32_t now_ms;
  uint8_t button_index;
  uint8_t volume_ready = 0U;
  uint8_t volume_level = AUDIO_VOLUME_SERVICE_LEVEL_DEFAULT;
  audio_volume_service_t volume_service;
  button_t buttons[START_TEST_BUTTON_COUNT];
  uint8_t button_ready[START_TEST_BUTTON_COUNT] = {0U};
  uint8_t gpio_error_logged[START_TEST_BUTTON_COUNT] = {0U};
  static const plat_gpio_id_t button_gpio[START_TEST_BUTTON_COUNT] =
  {
    BOARD_GPIO_KEY1,
    BOARD_GPIO_KEY2,
    BOARD_GPIO_KEY3,
    BOARD_GPIO_KEY4
  };
  static const button_timing_t button_timing =
  {
    START_TEST_KEY_DEBOUNCE_MS,
    START_TEST_KEY_LONG_PRESS_MS,
    START_TEST_KEY_DOUBLE_CLICK_MS
  };
  static const uint8_t voice_synthesis_test_text[] =
  {
    0xC9U, 0xADU, 0xC1U, 0xD6U, 0xB7U, 0xC0U, 0xBBU, 0xF0U,
    0xC8U, 0xCBU, 0xC8U, 0xCBU, 0xD3U, 0xD0U, 0xD4U, 0xF0U,
    0xA3U, 0xACU, 0xB1U, 0xA3U, 0xBBU, 0xA4U, 0xC9U, 0xADU,
    0xC1U, 0xD6U, 0xBEU, 0xCDU, 0xCAU, 0xC7U, 0xB1U, 0xA3U,
    0xBBU, 0xA4U, 0xCEU, 0xD2U, 0xC3U, 0xC7U, 0xB5U, 0xC4U,
    0xBCU, 0xD2U, 0xD4U, 0xB0U, 0xA1U, 0xA3U, 0xBDU, 0xF8U,
    0xC8U, 0xEBU, 0xC1U, 0xD6U, 0xC7U, 0xF8U, 0xD1U, 0xCFU,
    0xBDU, 0xFBU, 0xD0U, 0xAFU, 0xB4U, 0xF8U, 0xBBU, 0xF0U,
    0xD6U, 0xD6U, 0xA3U, 0xACU, 0xD1U, 0xCFU, 0xBDU, 0xFBU,
    0xD2U, 0xB0U, 0xCDU, 0xE2U, 0xD3U, 0xC3U, 0xBBU, 0xF0U,
    0xA3U, 0xACU, 0xB7U, 0xA2U, 0xCFU, 0xD6U, 0xBBU, 0xF0U,
    0xC7U, 0xE9U, 0xC7U, 0xEBU, 0xC1U, 0xA2U, 0xBCU, 0xB4U,
    0xB1U, 0xA8U, 0xB8U, 0xE6U, 0xA3U, 0xACU, 0xCCU, 0xFDU,
    0xB4U, 0xD3U, 0xCFU, 0xD6U, 0xB3U, 0xA1U, 0xC8U, 0xCBU,
    0xD4U, 0xB1U, 0xD6U, 0xB8U, 0xBBU, 0xD3U, 0xA3U, 0xACU,
    0xB9U, 0xB2U, 0xCDU, 0xACU, 0xCAU, 0xD8U, 0xBBU, 0xA4U,
    0xC2U, 0xCCU, 0xCBU, 0xAEU, 0xC7U, 0xE0U, 0xC9U, 0xBDU,
    0xBAU, 0xCDU, 0xC3U, 0xC0U, 0xBAU, 0xC3U, 0xBCU, 0xD2U,
    0xD4U, 0xB0U, 0xA1U, 0xA3U, 0xB0U, 0xB2U, 0xC8U, 0xABU,
    0xD4U, 0xF0U, 0xC8U, 0xCEU, 0xD6U, 0xD8U, 0xD3U, 0xDAU,
    0xCCU, 0xA9U, 0xC9U, 0xBDU, 0xA3U, 0xACU, 0xB7U, 0xC0U,
    0xBBU, 0xF0U, 0xD2U, 0xE2U, 0xCAU, 0xB6U, 0xC0U, 0xCEU,
    0xBCU, 0xC7U, 0xD0U, 0xC4U, 0xBCU, 0xE4U, 0xA1U, 0xA3U
  };
  /* Temporary input: future RS485 supplies this ID through the same App API. */
  static const uint8_t remote_modem_test_id[] = "kvt9dxr84qrryr1z";
  static const uint8_t volume_decrease_prompt[] =
  {
    0xD2U, 0xF4U, 0xC1U, 0xBFU, 0xBCU, 0xF5U, 0xD0U, 0xA1U
  };
  static const uint8_t volume_increase_prompt[] =
  {
    0xD2U, 0xF4U, 0xC1U, 0xBFU, 0xD4U, 0xF6U, 0xBCU, 0xD3U
  };
  (void)pvParameters;

  /* add user code end start_or_test_f 0 */

  /* add user code begin start_or_test_f 2 */
  log_ret = plat_log_init();
  plat_log_i("USB_TEST enabled: FAT32 drive=1:/, root max=32, first file preview=128 bytes, TXT write+verify");
  plat_log_i("Audio MP3 application start, log_init=%d", (int32_t)log_ret);
  volume_ret = audio_volume_service_init(&volume_service,
                                         START_TEST_I2C_TIMEOUT_MS);
  if(PLATFORM_ERR_OK == volume_ret)
  {
    volume_ret = audio_volume_service_level_get(&volume_service,
                                                &volume_level);
  }
  if(PLATFORM_ERR_OK == volume_ret)
  {
    volume_ready = 1U;
  }
  plat_log_i("Audio volume service init ret=%d, volume=%u%%",
             (int32_t)volume_ret,
             (unsigned int)volume_level);
  presentation_ret = voice_presentation_app_init();
  plat_log_i("Voice presentation app init=%d, idle display/LEDs=off",
             (int32_t)presentation_ret);
  app_ret = audio_playback_app_init();
  {
    const remote_modem_app_io_t modem_io = {NULL, NULL, 1U};
    plat_log_i("4G application init=%d, test: newline echo enabled",
               (int32_t)remote_modem_app_init(&modem_io));
  }
  plat_log_i("Audio app init=%d, SW2=MP3, SW3=volume-5, SW4=volume+5/hold=4G config, SW5=voice synthesis",
              (int32_t)app_ret);

  now_ms = plat_tick_get_ms();
  for(button_index = 0U;
      button_index < START_TEST_BUTTON_COUNT;
      button_index++)
  {
    gpio_ret = plat_gpio_read(button_gpio[button_index], &key_sample);
    if(PLATFORM_ERR_OK == gpio_ret)
    {
      button_ret = button_init(&buttons[button_index],
                               &button_timing,
                               (uint8_t)(PLAT_GPIO_RESET == key_sample),
                               now_ms);
      if(BUTTON_STATUS_OK == button_ret)
      {
        button_ready[button_index] = 1U;
      }
    }

    if(0U == button_ready[button_index])
    {
      plat_log_e("SW%d (KEY%d) init failed",
                 (int32_t)(button_index + 2U),
                 (int32_t)(button_index + 1U));
    }
  }
  /* add user code end start_or_test_f 2 */

  /* Infinite loop */
  while(1)
  {
    /* when use usb,the function wk_usb_app_task() will be generated,
       which is the usb application layer code that users can improve themselves */
    wk_usb_app_task();

  /* add user code begin start_or_test_f 1 */
    usb_test_poll();

    now_ms = plat_tick_get_ms();
    for(button_index = 0U;
        button_index < START_TEST_BUTTON_COUNT;
        button_index++)
    {
      if(0U == button_ready[button_index])
      {
        continue;
      }

      gpio_ret = plat_gpio_read(button_gpio[button_index], &key_sample);
      if(PLATFORM_ERR_OK != gpio_ret)
      {
        if(0U == gpio_error_logged[button_index])
        {
          plat_log_e("SW%d (KEY%d) read failed, ret=%d",
                     (int32_t)(button_index + 2U),
                     (int32_t)(button_index + 1U),
                     (int32_t)gpio_ret);
          gpio_error_logged[button_index] = 1U;
        }
        continue;
      }
      gpio_error_logged[button_index] = 0U;

      button_ret = button_update(&buttons[button_index],
                                 (uint8_t)(PLAT_GPIO_RESET == key_sample),
                                 now_ms,
                                 &button_event);
      if(BUTTON_STATUS_OK != button_ret)
      {
        plat_log_e("SW%d (KEY%d) update failed, ret=%d",
                   (int32_t)(button_index + 2U),
                   (int32_t)(button_index + 1U),
                   (int32_t)button_ret);
        button_ready[button_index] = 0U;
        continue;
      }

      if((2U == button_index) &&
         (BUTTON_EVENT_LONG_PRESS == button_event))
      {
        platform_err_t modem_ret;

        modem_ret = remote_modem_app_config_request(remote_modem_test_id,
            (uint16_t)(sizeof(remote_modem_test_id) - 1U));
        plat_log_i("SW4 4G config request accepted=%u ret=%d, test stack free=%lu words",
                   (unsigned int)(PLATFORM_ERR_OK == modem_ret), (int32_t)modem_ret,
                   (unsigned long)uxTaskGetStackHighWaterMark(NULL));
        continue;
      }
      if(BUTTON_EVENT_PRESS != button_event)
      {
        continue;
      }

      if(0U == button_index)
      {
        if(PLATFORM_ERR_OK == app_ret)
        {
          play_ret = audio_playback_app_play_default();
          plat_log_i("SW2 pressed, MP3 request enqueue ret=%d",
                     (int32_t)play_ret);
        }
      }
      else if(3U == button_index)
      {
//        platform_err_t vtx_ret;

//        vtx_ret = audio_playback_app_voice_speak(
//            BSP_VOICE_SYNTHESIS_ENCODING_GBK,
//            voice_synthesis_test_text,
//            (uint16_t)sizeof(voice_synthesis_test_text));
//        plat_log_i("SW5 pressed, VTX316 request enqueue ret=%d",
//                   (int32_t)vtx_ret);
      }
      else
      {
        int8_t volume_step;

        if(0U == volume_ready)
        {
          volume_ret = audio_volume_service_init(
              &volume_service,
              START_TEST_I2C_TIMEOUT_MS);
          if(PLATFORM_ERR_OK == volume_ret)
          {
            volume_ret = audio_volume_service_level_get(&volume_service,
                                                        &volume_level);
          }
          if(PLATFORM_ERR_OK == volume_ret)
          {
            volume_ready = 1U;
          }
          plat_log_i("SW%u audio volume retry init ret=%d, volume=%u%%",
                     (unsigned int)(button_index + 2U),
                     (int32_t)volume_ret,
                     (unsigned int)volume_level);
        }

        if(0U != volume_ready)
        {
          volume_step = (1U == button_index) ?
                        -(int8_t)START_TEST_VOLUME_STEP :
                        (int8_t)START_TEST_VOLUME_STEP;
          volume_ret = audio_volume_service_step(
              &volume_service,
              volume_step,
              START_TEST_I2C_TIMEOUT_MS,
              &volume_level);
          if(PLATFORM_ERR_OK == volume_ret)
          {
            if(1U == button_index)
            {
              play_ret = audio_playback_app_voice_speak(
                  BSP_VOICE_SYNTHESIS_ENCODING_GBK,
                  volume_decrease_prompt,
                  (uint16_t)sizeof(volume_decrease_prompt));
            }
            else
            {
              play_ret = audio_playback_app_voice_speak(
                  BSP_VOICE_SYNTHESIS_ENCODING_GBK,
                  volume_increase_prompt,
                  (uint16_t)sizeof(volume_increase_prompt));
            }
            plat_log_i("SW%u volume prompt enqueue ret=%d",
                       (unsigned int)(button_index + 2U),
                       (int32_t)play_ret);
          }
          else
          {
            volume_ready = 0U;
          }
          plat_log_i("SW%u audio volume step=%d ret=%d, volume=%u%%",
                     (unsigned int)(button_index + 2U),
                     (int32_t)volume_step,
                     (int32_t)volume_ret,
                     (unsigned int)volume_level);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(START_TEST_KEY_POLL_MS));

  /* add user code end start_or_test_f 1 */
  }
}


/* add user code begin 2 */

/* add user code end 2 */

