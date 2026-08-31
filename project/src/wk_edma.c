/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_edma.c
  * @brief    work bench config program
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

/* Includes ------------------------------------------------------------------*/
#include "wk_edma.h"

/* add user code begin 0 */

/* add user code end 0 */

/**
  * @brief  init edma stream1 for "spi1_tx"
  * @param  none
  * @retval none
  */
void wk_edma_stream1_init(void)
{
  /* add user code begin edma_stream1 0 */

  /* add user code end edma_stream1 0 */

  edma_init_type edma_init_struct;

  /* edma stream1 configuration */
  edma_default_para_init(&edma_init_struct);
  edma_init_struct.direction = EDMA_DIR_MEMORY_TO_PERIPHERAL;
  edma_init_struct.peripheral_data_width = EDMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
  edma_init_struct.peripheral_inc_enable = FALSE;
  edma_init_struct.memory_data_width = EDMA_MEMORY_DATA_WIDTH_HALFWORD;
  edma_init_struct.memory_inc_enable = TRUE;
  edma_init_struct.fifo_mode_enable = FALSE;
  edma_init_struct.fifo_threshold = EDMA_FIFO_THRESHOLD_FULL;
  edma_init_struct.peripheral_burst_mode = EDMA_PERIPHERAL_SINGLE;
  edma_init_struct.memory_burst_mode = EDMA_MEMORY_SINGLE;
  edma_init_struct.priority = EDMA_PRIORITY_LOW;
  edma_init_struct.loop_mode_enable = TRUE;
  edma_init(EDMA_STREAM1, &edma_init_struct);

  /* edmamux init */
  edmamux_enable(TRUE);
  edmamux_init(EDMAMUX_CHANNEL1, EDMAMUX_DMAREQ_ID_SPI1_TX);

  /* enable edma stream1 full data transfer interrupt */
  edma_interrupt_enable(EDMA_STREAM1, EDMA_FDT_INT, TRUE);

  /* enable edma stream1 half data transfer interrupt */
  edma_interrupt_enable(EDMA_STREAM1, EDMA_HDT_INT, TRUE);

  /* enable edma stream1 transfer error interrupt */
  edma_interrupt_enable(EDMA_STREAM1, EDMA_DTERR_INT, TRUE);

  /* add user code begin edma_stream1 1 */

  /* add user code end edma_stream1 1 */
}

/**
  * @brief  config edma stream transfer parameter
  * @param  edma_streamx: EDMA_STREAMx
  * @param  peripheral_base_addr: peripheral address.
  * @param  memory0_base_addr: memory address.
  * @param  buffer_size: buffer size.
  * @retval none
  */
void wk_edma_stream_config(edma_stream_type* edma_streamx, uint32_t peripheral_base_addr, uint32_t memory0_base_addr, uint16_t buffer_size)
{
  /* add user code begin edma_stream_config 0 */

  /* add user code end edma_stream_config 0 */

  edma_streamx->dtcnt = buffer_size;
  edma_streamx->paddr = peripheral_base_addr;
  edma_streamx->m0addr = memory0_base_addr;

  /* add user code begin edma_stream_config 1 */

  /* add user code end edma_stream_config 1 */
}

/* add user code begin 1 */

/* add user code end 1 */
