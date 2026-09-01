/* add user code begin Header */
/**
  **************************************************************************
  * @file     at32f435_437_int.c
  * @brief    main interrupt service routines.
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

/* includes ------------------------------------------------------------------*/
#include "at32f435_437_int.h"
#include "wk_system.h"
#include "freertos_app.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "board_resources.h"
#include "plat_i2s.h"

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

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

/* add user code end 0 */

/* external variables ---------------------------------------------------------*/
/* add user code begin external variables */

/* add user code end external variables */

/**
  * @brief  this function handles nmi exception.
  * @param  none
  * @retval none
  */
void NMI_Handler(void)
{
  /* add user code begin NonMaskableInt_IRQ 0 */

  /* add user code end NonMaskableInt_IRQ 0 */

  /* add user code begin NonMaskableInt_IRQ 1 */

  /* add user code end NonMaskableInt_IRQ 1 */
}

/**
  * @brief  this function handles hard fault exception.
  * @param  none
  * @retval none
  */
void HardFault_Handler(void)
{
  /* add user code begin HardFault_IRQ 0 */

  /* add user code end HardFault_IRQ 0 */
  /* go to infinite loop when hard fault exception occurs */
  while (1)
  {
    /* add user code begin W1_HardFault_IRQ 0 */

    /* add user code end W1_HardFault_IRQ 0 */
  }
}


/**
  * @brief  this function handles memory manage exception.
  * @param  none
  * @retval none
  */
void MemManage_Handler(void)
{
  /* add user code begin MemoryManagement_IRQ 0 */

  /* add user code end MemoryManagement_IRQ 0 */
  /* go to infinite loop when memory manage exception occurs */
  while (1)
  {
    /* add user code begin W1_MemoryManagement_IRQ 0 */

    /* add user code end W1_MemoryManagement_IRQ 0 */
  }
}

/**
  * @brief  this function handles bus fault exception.
  * @param  none
  * @retval none
  */
void BusFault_Handler(void)
{
  /* add user code begin BusFault_IRQ 0 */

  /* add user code end BusFault_IRQ 0 */
  /* go to infinite loop when bus fault exception occurs */
  while (1)
  {
    /* add user code begin W1_BusFault_IRQ 0 */

    /* add user code end W1_BusFault_IRQ 0 */
  }
}

/**
  * @brief  this function handles usage fault exception.
  * @param  none
  * @retval none
  */
void UsageFault_Handler(void)
{
  /* add user code begin UsageFault_IRQ 0 */

  /* add user code end UsageFault_IRQ 0 */
  /* go to infinite loop when usage fault exception occurs */
  while (1)
  {
    /* add user code begin W1_UsageFault_IRQ 0 */

    /* add user code end W1_UsageFault_IRQ 0 */
  }
}

/**
  * @brief  this function handles debug monitor exception.
  * @param  none
  * @retval none
  */
void DebugMon_Handler(void)
{
  /* add user code begin DebugMonitor_IRQ 0 */

  /* add user code end DebugMonitor_IRQ 0 */
  /* add user code begin DebugMonitor_IRQ 1 */

  /* add user code end DebugMonitor_IRQ 1 */
}

extern void xPortSysTickHandler(void);

/**
  * @brief  this function handles systick handler.
  * @param  none
  * @retval none
  */
void SysTick_Handler(void)
{
  /* add user code begin SysTick_IRQ 0 */

  /* add user code end SysTick_IRQ 0 */

#if (INCLUDE_xTaskGetSchedulerState == 1 )
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
  xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1 )
  }
#endif /* INCLUDE_xTaskGetSchedulerState */

  /* add user code begin SysTick_IRQ 1 */

  /* add user code end SysTick_IRQ 1 */
}

/**
  * @brief  this function handles EDMA Stream 1 handler.
  * @param  none
  * @retval none
  */
void EDMA_Stream1_IRQHandler(void)
{
  /* add user code begin EDMA_Stream1_IRQ 0 */
  plat_i2s_tx_dma_irq_handler(BOARD_I2S_AUDIO_OUTPUT);

  /* add user code end EDMA_Stream1_IRQ 0 */

  if(edma_interrupt_flag_get(EDMA_FDT1_FLAG) != RESET)
  {   
    /* add user code begin EDMA_FDT1_FLAG */
    /* handle full data transfer and clear flag */
    edma_flag_clear(EDMA_FDT1_FLAG);
    /* add user code end EDMA_FDT1_FLAG */ 
  }

  if(edma_interrupt_flag_get(EDMA_HDT1_FLAG) != RESET)
  {   
    /* add user code begin EDMA_HDT1_FLAG */
    /* handle half data transfer and clear flag */
    edma_flag_clear(EDMA_HDT1_FLAG);
    /* add user code end EDMA_HDT1_FLAG */ 
  }

  if(edma_interrupt_flag_get(EDMA_DTERR1_FLAG) != RESET)
  {   
    /* add user code begin EDMA_DTERR1_FLAG */
    /* handle transfer error and clear flag */
    edma_flag_clear(EDMA_DTERR1_FLAG);
    /* add user code end EDMA_DTERR1_FLAG */ 
  }

  /* add user code begin EDMA_Stream1_IRQ 1 */

  /* add user code end EDMA_Stream1_IRQ 1 */
}

/**
  * @brief  this function handles USART1 handler.
  * @param  none
  * @retval none
  */
void USART1_IRQHandler(void)
{
  /* add user code begin USART1_IRQ 0 */

  /* add user code end USART1_IRQ 0 */

  if(usart_interrupt_flag_get(USART1, USART_IDLEF_FLAG) != RESET)
  {
    /* add user code begin USART1_USART_IDLEF_FLAG */
    /* clear flag */
    usart_flag_clear(USART1, USART_IDLEF_FLAG);
    /* add user code end USART1_USART_IDLEF_FLAG */ 
  }

  /* add user code begin USART1_IRQ 1 */

  /* add user code end USART1_IRQ 1 */
}

/**
  * @brief  this function handles USART2 handler.
  * @param  none
  * @retval none
  */
void USART2_IRQHandler(void)
{
  /* add user code begin USART2_IRQ 0 */

  /* add user code end USART2_IRQ 0 */

  if(usart_interrupt_flag_get(USART2, USART_IDLEF_FLAG) != RESET)
  {
    /* add user code begin USART2_USART_IDLEF_FLAG */
    /* clear flag */
    usart_flag_clear(USART2, USART_IDLEF_FLAG);
    /* add user code end USART2_USART_IDLEF_FLAG */ 
  }

  /* add user code begin USART2_IRQ 1 */

  /* add user code end USART2_IRQ 1 */
}

/**
  * @brief  this function handles USART3 handler.
  * @param  none
  * @retval none
  */
void USART3_IRQHandler(void)
{
  /* add user code begin USART3_IRQ 0 */

  /* add user code end USART3_IRQ 0 */

  if(usart_interrupt_flag_get(USART3, USART_IDLEF_FLAG) != RESET)
  {
    /* add user code begin USART3_USART_IDLEF_FLAG */
    /* clear flag */
    usart_flag_clear(USART3, USART_IDLEF_FLAG);
    /* add user code end USART3_USART_IDLEF_FLAG */ 
  }

  /* add user code begin USART3_IRQ 1 */

  /* add user code end USART3_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 1 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* add user code begin DMA1_Channel1_IRQ 0 */

  /* add user code end DMA1_Channel1_IRQ 0 */

  if(dma_interrupt_flag_get(DMA1_FDT1_FLAG) != RESET)
  {   
    /* add user code begin DMA1_FDT1_FLAG */
    /* handle full data transfer and clear flag */
    dma_flag_clear(DMA1_FDT1_FLAG);
    /* add user code end DMA1_FDT1_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_HDT1_FLAG) != RESET)
  {   
    /* add user code begin DMA1_HDT1_FLAG */
    /* handle half data transfer and clear flag */
    dma_flag_clear(DMA1_HDT1_FLAG);
    /* add user code end DMA1_HDT1_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_DTERR1_FLAG) != RESET)
  {   
    /* add user code begin DMA1_DTERR1_FLAG */
    /* handle error transfer and clear flag */
    dma_flag_clear(DMA1_DTERR1_FLAG);
    /* add user code end DMA1_DTERR1_FLAG */ 
  }

  /* add user code begin DMA1_Channel1_IRQ 1 */

  /* add user code end DMA1_Channel1_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 2 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel2_IRQHandler(void)
{
  /* add user code begin DMA1_Channel2_IRQ 0 */

  /* add user code end DMA1_Channel2_IRQ 0 */

  if(dma_interrupt_flag_get(DMA1_FDT2_FLAG) != RESET)
  {   
    /* add user code begin DMA1_FDT2_FLAG */
    /* handle full data transfer and clear flag */
    dma_flag_clear(DMA1_FDT2_FLAG);
    /* add user code end DMA1_FDT2_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_HDT2_FLAG) != RESET)
  {   
    /* add user code begin DMA1_HDT2_FLAG */
    /* handle half data transfer and clear flag */
    dma_flag_clear(DMA1_HDT2_FLAG);
    /* add user code end DMA1_HDT2_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_DTERR2_FLAG) != RESET)
  {   
    /* add user code begin DMA1_DTERR2_FLAG */
    /* handle error transfer and clear flag */
    dma_flag_clear(DMA1_DTERR2_FLAG);
    /* add user code end DMA1_DTERR2_FLAG */ 
  }

  /* add user code begin DMA1_Channel2_IRQ 1 */

  /* add user code end DMA1_Channel2_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 3 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* add user code begin DMA1_Channel3_IRQ 0 */

  /* add user code end DMA1_Channel3_IRQ 0 */

  if(dma_interrupt_flag_get(DMA1_FDT3_FLAG) != RESET)
  {   
    /* add user code begin DMA1_FDT3_FLAG */
    /* handle full data transfer and clear flag */
    dma_flag_clear(DMA1_FDT3_FLAG);
    /* add user code end DMA1_FDT3_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_HDT3_FLAG) != RESET)
  {   
    /* add user code begin DMA1_HDT3_FLAG */
    /* handle half data transfer and clear flag */
    dma_flag_clear(DMA1_HDT3_FLAG);
    /* add user code end DMA1_HDT3_FLAG */ 
  }

  if(dma_interrupt_flag_get(DMA1_DTERR3_FLAG) != RESET)
  {   
    /* add user code begin DMA1_DTERR3_FLAG */
    /* handle error transfer and clear flag */
    dma_flag_clear(DMA1_DTERR3_FLAG);
    /* add user code end DMA1_DTERR3_FLAG */ 
  }

  /* add user code begin DMA1_Channel3_IRQ 1 */

  /* add user code end DMA1_Channel3_IRQ 1 */
}

/**
  * @brief  this function handles TMR20 overflow handler.
  * @param  none
  * @retval none
  */
void TMR20_OVF_IRQHandler(void)
{
  /* add user code begin TMR20_OVF_IRQ 0 */

  /* add user code end TMR20_OVF_IRQ 0 */

  wk_timebase_handler();

  /* add user code begin TMR20_OVF_IRQ 1 */

  /* add user code end TMR20_OVF_IRQ 1 */
}

/* add user code begin 1 */

/* add user code end 1 */
