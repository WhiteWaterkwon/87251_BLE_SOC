/*********************************************************************************************************//**
 * @file    bc5602_host.h
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
 * @brief   The header file of the BC5602B HOST functions.
 *************************************************************************************************************
 * @attention
 *
 * Firmware Disclaimer Information
 *
 * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
 *    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
 *    other intellectual property laws.
 *
 * 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
 *    other than HOLTEK and the customer.
 *
 * 3. The program technical documentation, including the code, is provided "as is" and for customer reference
 *    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
 *    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
 *    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
 *
 * <h2><center>Copyright (C) 2021 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __BC5602_IRQ1_ISR_H
#define __BC5602_IRQ1_ISR_H


/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdbool.h>                         //bool
#include <stdint.h>                          //uint8_t
#include "leacl.h"          //LEACL_TypeDef

/* Exported constants --------------------------------------------------------------------------------------*/



/* Exported types ------------------------------------------------------------------------------------------*/


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/
void irq1isr_open_TRT_advertising_event  (LEACL_TypeDef *, BLETIMER_TypeDef anchor, uint8_t type);
void irq1isr_open_RTR_scanning_event     (LEACL_TypeDef *, BLETIMER_TypeDef anchor);
void irq1isr_open_slave_connection_event (LEACL_TypeDef *, BLETIMER_TypeDef anchor);
void irq1isr_open_master_connection_event(LEACL_TypeDef *, BLETIMER_TypeDef anchor);
void irq1isr_open_DTM_advertising_event  (LEACL_TypeDef *, BLETIMER_TypeDef anchor);
void irq1isr_open_EXT_advertising_event  (LEACL_TypeDef *, BLETIMER_TypeDef anchor,  uint8_t type);
void irq1isr_open_DTM_scanning_event(LEACL_TypeDef *);

bool irq1isr_TRT_advertising_event_is_closed(void);
bool irq1isr_RTR_scanning_event_is_closed(void);
bool irq1isr_slave_connection_event_is_closed(void);
bool irq1isr_master_connection_event_is_closed(void);
bool irq1isr_DTM_TX_event_is_closed(void);
bool irq1isr_DTM_RX_event_is_closed(void);
bool irq1isr_DTM_RX_RXDR_CTE_event_is_closed(void);
bool irq1isr_EXT_TX_event_is_closed(void);

void debug_rssi_clear_count(void);

#endif /* __BC5602_IRQ1_ISR_H */
