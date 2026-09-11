/* add user code begin Header */
/**
  ******************************************************************************
  * File Name          : freertos_app.c
  * Description        : Code for freertos applications
  */
/* add user code end Header */

/* Includes ------------------------------------------------------------------*/
#include "freertos_app.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "plat_log.h"
#include "plat_gpio.h"
#include "plat_sys.h"
#include "board_resources.h"
#include "audio_playback_app.h"
#include "button.h"
/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */
#define START_TEST_TASK_STACK_WORDS       512U
#define START_TEST_KEY_POLL_MS             10U
#define START_TEST_KEY_DEBOUNCE_MS         30U
#define START_TEST_KEY_LONG_PRESS_MS     1000U
#define START_TEST_KEY_DOUBLE_CLICK_MS    300U
#define START_TEST_BUTTON_COUNT             3U

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
  platform_err_t play_ret;
  button_status_t button_ret;
  button_event_t button_event;
  plat_gpio_state_t key_sample;
  uint32_t now_ms;
  uint8_t button_index;
  uint8_t emergency_active = 0U;
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
  static const uint8_t voice_synthesis_test_text[] =
  {
    0xD3U, 0xEEU, 0xD2U, 0xF4U, 0xCCU, 0xECU, 0xCFU, 0xC2U
  };
  (void)pvParameters;

  /* add user code end start_or_test_f 0 */

  /* add user code begin start_or_test_f 2 */
  log_ret = plat_log_init();
  plat_log_i("Audio MP3 application start, log_init=%d", (int32_t)log_ret);
  app_ret = audio_playback_app_init();
  plat_log_i("Audio app init=%d, KEY1=MP3, KEY2=VTX316, KEY3=emergency",
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
      plat_log_e("KEY%d init failed", (int32_t)(button_index + 1U));
    }
  }
  /* add user code end start_or_test_f 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin start_or_test_f 1 */
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
          plat_log_e("KEY%d read failed, ret=%d",
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
        plat_log_e("KEY%d update failed, ret=%d",
                   (int32_t)(button_index + 1U),
                   (int32_t)button_ret);
        button_ready[button_index] = 0U;
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
          plat_log_i("KEY1 pressed, MP3 request enqueue ret=%d",
                     (int32_t)play_ret);
        }
      }
      else if(1U == button_index)
      {
        platform_err_t vtx_ret;

        vtx_ret = audio_playback_app_voice_speak(
            BSP_VOICE_SYNTHESIS_ENCODING_GBK,
            voice_synthesis_test_text,
            (uint16_t)sizeof(voice_synthesis_test_text));
        plat_log_i("KEY2 pressed, VTX316 request enqueue ret=%d",
                   (int32_t)vtx_ret);
      }
      else
      {
        platform_err_t emergency_ret;
        uint8_t requested_active;

        requested_active = (0U == emergency_active) ? 1U : 0U;
        emergency_ret = audio_playback_app_emergency_set(requested_active);
        if(PLATFORM_ERR_OK == emergency_ret)
        {
          emergency_active = requested_active;
        }
        plat_log_i("KEY3 pressed, emergency enqueue active=%u ret=%d",
                   (unsigned int)requested_active,
                   (int32_t)emergency_ret);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(START_TEST_KEY_POLL_MS));

  /* add user code end start_or_test_f 1 */
  }
}


/* add user code begin 2 */

/* add user code end 2 */

