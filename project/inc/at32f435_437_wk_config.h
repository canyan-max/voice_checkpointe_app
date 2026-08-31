/* add user code begin Header */
/**
  **************************************************************************
  * @file     at32f435_437_wk_config.h
  * @brief    header file of work bench config
  **************************************************************************
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

/* define to prevent recursive inclusion -----------------------------------*/
#ifndef __AT32F435_437_WK_CONFIG_H
#define __AT32F435_437_WK_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes -----------------------------------------------------------------------*/
#include "stdio.h"
#include "at32f435_437.h"
/* private includes -------------------------------------------------------------*/
/* add user code begin private includes */

/* add user code end private includes */

/* exported types -------------------------------------------------------------*/
/* add user code begin exported types */

/* add user code end exported types */

/* exported constants --------------------------------------------------------*/
/* add user code begin exported constants */

/* add user code end exported constants */

/* exported macro ------------------------------------------------------------*/
/* add user code begin exported macro */

/* add user code end exported macro */

/* add user code begin dma define */
/* user can only modify the dma define value */
#define DMA1_CHANNEL1_BUFFER_SIZE   0
#define DMA1_CHANNEL1_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL1_PERIPHERAL_BASE_ADDR  0

#define DMA1_CHANNEL2_BUFFER_SIZE   0
#define DMA1_CHANNEL2_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL2_PERIPHERAL_BASE_ADDR   0

#define DMA1_CHANNEL3_BUFFER_SIZE   0
#define DMA1_CHANNEL3_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL3_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL4_BUFFER_SIZE   0
//#define DMA1_CHANNEL4_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL4_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL5_BUFFER_SIZE   0
//#define DMA1_CHANNEL5_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL5_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL6_BUFFER_SIZE   0
//#define DMA1_CHANNEL6_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL6_PERIPHERAL_BASE_ADDR   0

//#define DMA1_CHANNEL7_BUFFER_SIZE   0
//#define DMA1_CHANNEL7_MEMORY_BASE_ADDR   0
//#define DMA1_CHANNEL7_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL1_BUFFER_SIZE   0
//#define DMA2_CHANNEL1_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL1_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL2_BUFFER_SIZE   0
//#define DMA2_CHANNEL2_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL2_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL3_BUFFER_SIZE   0
//#define DMA2_CHANNEL3_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL3_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL4_BUFFER_SIZE   0
//#define DMA2_CHANNEL4_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL4_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL5_BUFFER_SIZE   0
//#define DMA2_CHANNEL5_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL5_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL6_BUFFER_SIZE   0
//#define DMA2_CHANNEL6_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL6_PERIPHERAL_BASE_ADDR   0

//#define DMA2_CHANNEL7_BUFFER_SIZE   0
//#define DMA2_CHANNEL7_MEMORY_BASE_ADDR   0
//#define DMA2_CHANNEL7_PERIPHERAL_BASE_ADDR   0
 
#define EDMA_STREAM1_BUFFER_SIZE   0
#define EDMA_STREAM1_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM1_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM1_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM1_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM2_BUFFER_SIZE   0
//#define EDMA_STREAM2_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM2_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM2_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM2_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM3_BUFFER_SIZE   0
//#define EDMA_STREAM3_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM3_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM3_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM3_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM4_BUFFER_SIZE   0
//#define EDMA_STREAM4_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM4_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM4_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM4_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM5_BUFFER_SIZE   0
//#define EDMA_STREAM5_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM5_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM5_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM5_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM6_BUFFER_SIZE   0
//#define EDMA_STREAM6_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM6_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM6_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM6_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM7_BUFFER_SIZE   0
//#define EDMA_STREAM7_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM7_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM7_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM7_LINK_LIST_POINTER   0
 
//#define EDMA_STREAM8_BUFFER_SIZE   0
//#define EDMA_STREAM8_MEMORY0_BASE_ADDR   0
//#define EDMA_STREAM8_PERIPHERAL_BASE_ADDR   0
//#define EDMA_STREAM8_MEMORY1_BASE_ADDR   0
//#define EDMA_STREAM8_LINK_LIST_POINTER   0
/* add user code end dma define */

/* Private defines -------------------------------------------------------------*/
#define KEY3_PE2_PIN    GPIO_PINS_2
#define KEY3_PE2_GPIO_PORT    GPIOE
#define KEY4_PE3_PIN    GPIO_PINS_3
#define KEY4_PE3_GPIO_PORT    GPIOE
#define TPA_FAULTZ_PE4_PIN    GPIO_PINS_4
#define TPA_FAULTZ_PE4_GPIO_PORT    GPIOE
#define TPA_SDZ_PE5_PIN    GPIO_PINS_5
#define TPA_SDZ_PE5_GPIO_PORT    GPIOE
#define TPA_MUTE_PE6_PIN    GPIO_PINS_6
#define TPA_MUTE_PE6_GPIO_PORT    GPIOE
#define SDIO2_TFCD_PC13_PIN    GPIO_PINS_13
#define SDIO2_TFCD_PC13_GPIO_PORT    GPIOC
#define SDDIO2_TFD0_PC0_PIN    GPIO_PINS_0
#define SDDIO2_TFD0_PC0_GPIO_PORT    GPIOC
#define SDDIO2_TFD1_PC1_PIN    GPIO_PINS_1
#define SDDIO2_TFD1_PC1_GPIO_PORT    GPIOC
#define SDDIO2_TFD2_PC2_PIN    GPIO_PINS_2
#define SDDIO2_TFD2_PC2_GPIO_PORT    GPIOC
#define SDDIO2_TFD3_PC3_PIN    GPIO_PINS_3
#define SDDIO2_TFD3_PC3_GPIO_PORT    GPIOC
#define AUDIO_IN_ADC_PA0_PIN    GPIO_PINS_0
#define AUDIO_IN_ADC_PA0_GPIO_PORT    GPIOA
#define BATV_ADC_PA1_PIN    GPIO_PINS_1
#define BATV_ADC_PA1_GPIO_PORT    GPIOA
#define SDDIO2_TFCLX_PA2_PIN    GPIO_PINS_2
#define SDDIO2_TFCLX_PA2_GPIO_PORT    GPIOA
#define SDDIO2_TFCMD_PA3_PIN    GPIO_PINS_3
#define SDDIO2_TFCMD_PA3_GPIO_PORT    GPIOA
#define IIS_LR_WS_PA4_PIN    GPIO_PINS_4
#define IIS_LR_WS_PA4_GPIO_PORT    GPIOA
#define PIR_POWER_PA5_PIN    GPIO_PINS_5
#define PIR_POWER_PA5_GPIO_PORT    GPIOA
#define QSPI1_SI0_PA6_PIN    GPIO_PINS_6
#define QSPI1_SI0_PA6_GPIO_PORT    GPIOA
#define QSPI1_SI1_PA7_PIN    GPIO_PINS_7
#define QSPI1_SI1_PA7_GPIO_PORT    GPIOA
#define QSPI1_SI2_PC4_PIN    GPIO_PINS_4
#define QSPI1_SI2_PC4_GPIO_PORT    GPIOC
#define QSPI1_SI3_PC5_PIN    GPIO_PINS_5
#define QSPI1_SI3_PC5_GPIO_PORT    GPIOC
#define USART2_RX_PB0_PIN    GPIO_PINS_0
#define USART2_RX_PB0_GPIO_PORT    GPIOB
#define QSPI1_SCK_PB1_PIN    GPIO_PINS_1
#define QSPI1_SCK_PB1_GPIO_PORT    GPIOB
#define SPI3GT_MO_PB2_PIN    GPIO_PINS_2
#define SPI3GT_MO_PB2_GPIO_PORT    GPIOB
#define E27_RELOAD_PE7_PIN    GPIO_PINS_7
#define E27_RELOAD_PE7_GPIO_PORT    GPIOE
#define E27_RESET_PE8_PIN    GPIO_PINS_8
#define E27_RESET_PE8_GPIO_PORT    GPIOE
#define DEBUG_SCOPE_PE9_PIN    GPIO_PINS_9
#define DEBUG_SCOPE_PE9_GPIO_PORT    GPIOE
#define UART5_LOG_TX_PE10_PIN    GPIO_PINS_10
#define UART5_LOG_TX_PE10_GPIO_PORT    GPIOE
#define UART5_LOG_RX_PE11_PIN    GPIO_PINS_11
#define UART5_LOG_RX_PE11_GPIO_PORT    GPIOE
#define LED1_REMOTE_PE12_PIN    GPIO_PINS_12
#define LED1_REMOTE_PE12_GPIO_PORT    GPIOE
#define LED2_UPDATA_PE13_PIN    GPIO_PINS_13
#define LED2_UPDATA_PE13_GPIO_PORT    GPIOE
#define LED3_BRCAT_PE14_PIN    GPIO_PINS_14
#define LED3_BRCAT_PE14_GPIO_PORT    GPIOE
#define LED4_RUN_PE15_PIN    GPIO_PINS_15
#define LED4_RUN_PE15_GPIO_PORT    GPIOE
#define TTS_TX_PB10_PIN    GPIO_PINS_10
#define TTS_TX_PB10_GPIO_PORT    GPIOB
#define TTS_RX_PB11_PIN    GPIO_PINS_11
#define TTS_RX_PB11_GPIO_PORT    GPIOB
#define SPI3GT_SCK_PB12_PIN    GPIO_PINS_12
#define SPI3GT_SCK_PB12_GPIO_PORT    GPIOB
#define USB_VBUS_PB13_PIN    GPIO_PINS_13
#define USB_VBUS_PB13_GPIO_PORT    GPIOB
#define USB_DN_PB14_PIN    GPIO_PINS_14
#define USB_DN_PB14_GPIO_PORT    GPIOB
#define USB_DP_PB15_PIN    GPIO_PINS_15
#define USB_DP_PB15_GPIO_PORT    GPIOB
#define HUB_R1_PD8_PIN    GPIO_PINS_8
#define HUB_R1_PD8_GPIO_PORT    GPIOD
#define HUB_G1_PD9_PIN    GPIO_PINS_9
#define HUB_G1_PD9_GPIO_PORT    GPIOD
#define HUB_B1_PD10_PIN    GPIO_PINS_10
#define HUB_B1_PD10_GPIO_PORT    GPIOD
#define HUB_R2_PD11_PIN    GPIO_PINS_11
#define HUB_R2_PD11_GPIO_PORT    GPIOD
#define HUB_G2_PD12_PIN    GPIO_PINS_12
#define HUB_G2_PD12_GPIO_PORT    GPIOD
#define HUB_B2_PD13_PIN    GPIO_PINS_13
#define HUB_B2_PD13_GPIO_PORT    GPIOD
#define HUB_SCK_PD15_PIN    GPIO_PINS_15
#define HUB_SCK_PD15_GPIO_PORT    GPIOD
#define LED_OUT_B_PC7_PIN    GPIO_PINS_7
#define LED_OUT_B_PC7_GPIO_PORT    GPIOC
#define LED_OUT_R_PC8_PIN    GPIO_PINS_8
#define LED_OUT_R_PC8_GPIO_PORT    GPIOC
#define LED_POWER_CS_PC9_PIN    GPIO_PINS_9
#define LED_POWER_CS_PC9_GPIO_PORT    GPIOC
#define USART2_TX_PA8_PIN    GPIO_PINS_8
#define USART2_TX_PA8_GPIO_PORT    GPIOA
#define UART1485_RX_PA9_PIN    GPIO_PINS_9
#define UART1485_RX_PA9_GPIO_PORT    GPIOA
#define UART1485_RX_PA10_PIN    GPIO_PINS_10
#define UART1485_RX_PA10_GPIO_PORT    GPIOA
#define E27_POWEREN_PA11_PIN    GPIO_PINS_11
#define E27_POWEREN_PA11_GPIO_PORT    GPIOA
#define RS485_RXEN_PA12_PIN    GPIO_PINS_12
#define RS485_RXEN_PA12_GPIO_PORT    GPIOA
#define EEPROM_SCL_PH2_PIN    GPIO_PINS_2
#define EEPROM_SCL_PH2_GPIO_PORT    GPIOH
#define SPI3GT_CS_PA15_PIN    GPIO_PINS_15
#define SPI3GT_CS_PA15_GPIO_PORT    GPIOA
#define PIR_IN_PC10_PIN    GPIO_PINS_10
#define PIR_IN_PC10_GPIO_PORT    GPIOC
#define QSPI1_WS_PC11_PIN    GPIO_PINS_11
#define QSPI1_WS_PC11_GPIO_PORT    GPIOC
#define EEPROM_SDA_PC12_PIN    GPIO_PINS_12
#define EEPROM_SDA_PC12_GPIO_PORT    GPIOC
#define HUB_A_PD0_PIN    GPIO_PINS_0
#define HUB_A_PD0_GPIO_PORT    GPIOD
#define HUB_B_PD1_PIN    GPIO_PINS_1
#define HUB_B_PD1_GPIO_PORT    GPIOD
#define HUB_C_PD2_PIN    GPIO_PINS_2
#define HUB_C_PD2_GPIO_PORT    GPIOD
#define HUB_D_PD3_PIN    GPIO_PINS_3
#define HUB_D_PD3_GPIO_PORT    GPIOD
#define HUB_E_PD4_PIN    GPIO_PINS_4
#define HUB_E_PD4_GPIO_PORT    GPIOD
#define HUB_LAT_PD5_PIN    GPIO_PINS_5
#define HUB_LAT_PD5_GPIO_PORT    GPIOD
#define HUB_OE245_PD6_PIN    GPIO_PINS_6
#define HUB_OE245_PD6_GPIO_PORT    GPIOD
#define ALARM_IN_PD7_PIN    GPIO_PINS_7
#define ALARM_IN_PD7_GPIO_PORT    GPIOD
#define IIS_CLK_PB3_PIN    GPIO_PINS_3
#define IIS_CLK_PB3_GPIO_PORT    GPIOB
#define SPI3GT_MI_PB4_PIN    GPIO_PINS_4
#define SPI3GT_MI_PB4_GPIO_PORT    GPIOB
#define IIS_SD_PB5_PIN    GPIO_PINS_5
#define IIS_SD_PB5_GPIO_PORT    GPIOB
#define IIS_MCK_PB6_PIN    GPIO_PINS_6
#define IIS_MCK_PB6_GPIO_PORT    GPIOB
#define ALARM_OUT_PB7_PIN    GPIO_PINS_7
#define ALARM_OUT_PB7_GPIO_PORT    GPIOB
#define TTS_R_B_PB8_PIN    GPIO_PINS_8
#define TTS_R_B_PB8_GPIO_PORT    GPIOB
#define TTS_RST_PB9_PIN    GPIO_PINS_9
#define TTS_RST_PB9_GPIO_PORT    GPIOB
#define KEY1_PE0_PIN    GPIO_PINS_0
#define KEY1_PE0_GPIO_PORT    GPIOE
#define KEY2_PE1_PIN    GPIO_PINS_1
#define KEY2_PE1_GPIO_PORT    GPIOE

/* exported functions ------------------------------------------------------- */
  /* system clock config. */
  void wk_system_clock_config(void);

  /* config periph clock. */
  void wk_periph_clock_config(void);

  /* nvic config. */
  void wk_nvic_config(void);

/* add user code begin exported functions */

/* add user code end exported functions */

#ifdef __cplusplus
}
#endif

#endif
