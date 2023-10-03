/*********************************************************************************************************//**
 * @file    hc.h
 * @version $Rev:: 101          $
 * @date    $Date:: 2016-12-30 #$
 * @brief   The header file of the BC7161 HOST functions.
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
 * <h2><center>Copyright (C) 2016 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------------------------------------*/

struct ChMtable{
	int Chm_numbers;
	uint8_t Chm_indextable[37];
	
};

/* Exported constants --------------------------------------------------------------------------------------*/



/* Exported functions --------------------------------------------------------------------------------------*/
struct ChMtable ChMtoChanneltable(uint64_t ChM);
int Min(int a,int b);
int Max(int a,int b);
//uint8_t *ChannelSelectionAlgorithm2 (uint32_t AccessAddress,uint16_t Counter, uint8_t *ChM);
void ChannelSelectionAlgorithm2 (LEACL_TypeDef *pacl);
uint8_t SecondaryAdvChannelSelectionRandom (uint8_t *ChM);
extern uint8_t indextable[37];
extern uint8_t mappedChannelindex[4];
extern uint8_t ChannelindexTable[40];

