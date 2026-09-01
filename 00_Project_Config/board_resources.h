/**
 ******************************************************************************
 *@file               :   board_resources.h
 *@brief              :   Define vendor-independent resources fitted on the board.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef BOARD_RESOURCES_H
#define BOARD_RESOURCES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include "plat_resource.h"

/* define -------------------------------------------------------------------*/
/* GPIO outputs */
#define BOARD_GPIO_PIR_POWER        ((plat_gpio_id_t)0U)
#define BOARD_GPIO_E27_POWEREN      ((plat_gpio_id_t)1U)
#define BOARD_GPIO_LED_OUT_B        ((plat_gpio_id_t)2U)
#define BOARD_GPIO_LED_OUT_R        ((plat_gpio_id_t)3U)
#define BOARD_GPIO_LED_POWER_CS     ((plat_gpio_id_t)4U)
#define BOARD_GPIO_ALARM_OUT        ((plat_gpio_id_t)5U)
#define BOARD_GPIO_TTS_RST          ((plat_gpio_id_t)6U)
#define BOARD_GPIO_TPA_SDZ          ((plat_gpio_id_t)7U)
#define BOARD_GPIO_TPA_MUTE         ((plat_gpio_id_t)8U)
#define BOARD_GPIO_E27_RELOAD       ((plat_gpio_id_t)9U)
#define BOARD_GPIO_E27_RESET        ((plat_gpio_id_t)10U)
#define BOARD_GPIO_LED1_REMOTE      ((plat_gpio_id_t)11U)
#define BOARD_GPIO_LED2_UPDATA      ((plat_gpio_id_t)12U)
#define BOARD_GPIO_LED3_BRCAT       ((plat_gpio_id_t)13U)
#define BOARD_GPIO_LED4_RUN         ((plat_gpio_id_t)14U)
#define BOARD_GPIO_HUB_A            ((plat_gpio_id_t)15U)
#define BOARD_GPIO_HUB_B            ((plat_gpio_id_t)16U)
#define BOARD_GPIO_HUB_C            ((plat_gpio_id_t)17U)
#define BOARD_GPIO_HUB_D            ((plat_gpio_id_t)18U)
#define BOARD_GPIO_HUB_E            ((plat_gpio_id_t)19U)
#define BOARD_GPIO_HUB_LAT          ((plat_gpio_id_t)20U)
#define BOARD_GPIO_HUB_OE245        ((plat_gpio_id_t)21U)
#define BOARD_GPIO_HUB_R1           ((plat_gpio_id_t)22U)
#define BOARD_GPIO_HUB_G1           ((plat_gpio_id_t)23U)
#define BOARD_GPIO_HUB_B1           ((plat_gpio_id_t)24U)
#define BOARD_GPIO_HUB_R2           ((plat_gpio_id_t)25U)
#define BOARD_GPIO_HUB_G2           ((plat_gpio_id_t)26U)
#define BOARD_GPIO_HUB_B2           ((plat_gpio_id_t)27U)
#define BOARD_GPIO_HUB_SCK          ((plat_gpio_id_t)28U)

/* GPIO inputs */
#define BOARD_GPIO_TTS_R_B          ((plat_gpio_id_t)29U)
#define BOARD_GPIO_PIR_IN           ((plat_gpio_id_t)30U)
#define BOARD_GPIO_ALARM_IN         ((plat_gpio_id_t)31U)
#define BOARD_GPIO_TPA_FAULTZ       ((plat_gpio_id_t)32U)
#define BOARD_GPIO_KEY1             ((plat_gpio_id_t)33U)
#define BOARD_GPIO_KEY2             ((plat_gpio_id_t)34U)
#define BOARD_GPIO_KEY3             ((plat_gpio_id_t)35U)
#define BOARD_GPIO_KEY4             ((plat_gpio_id_t)36U)
#define BOARD_GPIO_RESOURCE_NUM     ((plat_gpio_id_t)37U)

#define BOARD_UART_PROTOCOL         ((plat_uart_id_t)0U)
#define BOARD_UART_RESOURCE_NUM     ((plat_uart_id_t)1U)

#define BOARD_I2S_AUDIO_OUTPUT      ((plat_i2s_id_t)0U)
#define BOARD_I2S_RESOURCE_NUM      ((plat_i2s_id_t)1U)

/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* function  ----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* BOARD_RESOURCES_H */
