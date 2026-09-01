/**
 ******************************************************************************
 *@file               :   board_at32f435_binding.h
 *@brief              :   Bind board GPIO resources to the AT32F435 target.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef BOARD_AT32F435_BINDING_H
#define BOARD_AT32F435_BINDING_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include "at32f435_437_wk_config.h"

/* define -------------------------------------------------------------------*/
/* GPIO outputs */
#define BOARD_GPIO_PIR_POWER_PORT        PIR_POWER_PA5_GPIO_PORT
#define BOARD_GPIO_PIR_POWER_PIN         PIR_POWER_PA5_PIN
#define BOARD_GPIO_E27_POWEREN_PORT      E27_POWEREN_PA11_GPIO_PORT
#define BOARD_GPIO_E27_POWEREN_PIN       E27_POWEREN_PA11_PIN
#define BOARD_GPIO_LED_OUT_B_PORT        LED_OUT_B_PC7_GPIO_PORT
#define BOARD_GPIO_LED_OUT_B_PIN         LED_OUT_B_PC7_PIN
#define BOARD_GPIO_LED_OUT_R_PORT        LED_OUT_R_PC8_GPIO_PORT
#define BOARD_GPIO_LED_OUT_R_PIN         LED_OUT_R_PC8_PIN
#define BOARD_GPIO_LED_POWER_CS_PORT     LED_POWER_CS_PC9_GPIO_PORT
#define BOARD_GPIO_LED_POWER_CS_PIN      LED_POWER_CS_PC9_PIN
#define BOARD_GPIO_ALARM_OUT_PORT        ALARM_OUT_PB7_GPIO_PORT
#define BOARD_GPIO_ALARM_OUT_PIN         ALARM_OUT_PB7_PIN
#define BOARD_GPIO_TTS_RST_PORT          TTS_RST_PB9_GPIO_PORT
#define BOARD_GPIO_TTS_RST_PIN           TTS_RST_PB9_PIN
#define BOARD_GPIO_TPA_SDZ_PORT          TPA_SDZ_PE5_GPIO_PORT
#define BOARD_GPIO_TPA_SDZ_PIN           TPA_SDZ_PE5_PIN
#define BOARD_GPIO_TPA_MUTE_PORT         TPA_MUTE_PE6_GPIO_PORT
#define BOARD_GPIO_TPA_MUTE_PIN          TPA_MUTE_PE6_PIN
#define BOARD_GPIO_E27_RELOAD_PORT       E27_RELOAD_PE7_GPIO_PORT
#define BOARD_GPIO_E27_RELOAD_PIN        E27_RELOAD_PE7_PIN
#define BOARD_GPIO_E27_RESET_PORT        E27_RESET_PE8_GPIO_PORT
#define BOARD_GPIO_E27_RESET_PIN         E27_RESET_PE8_PIN
#define BOARD_GPIO_LED1_REMOTE_PORT      LED1_REMOTE_PE12_GPIO_PORT
#define BOARD_GPIO_LED1_REMOTE_PIN       LED1_REMOTE_PE12_PIN
#define BOARD_GPIO_LED2_UPDATA_PORT      LED2_UPDATA_PE13_GPIO_PORT
#define BOARD_GPIO_LED2_UPDATA_PIN       LED2_UPDATA_PE13_PIN
#define BOARD_GPIO_LED3_BRCAT_PORT       LED3_BRCAT_PE14_GPIO_PORT
#define BOARD_GPIO_LED3_BRCAT_PIN        LED3_BRCAT_PE14_PIN
#define BOARD_GPIO_LED4_RUN_PORT         LED4_RUN_PE15_GPIO_PORT
#define BOARD_GPIO_LED4_RUN_PIN          LED4_RUN_PE15_PIN
#define BOARD_GPIO_HUB_A_PORT            HUB_A_PD0_GPIO_PORT
#define BOARD_GPIO_HUB_A_PIN             HUB_A_PD0_PIN
#define BOARD_GPIO_HUB_B_PORT            HUB_B_PD1_GPIO_PORT
#define BOARD_GPIO_HUB_B_PIN             HUB_B_PD1_PIN
#define BOARD_GPIO_HUB_C_PORT            HUB_C_PD2_GPIO_PORT
#define BOARD_GPIO_HUB_C_PIN             HUB_C_PD2_PIN
#define BOARD_GPIO_HUB_D_PORT            HUB_D_PD3_GPIO_PORT
#define BOARD_GPIO_HUB_D_PIN             HUB_D_PD3_PIN
#define BOARD_GPIO_HUB_E_PORT            HUB_E_PD4_GPIO_PORT
#define BOARD_GPIO_HUB_E_PIN             HUB_E_PD4_PIN
#define BOARD_GPIO_HUB_LAT_PORT          HUB_LAT_PD5_GPIO_PORT
#define BOARD_GPIO_HUB_LAT_PIN           HUB_LAT_PD5_PIN
#define BOARD_GPIO_HUB_OE245_PORT        HUB_OE245_PD6_GPIO_PORT
#define BOARD_GPIO_HUB_OE245_PIN         HUB_OE245_PD6_PIN
#define BOARD_GPIO_HUB_R1_PORT           HUB_R1_PD8_GPIO_PORT
#define BOARD_GPIO_HUB_R1_PIN            HUB_R1_PD8_PIN
#define BOARD_GPIO_HUB_G1_PORT           HUB_G1_PD9_GPIO_PORT
#define BOARD_GPIO_HUB_G1_PIN            HUB_G1_PD9_PIN
#define BOARD_GPIO_HUB_B1_PORT           HUB_B1_PD10_GPIO_PORT
#define BOARD_GPIO_HUB_B1_PIN            HUB_B1_PD10_PIN
#define BOARD_GPIO_HUB_R2_PORT           HUB_R2_PD11_GPIO_PORT
#define BOARD_GPIO_HUB_R2_PIN            HUB_R2_PD11_PIN
#define BOARD_GPIO_HUB_G2_PORT           HUB_G2_PD12_GPIO_PORT
#define BOARD_GPIO_HUB_G2_PIN            HUB_G2_PD12_PIN
#define BOARD_GPIO_HUB_B2_PORT           HUB_B2_PD13_GPIO_PORT
#define BOARD_GPIO_HUB_B2_PIN            HUB_B2_PD13_PIN
#define BOARD_GPIO_HUB_SCK_PORT          HUB_SCK_PD15_GPIO_PORT
#define BOARD_GPIO_HUB_SCK_PIN           HUB_SCK_PD15_PIN

/* GPIO inputs */
#define BOARD_GPIO_TTS_R_B_PORT          TTS_R_B_PB8_GPIO_PORT
#define BOARD_GPIO_TTS_R_B_PIN           TTS_R_B_PB8_PIN
#define BOARD_GPIO_PIR_IN_PORT           PIR_IN_PC10_GPIO_PORT
#define BOARD_GPIO_PIR_IN_PIN            PIR_IN_PC10_PIN
#define BOARD_GPIO_ALARM_IN_PORT         ALARM_IN_PD7_GPIO_PORT
#define BOARD_GPIO_ALARM_IN_PIN          ALARM_IN_PD7_PIN
#define BOARD_GPIO_TPA_FAULTZ_PORT       TPA_FAULTZ_PE4_GPIO_PORT
#define BOARD_GPIO_TPA_FAULTZ_PIN        TPA_FAULTZ_PE4_PIN
#define BOARD_GPIO_KEY1_PORT             KEY1_PE0_GPIO_PORT
#define BOARD_GPIO_KEY1_PIN              KEY1_PE0_PIN
#define BOARD_GPIO_KEY2_PORT             KEY2_PE1_GPIO_PORT
#define BOARD_GPIO_KEY2_PIN              KEY2_PE1_PIN
#define BOARD_GPIO_KEY3_PORT             KEY3_PE2_GPIO_PORT
#define BOARD_GPIO_KEY3_PIN              KEY3_PE2_PIN
#define BOARD_GPIO_KEY4_PORT             KEY4_PE3_GPIO_PORT
#define BOARD_GPIO_KEY4_PIN              KEY4_PE3_PIN

#define BOARD_UART_PROTOCOL_INSTANCE          USART2
#define BOARD_UART_PROTOCOL_RX_DMA            DMA1_CHANNEL3
#define BOARD_UART_PROTOCOL_RX_DMA_GLOBAL     DMA1_GL3_FLAG
#define BOARD_UART_PROTOCOL_RX_DMA_HALF       DMA1_HDT3_FLAG
#define BOARD_UART_PROTOCOL_RX_DMA_FULL       DMA1_FDT3_FLAG
#define BOARD_UART_PROTOCOL_RX_DMA_ERROR      DMA1_DTERR3_FLAG

#define BOARD_I2S_AUDIO_OUTPUT_INSTANCE       SPI1
#define BOARD_I2S_AUDIO_OUTPUT_TX_DMA         DMA1_CHANNEL1
#define BOARD_I2S_AUDIO_OUTPUT_TX_DMA_HALF    DMA1_HDT1_FLAG
#define BOARD_I2S_AUDIO_OUTPUT_TX_DMA_FULL    DMA1_FDT1_FLAG
#define BOARD_I2S_AUDIO_OUTPUT_TX_DMA_ERROR   DMA1_DTERR1_FLAG

/* typedef ------------------------------------------------------------------*/

/* variables ----------------------------------------------------------------*/

/* function  ----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* BOARD_AT32F435_BINDING_H */
