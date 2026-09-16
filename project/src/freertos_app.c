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
#include "audio_volume_service.h"
#include "button.h"
#include "led_indicator_app.h"
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
  platform_err_t led_ret;
  platform_err_t gpio_ret;
  platform_err_t play_ret;
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
    0xD3U, 0xEEU, 0xD2U, 0xF4U, 0xCCU, 0xECU, 0xCFU, 0xC2U
  };
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
  led_ret = led_indicator_app_init();
  plat_log_i("LED indicator app init=%d", (int32_t)led_ret);
  app_ret = audio_playback_app_init();
  plat_log_i("Audio app init=%d, SW2=MP3, SW3=volume-5, SW4=volume+5, SW5=VTX316",
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
  plat_gpio_write(BOARD_GPIO_HUB_OE245,PLAT_GPIO_RESET);
  plat_delay_ms(100);
  plat_gpio_write(BOARD_GPIO_HUB_A     ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_B     ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_C     ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_D     ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_E     ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_LAT   ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_R1    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_G1    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_B1    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_R2    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_G2    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_B2    ,PLAT_GPIO_SET);
  plat_gpio_write(BOARD_GPIO_HUB_SCK   ,PLAT_GPIO_SET);
//  plat_gpio_write(BOARD_GPIO_HUB_C,PLAT_GPIO_SET);
//  plat_gpio_write(BOARD_GPIO_HUB_D,PLAT_GPIO_SET);
//  plat_gpio_write(BOARD_GPIO_HUB_B,PLAT_GPIO_SET);
//  plat_gpio_write(BOARD_GPIO_HUB_C,PLAT_GPIO_SET);
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
        platform_err_t vtx_ret;

        vtx_ret = audio_playback_app_voice_speak(
            BSP_VOICE_SYNTHESIS_ENCODING_GBK,
            voice_synthesis_test_text,
            (uint16_t)sizeof(voice_synthesis_test_text));
        plat_log_i("SW5 pressed, VTX316 request enqueue ret=%d",
                   (int32_t)vtx_ret);
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

