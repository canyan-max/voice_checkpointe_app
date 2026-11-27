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
#include "sd_card_presence_app.h"
#include "bsp_rs485.h"
#include "comm_app.h"
#include "checkpoint_config.h"
#include "checkpoint_config_storage.h"
#include "proto_checkpoint.h"
#include "semphr.h"
#include "firmware_upgrade_app.h"
#include "usb_upgrade_port.h"
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
#define START_TEST_BUTTON_COUNT             3U
#define START_TEST_VOLUME_STEP              5U
#define START_TEST_I2C_TIMEOUT_MS          100U
#define START_TEST_4G_ENABLED               (0U)

#define CHECKPOINT_FLASH_TIMEOUT_MS         (100U)
#define CHECKPOINT_SAVE_QUIET_MS            (250U)
#define CHECKPOINT_SAVE_RETRY_MS           (1000U)
#define CHECKPOINT_PIR_MIN_INTERVAL_MS       (2000U)

/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */

/* Bound to the COMM task at startup; callbacks are its only runtime callers. */
static checkpoint_config_t s_checkpoint_config;
static uint8_t s_checkpoint_storage_ready;
static StaticSemaphore_t s_qspi_lock_storage;
static SemaphoreHandle_t p_qspi_lock;
static uint8_t s_checkpoint_save_pending;
static uint32_t s_checkpoint_save_since_ms;
static uint32_t s_checkpoint_save_wait_ms;

/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

static void checkpoint_storage_init(void)
{
  s_checkpoint_config.device_id = PROTO_CHECKPOINT_DEFAULT_ID;
  s_checkpoint_config.voice_repeat = CHECKPOINT_CONFIG_VOICE_REPEAT_DEFAULT;
  s_checkpoint_config.display_repeat = CHECKPOINT_CONFIG_DISPLAY_REPEAT_DEFAULT;
  s_checkpoint_config.pir_enabled = CHECKPOINT_CONFIG_PIR_ENABLED_DEFAULT;
  s_checkpoint_config.pir_item_number = CHECKPOINT_CONFIG_PIR_ITEM_DEFAULT;
  uint8_t formatted = 0U;
  storage_filesystem_service_ret_t ret =
      storage_filesystem_service_init(CHECKPOINT_FLASH_TIMEOUT_MS, &formatted);
  if(STORAGE_FILESYSTEM_SERVICE_RET_OK != ret)
  {
    plat_log_w("Checkpoint QSPI mount failed: ret=%d, defaults active", (int)ret);
    return;
  }
  s_checkpoint_storage_ready = 1U;
  ret = checkpoint_config_storage_load(&s_checkpoint_config);
  if(STORAGE_FILESYSTEM_SERVICE_RET_OK == ret)
  {
    plat_log_i("Checkpoint QSPI loaded: id=%02X stack_free=%lu words",
               (unsigned int)s_checkpoint_config.device_id,
               (unsigned long)uxTaskGetStackHighWaterMark(NULL));
  }
  else if(STORAGE_FILESYSTEM_SERVICE_RET_NOT_FOUND == ret)
  {
    plat_log_i("Checkpoint QSPI no config: defaults active, formatted=%u stack_free=%lu words",
               (unsigned int)formatted, (unsigned long)uxTaskGetStackHighWaterMark(NULL));
  }
  else
  {
    plat_log_w("Checkpoint QSPI load failed: ret=%d, defaults active", (int)ret);
  }
}

static platform_err_t checkpoint_config_mark_changed(platform_err_t ret)
{
  if((PLATFORM_ERR_OK == ret) && (0U != s_checkpoint_storage_ready))
  {
    s_checkpoint_save_pending = 1U;
    s_checkpoint_save_since_ms = plat_tick_get_ms();
    s_checkpoint_save_wait_ms = CHECKPOINT_SAVE_QUIET_MS;
  }
  return ret;
}

static void checkpoint_comm_device_id_changed(void *p_context, uint8_t device_id)
{
  checkpoint_config_t *p_config = p_context;
  if(NULL != p_config)
  {
    p_config->device_id = device_id;
    (void)checkpoint_config_mark_changed(PLATFORM_ERR_OK);
  }
}

static void checkpoint_comm_config_poll(void *p_context)
{
  const checkpoint_config_t *p_config = p_context;
  if((NULL == p_config) || (0U == s_checkpoint_save_pending))
  {
    return;
  }
  const uint32_t now_ms = plat_tick_get_ms();
  if((uint32_t)(now_ms - s_checkpoint_save_since_ms) < s_checkpoint_save_wait_ms)
  {
    return;
  }

  if (NULL == p_qspi_lock || pdTRUE != xSemaphoreTake(p_qspi_lock, 0U))
  {
    return;
  }
  const storage_filesystem_service_ret_t ret = checkpoint_config_storage_save(p_config);
  (void)xSemaphoreGive(p_qspi_lock);
  const uint32_t done_ms = plat_tick_get_ms();
  if(STORAGE_FILESYSTEM_SERVICE_RET_OK == ret)
  {
    s_checkpoint_save_pending = 0U;
    plat_log_i("Checkpoint QSPI saved: id=%02X took=%lu ms stack_free=%lu words",
               (unsigned int)p_config->device_id, (unsigned long)(done_ms - now_ms),
               (unsigned long)uxTaskGetStackHighWaterMark(NULL));
  }
  else
  {
    s_checkpoint_save_since_ms = done_ms;
    s_checkpoint_save_wait_ms = CHECKPOINT_SAVE_RETRY_MS;
    plat_log_w("Checkpoint QSPI save failed: ret=%d, retry scheduled", (int)ret);
  }
}

static platform_err_t checkpoint_comm_voice_set(void *p_context,
                                                uint8_t item_number,
                                                const uint8_t *p_text,
                                                uint16_t text_size)
{
  return checkpoint_config_mark_changed(
      checkpoint_config_voice_set(p_context, item_number, p_text, text_size));
}

static platform_err_t checkpoint_comm_voice_repeat_set(void *p_context,
                                                       uint8_t repeat_count)
{
  return checkpoint_config_mark_changed(
      checkpoint_config_voice_repeat_set(p_context, repeat_count));
}

static platform_err_t checkpoint_comm_display_set(void *p_context,
                                                  uint8_t item_number,
                                                  const uint8_t *p_text,
                                                  uint16_t text_size)
{
  return checkpoint_config_mark_changed(
      checkpoint_config_display_set(p_context, item_number, p_text, text_size));
}

static platform_err_t checkpoint_comm_display_repeat_set(void *p_context,
                                                         uint8_t repeat_count)
{
  return checkpoint_config_mark_changed(
      checkpoint_config_display_repeat_set(p_context, repeat_count));
}

static platform_err_t checkpoint_comm_play(void *p_context, uint8_t item_number)
{
  const checkpoint_config_t *p_config = p_context;
  const checkpoint_item_config_t *p_item =
      checkpoint_config_item_get(p_config, item_number);

  if((NULL == p_item) || (0U == p_item->voice_text_size))
  {
    return PLATFORM_ERR_PARAM;
  }

  const audio_playback_voice_request_t request =
  {
    .encoding = BSP_VOICE_SYNTHESIS_ENCODING_GBK,
    .p_text = p_item->voice_text,
    .text_size = p_item->voice_text_size,
    .repeat_count = p_config->voice_repeat,
    .p_caption_gbk = p_item->display_text,
    .caption_size = p_item->display_text_size,
    .caption_cycles = (0U == p_config->display_repeat) ? 1U : p_config->display_repeat
  };
  return audio_playback_app_voice_request(&request);
}

static platform_err_t checkpoint_comm_stop(void *p_context)
{
  (void)p_context;
  return audio_playback_app_stop();
}

/* ISR owns the counters. COMM consumes PIR edges; the board test reports both inputs. */
static volatile uint32_t gpio_irq_test_count[2];
static uint32_t s_pir_edge_consumed;
static uint32_t s_pir_last_accepted_ms;
static uint8_t s_pir_edge_accepted;
static uint32_t gpio_irq_test_reported[2];
static const plat_gpio_id_t gpio_irq_test_ids[2] =
{
  BOARD_GPIO_ALARM_IN, BOARD_GPIO_PIR_IN
};
static const char *const gpio_irq_test_names[2] =
{
  "ALARM_IN PD7", "PIR_IN PC10"
};

static void gpio_irq_test_callback(plat_gpio_id_t id)
{
  if(id == BOARD_GPIO_ALARM_IN)
    gpio_irq_test_count[0]++;
  else if(id == BOARD_GPIO_PIR_IN)
    gpio_irq_test_count[1]++;
}

static platform_err_t checkpoint_comm_pir_enable_set(void *p_context, uint8_t enabled)
{
  checkpoint_config_t *p_config = p_context;
  if(NULL == p_config)
  {
    return PLATFORM_ERR_PARAM;
  }

  const uint8_t was_enabled = p_config->pir_enabled;
  const platform_err_t ret = checkpoint_config_pir_enable_set(p_config, enabled);
  if((PLATFORM_ERR_OK == ret) && (0U == was_enabled) && (0U != enabled))
  {
    /* Ignore edges from before PIR was enabled. Only the ISR updates the counter. */
    s_pir_edge_consumed = gpio_irq_test_count[1U];
    s_pir_edge_accepted = 0U;
  }
  return (was_enabled != enabled) ? checkpoint_config_mark_changed(ret) : ret;
}

static platform_err_t checkpoint_comm_pir_item_set(void *p_context, uint8_t item_number)
{
  return checkpoint_config_mark_changed(checkpoint_config_pir_item_set(p_context, item_number));
}

static void checkpoint_comm_pir_poll(void *p_context)
{
  const checkpoint_config_t *p_config = p_context;
  const uint32_t edge_count = gpio_irq_test_count[1U];
  if((NULL == p_config) || (edge_count == s_pir_edge_consumed))
  {
    return;
  }

  /* Coalesce edges between polls; do not retry a rejected request. */
  s_pir_edge_consumed = edge_count;
  if(0U != p_config->pir_enabled)
  {
    const uint32_t now_ms = plat_tick_get_ms();
    if((0U != s_pir_edge_accepted) &&
       ((uint32_t)(now_ms - s_pir_last_accepted_ms) < CHECKPOINT_PIR_MIN_INTERVAL_MS))
    {
      return;
    }

    /* Ignored edges do not extend the window, even if the player is busy. */
    s_pir_last_accepted_ms = now_ms;
    s_pir_edge_accepted = 1U;
    const uint8_t item_number = p_config->pir_item_number;
    const platform_err_t ret = checkpoint_comm_play(p_context, item_number);
    if(PLATFORM_ERR_OK == ret)
    {
      plat_log_i("PIR falling edge, voice enqueued: item=%u stack_free=%lu words",
                 (unsigned int)item_number,
                 (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    }
    else
    {
      plat_log_w("PIR falling edge, voice not enqueued: item=%u ret=%d",
                 (unsigned int)item_number, (int)ret);
    }
  }
}

static void gpio_irq_test_init(void)
{
  unsigned int index;
  platform_err_t result;

  for(index = 0U; index < 2U; index++)
  {
    result = plat_gpio_irq_disable(gpio_irq_test_ids[index]);
    if(result == PLATFORM_ERR_OK)
      result = plat_gpio_set_irq_callback(gpio_irq_test_ids[index],
                                           gpio_irq_test_callback);
    if(result == PLATFORM_ERR_OK)
      result = plat_gpio_irq_enable(gpio_irq_test_ids[index]);
    if(result == PLATFORM_ERR_OK)
      plat_log_i("EXTI_TEST ready: %s falling edge", gpio_irq_test_names[index]);
    else
      plat_log_e("EXTI_TEST init failed: %s ret=%d",
                 gpio_irq_test_names[index], (int)result);
  }
}

static void gpio_irq_test_poll(void)
{
  uint32_t snapshot[2];
  uint32_t delta;
  unsigned int index;

  /* Priority-5 EXINTs are masked only during this two-word snapshot. */
  taskENTER_CRITICAL();
  snapshot[0] = gpio_irq_test_count[0];
  snapshot[1] = gpio_irq_test_count[1];
  taskEXIT_CRITICAL();

  for(index = 0U; index < 2U; index++)
  {
    delta = snapshot[index] - gpio_irq_test_reported[index];
    if(delta != 0U)
    {
      gpio_irq_test_reported[index] = snapshot[index];
      plat_log_i("EXTI_TEST %s falling: delta=%lu total=%lu stack_free=%lu words",
                 gpio_irq_test_names[index], (unsigned long)delta,
                 (unsigned long)snapshot[index],
                 (unsigned long)uxTaskGetStackHighWaterMark(NULL));
    }
  }
}

static uint8_t checkpoint_qspi_try_lock(void)
{
  return NULL != p_qspi_lock && pdTRUE == xSemaphoreTake(p_qspi_lock, 0U);
}

static void checkpoint_qspi_unlock(void)
{
  (void)xSemaphoreGive(p_qspi_lock);
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
              512,
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
    BOARD_GPIO_KEY3
  };
  static const button_timing_t button_timing =
  {
    START_TEST_KEY_DEBOUNCE_MS,
    START_TEST_KEY_LONG_PRESS_MS,
    START_TEST_KEY_DOUBLE_CLICK_MS
  };
#if START_TEST_4G_ENABLED
  /* Temporary input: future RS485 supplies this ID through the same App API. */
  static const uint8_t remote_modem_test_id[] = "kvt9dxr84qrryr1z";
#endif
  (void)pvParameters;

  /* add user code end start_or_test_f 0 */

  /* add user code begin start_or_test_f 2 */
  log_ret = plat_log_init();
  gpio_irq_test_init();
  p_qspi_lock = xSemaphoreCreateMutexStatic(&s_qspi_lock_storage);
  if (NULL == p_qspi_lock)
  {
    plat_log_e("QSPI lock unavailable");
  }
  checkpoint_storage_init();
  if (0U != s_checkpoint_storage_ready && NULL != p_qspi_lock)
  {
    const firmware_upgrade_lock_t upgrade_lock =
    {
      .pf_try_lock = checkpoint_qspi_try_lock,
      .pf_unlock = checkpoint_qspi_unlock
    };
    firmware_upgrade_app_init(&upgrade_lock);
  }
  plat_log_i("TF_CD_TEST app init ret=%d, debounce=50 ms",
             (int)sd_card_presence_app_init());
  sd_card_presence_app_poll();
  plat_log_i("USB_OTA enabled: FAT32 UPDATE.BIN -> QSPI, SW2=start");
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
    const comm_app_io_t comm_io =
    {
      .pf_init = bsp_rs485_init,
      .pf_receive = bsp_rs485_poll,
      .pf_send = bsp_rs485_send,
      .pf_get_rx_progress = bsp_rs485_get_rx_progress
    };
    const comm_app_actions_t comm_actions =
    {
      .p_context = &s_checkpoint_config,
      .pf_voice_set = checkpoint_comm_voice_set,
      .pf_voice_repeat_set = checkpoint_comm_voice_repeat_set,
      .pf_display_set = checkpoint_comm_display_set,
      .pf_display_repeat_set = checkpoint_comm_display_repeat_set,
      .pf_play = checkpoint_comm_play,
      .pf_stop = checkpoint_comm_stop,
      .pf_pir_enable_set = checkpoint_comm_pir_enable_set,
      .pf_pir_item_set = checkpoint_comm_pir_item_set,
      .pf_pir_poll = checkpoint_comm_pir_poll,
      .pf_device_id_changed = checkpoint_comm_device_id_changed,
      .pf_config_poll = checkpoint_comm_config_poll
    };
    s_pir_edge_consumed = gpio_irq_test_count[1U];
    const platform_err_t comm_ret = comm_app_init(&comm_io, &comm_actions,
                                                  s_checkpoint_config.device_id);
    if (PLATFORM_ERR_OK != comm_ret)
      plat_log_e("COMM startup failed: ret=%d", (int)comm_ret);
  }
#if START_TEST_4G_ENABLED
  {
    const remote_modem_app_io_t modem_io = {NULL, NULL, 1U};
    plat_log_i("4G application init=%d, test: newline echo enabled",
               (int32_t)remote_modem_app_init(&modem_io));
  }
#endif
  plat_log_i("Audio app init=%d, SW3=volume-5, SW4=volume+5", (int32_t)app_ret);

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
    gpio_irq_test_poll();
    sd_card_presence_app_poll();
    usb_upgrade_port_poll();

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

#if START_TEST_4G_ENABLED
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
#endif
      if(BUTTON_EVENT_PRESS != button_event)
      {
        continue;
      }

      if(0U == button_index)
      {
        usb_upgrade_port_request();
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
          if(PLATFORM_ERR_OK != volume_ret)
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

    vTaskDelay(usb_upgrade_port_is_active() ? 1U : pdMS_TO_TICKS(START_TEST_KEY_POLL_MS));

  /* add user code end start_or_test_f 1 */
  }
}


/* add user code begin 2 */

/* add user code end 2 */

