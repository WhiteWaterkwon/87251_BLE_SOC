/*********************************************************************************************************//**
 * @file    csa.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2018-10-26 #$
 * @brief   This file provides all BC5602 HOST functions.
 *************************************************************************************************************
 * @attention
 *
 * Firmware Disclaimer Information
 *
 * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, which is supplied by Holtek Semiconductor Inc., is the
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

/* Includes ------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcpy,memset   ==>   ht_memory_copy    ht_memory_set
#include <stdbool.h>                         //bool
#include <math.h>
#include "hwlib/socal/socal.h"
#include "bc5602b_host.h"                    // delay_unit625us
#include "ble_soc.h"                         // RAM_WT32
#include "usart.h"                           // uart_puts, uart_putu8
#include "bc5602b_irq1_ISR.h"
#include "hc.h"
#include "csa.h"



/* Private variables ---------------------------------------------------------------------------------------*/
#define MatchingTRUE 1
#define MatchingFALSE 0

struct MatchingandIndex{
	int MatchingTF;
	int index;
};
/* Global functions ----------------------------------------------------------------------------------------*/

uint8_t indextable[37]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x0018,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24}; 
uint8_t mappedChannelindex[4]={0x00,0x00,0x00,0x00};
uint8_t ChannelindexTable[40]={4,6,8,10,12,14,16,18,20,22,24,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,2,26,80};
/////////////////////////////////////////////////////////////////////////////////////////////

/********************************************************************************/

int floor_func(float num) {
    int floor_num = (int) num; // 盢疊翴计锣传俱计
    
    if (num < 0 && num != floor_num) {
        floor_num -= 1; // 狦璽计ぃ俱计玥俱
    }
    
    return floor_num;
}

int Min(int a,int b)
{
	if (a<b)	return a;
	else   		return b;
}

int Max(int a,int b)
{
	if (a>b)  return a;
	else 			return b;
}


uint16_t MAM(uint16_t a, uint16_t b)
{
	uint16_t output;


	output = ((a*17)+b) & 0xFFFF;

	return output;
}

uint16_t PERM(uint16_t input)
{
	input = ((0xF0F0&input)>>4)|((0x0F0F&input)<<4);
	input = ((0xCCCC&input)>>2)|((0x3333&input)<<2);
	input = ((0xAAAA&input)>>1)|((0x5555&input)<<1);
	return input;
}

struct ChMtable ChMtoChanneltable(uint64_t ChM)
{
	struct ChMtable a;
	uint64_t mask;
	int i;
	int n=0;
	int j=0;
	mask = 0x0000000001;
	for(i=0;i<37;i++)
	{
		if(ChM&mask)
		{
			a.Chm_indextable[j]=indextable[i];
			j++; n++;
		}

			mask = (mask<<1);
	}
	a.Chm_numbers=n;
	return a;
}

struct MatchingandIndex MatchingChannelMap(uint8_t unmappedchannel,struct ChMtable UsedChM)
{
	struct MatchingandIndex a;
	int i;
			a.MatchingTF =255;
			a.index =255;	
	for(i=0;i<UsedChM.Chm_numbers;i++)
	{
		if(unmappedchannel== UsedChM.Chm_indextable[i])
		{
			a.MatchingTF =1;
			a.index =i;
			
		}
			//uart_puts("a.MatchingTF = ");uart_putu8(a.MatchingTF);uart_putchar_n('\n');
			//uart_puts("a.index  = ");uart_putu8(a.index);uart_putchar_n('\n');
	}
	return a;
}

uint16_t SubevenPseudoNum_generation_lu(uint16_t lastUsedprn,uint16_t channelidentifier)
{
	uint16_t prnSubEvent_lu;	

	prnSubEvent_lu = MAM(PERM(lastUsedprn),channelidentifier);
	
	return prnSubEvent_lu;
}

uint8_t SubevenMappingChannelindex(uint16_t prnSubEvent_se, int d ,uint8_t indexOfLastUsedChannel,int N)
{
	uint8_t a;
	
	a= (indexOfLastUsedChannel+d+(uint8_t)floor_func(((int)prnSubEvent_se*(N-(2*d)+1))/65536)) %N;
	
	return a;
}
void ChannelSelectionAlgorithm2 (LEACL_TypeDef *pacl)
{
	int d;
	struct ChMtable UsedChM;
	int indexofunmappedchannel;
	uint16_t channelidentifier,prn_s,prn_e,unmappedchannel,prnSubEvent_lu,prnSubEvent_se;
	uint64_t ChM_u64;
	
	channelidentifier = ((pacl->AA&0xFFFF0000) >> 16)^(pacl->AA&0x0000FFFF);
	
	ChM_u64 =pacl->currChM.chM[4]&0x1F;
	ChM_u64 = (ChM_u64 <<8)|pacl->currChM.chM[3];
	ChM_u64 = (ChM_u64 <<8)|pacl->currChM.chM[2];
	ChM_u64 = (ChM_u64 <<8)|pacl->currChM.chM[1];
	ChM_u64 = (ChM_u64 <<8)|pacl->currChM.chM[0];	
	UsedChM = ChMtoChanneltable(ChM_u64);
	d = Max(1,Max(Min(11,((UsedChM.Chm_numbers-10)/2)),Min(3,UsedChM.Chm_numbers-5)));
	
	//Event pseudo-random number generation
	prn_s = MAM(PERM(MAM(PERM(MAM(PERM(pacl->periodicEventCount ^ channelidentifier),channelidentifier)),channelidentifier)),channelidentifier);
	prn_e = prn_s ^ channelidentifier;
	//uart_puts("prn_e= ");uart_putu16(prn_e);uart_puts(" ; ");
	
	//Unmapped channel selection process
	unmappedchannel = prn_e %37;

	//Event mapping to used channel index
	if(MatchingChannelMap(unmappedchannel,UsedChM).MatchingTF == MatchingTRUE)
	{
		mappedChannelindex[0] = unmappedchannel;
		pacl->mappedChannelIndex[0] = mappedChannelindex[0];
		indexofunmappedchannel = MatchingChannelMap(mappedChannelindex[0],UsedChM).index;
	}
	else
	{
		mappedChannelindex[0] = UsedChM.Chm_indextable[(int)(floor_func((prn_e*UsedChM.Chm_numbers)/65536))];
		indexofunmappedchannel = MatchingChannelMap(mappedChannelindex[0],UsedChM).index;
	}
	

	//Subevent pseudo-random number generation#1
	prnSubEvent_lu = SubevenPseudoNum_generation_lu(prn_s,channelidentifier);
	prnSubEvent_se = prnSubEvent_lu ^ channelidentifier;
	//uart_puts("prnSubEvent_se#1= ");uart_putu16(prnSubEvent_se);uart_puts(" ; ");
	// Subevent mapping to used channel index#1
	mappedChannelindex[1] = UsedChM.Chm_indextable[SubevenMappingChannelindex(prnSubEvent_se,d,indexofunmappedchannel,UsedChM.Chm_numbers)];
	pacl->mappedChannelIndex[1] = mappedChannelindex[1];	
	indexofunmappedchannel = MatchingChannelMap(mappedChannelindex[1],UsedChM).index;
	
	//Subevent pseudo-random number generation#2
	prnSubEvent_lu = SubevenPseudoNum_generation_lu(prnSubEvent_lu,channelidentifier);
	prnSubEvent_se = prnSubEvent_lu ^ channelidentifier;
	//uart_puts("prnSubEvent_se#2= ");uart_putu16(prnSubEvent_se);uart_puts(" ; ");
	//Subevent mapping to used channel index#2
	mappedChannelindex[2] = UsedChM.Chm_indextable[SubevenMappingChannelindex(prnSubEvent_se,d,indexofunmappedchannel,UsedChM.Chm_numbers)];
	pacl->mappedChannelIndex[2] = mappedChannelindex[2];	
	indexofunmappedchannel = MatchingChannelMap(mappedChannelindex[2],UsedChM).index;
	
	//Subevent pseudo-random number generation#3
	prnSubEvent_lu = SubevenPseudoNum_generation_lu(prnSubEvent_lu,channelidentifier);
	prnSubEvent_se = prnSubEvent_lu ^ channelidentifier;
	//uart_puts("prnSubEvent_se#3= ");uart_putu16(prnSubEvent_se);uart_puts(" ; ");
	// Subevent mapping to used channel index#3
	mappedChannelindex[3] = UsedChM.Chm_indextable[SubevenMappingChannelindex(prnSubEvent_se,d,indexofunmappedchannel,UsedChM.Chm_numbers)];
	pacl->mappedChannelIndex[3] = mappedChannelindex[3];
	//uart_puts("mappedChannelindex1= ");uart_putu8(mappedChannelindex[0]);uart_puts(" ; ");
	//uart_puts("mappedChannelindex2= ");uart_putu8(mappedChannelindex[1]);uart_puts(" ; ");uart_putchar_n('\n');
	//uart_puts("mappedChannelindex3= ");uart_putu8(mappedChannelindex[2]);uart_puts(" ; ");uart_putchar_n('\n');
	//uart_puts("mappedChannelindex4= ");uart_putu8(mappedChannelindex[3]);uart_puts(" ; ");uart_putchar_n('\n');
	//return mappedChannelindex;
}

uint8_t SecondaryAdvChannelSelectionRandom (uint8_t *ChM)
{
	uint8_t channelIndex;
	struct ChMtable UsedChM;
	uint16_t random_e,unmappedchannel;
	uint64_t ChM_u64;
	
	ChM_u64 =ChM[4]&0x1F;
	ChM_u64 = (ChM_u64 <<8)|ChM[3];
	ChM_u64 = (ChM_u64 <<8)|ChM[2];
	ChM_u64 = (ChM_u64 <<8)|ChM[1];
	ChM_u64 = (ChM_u64 <<8)|ChM[0];	
	
	random_e = rand();
	UsedChM = ChMtoChanneltable(ChM_u64);
	unmappedchannel = random_e %37;
	if(MatchingChannelMap(unmappedchannel,UsedChM).MatchingTF == MatchingTRUE)
	{
		channelIndex = unmappedchannel;
	}
	else
	{
		channelIndex = UsedChM.Chm_indextable[(int)(floor_func((random_e*UsedChM.Chm_numbers)/65536))];
	}	
	return channelIndex;
}