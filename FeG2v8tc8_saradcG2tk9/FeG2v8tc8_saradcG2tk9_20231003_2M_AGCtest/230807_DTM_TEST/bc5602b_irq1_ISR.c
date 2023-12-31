/*********************************************************************************************************//**
 * @file    bc5602_irq1_ISR.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
 * @brief   This file provides all BC5602 IRQ1 ISR functions.
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
 * <h2><center>Copyright (C) 2021 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcpy,memset   ==>   ht_memory_copy    ht_memory_set
//#include <stdbool.h>                         //bool

#include "hwlib/socal/socal.h"

#include "bc5602b_host.h"
#include "reg87251.h"
#include "ble_soc.h"
#include "strobe_cmd.h"
#include "pdma.h"                            //pdma_ram_bleTx_write_payload()
#include "usart.h"                           // uart_puts, uart_putu8
#include "bch_5602.h"
#include "htqueue.h"
#include "mac.h"                             //ADV_PDU_HDR_TypeDef
#include "advrxq.h"                          //irq1isr_read_rxfifo_to_advRxQ()
#include "leconfig.h"
#include "leacl.h"
#include "llc.h"
#include "lldata.h"
#include "hc.h"

//#include "FreeRTOS.h"
//#include "task.h"               //taskENTER_CRITICAL()
//#include "queue.h"

/* Public variables ----------------------------------------------------------------------------------------*/
ADV_PDU_HDR_S0_TypeDef txAdvHeaderS0;

/* Private variables ---------------------------------------------------------------------------------------*/
#define  BCCNT_0x10                        (((2+0)*8))      // end header
#define  BCCNT_ENDHEADER_AESCCMON          (((2+0)*8)+8)    //+8:skip aesccm rx pdma get SessionKey at BC_CNT==16

static          bool     first_roundtrip_in_a_ce;
static volatile uint8_t  irq1isr_TRT_event_kind; // 1:TRT event, 0:T1 only
static unsigned char iq_sample[164];


extern unsigned char debug_rssi_id_ok_note_max[110];
extern unsigned char debug_rssi_id_ok_note_min[110];
extern unsigned char debug_rssi_gainsel_note_max[110];
extern unsigned char debug_rssi_gainsel_note_min[110];
static unsigned char debug_rssi_id_ok[1500];
static unsigned char debug_rssi_gain_sel[1500];
int debug_rssi_count_i;


volatile uint8_t  change_channel_index = 37;
static uint16_t   compute_aux_offset;
const uint8_t RfCenterFreq[50] = { // index by channelIndex
     4, 6, 8,10,12,14,16,18,20,22,24,
    28,30,32,34,36,38,40,42,44,46,
    48,50,52,54,56,58,60,62,64,66,
    68,70,72,74,76,78,
     2,26,80,
    82,83,84,85,86,87,88,89,90,91                       // debug channel 2482~2490 MHz
};
const uint8_t WhiteningSeed[50] = { // index by channelIndex
    0x01,0x41,0x21,0x61,0x11,0x51,0x31,0x71,0x09,0x49,
    0x29,0x69,0x19,0x59,0x39,0x79,0x05,0x45,0x25,0x65,
    0x15,0x55,0x35,0x75,0x0D,0x4D,0x2D,0x6D,0x1D,0x5D,
    0x3D,0x7D,0x03,0x43,0x23,0x63,0x13,0x53,0x33,0x73,
                                       //37,  38,  39  channelIndex
                                       // 2402 MHz BLE channel index 37=0x25= 1 10 0101 ==> 0x53
                                       // 2426 MHz BLE channel index 38=0x26= 1 10 0110 ==> 0x33
                                       // 2480 MHz BLE channel index 39=0x27= 1 10 0111 ==> 0x73
    0x0B,0x4B,0x2B,0x6B,0x1B,0x5B,0x3B,0x7B,0x07,0x47   // debug channel 2482~2490 MHz
};

extern volatile uint32_t schedluezzzz;

typedef struct __attribute__((packed))
{
    uint8_t  length;
    uint8_t  ExtAdvHeader[255]; //0-63
    uint8_t  ExtAdvData[255]; //0-254
}      ext_adv_data_typedef ;
static ext_adv_data_typedef ext_adv_data;

#define LL_TRT_W4_IRQ_WAKEUP                    0x41      // TRT:Advertiser TRT event
#define LL_TRT_W4_IRQ_B_SET                     0x42      //
#define LL_TRT_W4_IRQ_ANCHOR                    0x43      //
#define LL_TRT_T1                               0x44      //
#define LL_TRT_RX                               0x45      //  R  connectind or scanreq
#define LL_TRT_T2_SCANRSP                       0x46      //
#define LL_TRT_CLOSE_EVENT                      0x47      //

#define LL_RTR_W4_IRQ_WAKEUP                    0x51      // RTR:Scanner RTR event
#define LL_RTR_W4_IRQ_B_SET                     0x52      //
#define LL_RTR_W4_IRQ_ANCHOR                    0x53      //
#define LL_RTR_R1                               0x54      //
#define LL_RTR_TX_CONNECTIND                    0x55      //
#define LL_RTR_TX_SCANREQ                       0x56      //
#define LL_RTR_R2_SCANRSP                       0x57      //
#define LL_RTR_CLOSE_EVENT                      0x58      //

#define LL_SLA_W4_IRQ_WAKEUP                    0x63      // SLA:Slave Connection Event
#define LL_SLA_W4_IRQ_B_SET                     0x64      //
#define LL_SLA_W4_IRQ_ANCHOR                    0x65      //
#define LL_SLA_RX_M2S                           0x66      //
#define LL_SLA_TX_S2M                           0x67      //
#define LL_SLA_CLOSE_EVENT                      0x68      //

#define LL_MAS_W4_IRQ_WAKEUP                    0x72      // MAS:Master Connection Event
#define LL_MAS_W4_IRQ_B_SET                     0x73      //
#define LL_MAS_W4_IRQ_ANCHOR                    0x74      //
#define LL_MAS_TX_M2S                           0x75      //
#define LL_MAS_RX_S2M                           0x76      //
#define LL_MAS_CLOSE_EVENT                      0x77      //

#define LL_DTM_TX_W4_IRQ_ANCHOR			0x80
#define LL_DTM_TX_ADVERTISING			0x81
#define LL_DTM_TX_CLOSE_EVENT			0x82

#define LL_DTM_RX_SCAN                          0x90
#define LL_DTM_RX_CLOSE_EVENT			0x91
#define LL_DTM_RX_RXDR_CTE_EVENT		0x92

#define LL_ext_adv_CLOSE_EVENT					0xA0
#define LL_ext_adv_ADV_EXT_IND_W4_IRQ_WAKEUP	     	        0xA1
#define LL_ext_adv_ADV_EXT_IND_W4_IRQ_B_SET	         	0xA2
#define LL_ext_adv_ADV_EXT_IND_W4_IRQ_ANCHOR	     	  	0xA3
#define LL_ext_adv_ADV_EXT_IND_W4_IRQ_TX_DS   		 	0xA4
#define LL_ext_adv_AUX_ADV_IND_W4_IRQ_WAKEUP	     	        0xA5
#define LL_ext_adv_AUX_ADV_IND_W4_IRQ_B_SET	         	0xA6
#define LL_ext_adv_AUX_ADV_IND_W4_IRQ_ANCHOR	     	  	0xA7
#define LL_ext_adv_AUX_ADV_IND_W4_IRQ_TX_DS		    	0xA8
#define LL_ext_adv_AUX_SYNC_IND_W4_IRQ_WAKEUP	     	        0xA9
#define LL_ext_adv_AUX_SYNC_IND_W4_IRQ_B_SET	         	0xAA
#define LL_ext_adv_AUX_SYNC_IND_W4_IRQ_ANCHOR	     	  	0xAB
#define LL_ext_adv_AUX_SYNC_IND_W4_IRQ_TX_DS		    	0xAC

typedef struct {
    uint32_t        state;
    LEACL_TypeDef  *pacl ;
} bc5602b_llfsm_holtek_TypeDef;
static volatile bc5602b_llfsm_holtek_TypeDef llfsm;

/* Global functions ----------------------------------------------------------------------------------------*/
void disable_A9_interrupts(void);
void enable_A9_interrupts(void);
void channel_selection_algorithm_1_leacl_currChannel(LEACL_TypeDef *);
void init_0rf_2in1_5602B_set_rfch(uint8_t ch); //5602B RF

/* Private functions ---------------------------------------------------------------------------------------*/
static inline void irq1isr_memory_copy(uint8_t *dst, uint8_t *src, uint32_t len)
{
    while (len) {
        ht_write_byte( dst, *src);
        dst ++; src ++;
        len --;
    }
}

static inline void debug_RFLA_pulse_to_trigger_LA(void)
{
    RF_WT08(0x100, 0x1);
}

static volatile _53h_max_rxpl_TypeDef written_53h_max_rxpl;
static inline void RF_WT08_53h_MAX_RXPL(uint8_t maxrxpl)
{
    written_53h_max_rxpl.reg = maxrxpl;
    RF_WT08(0x52, maxrxpl);
                            //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                            //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                            //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                            //        then   HW treat received rxdlen as MAX_RXPL
                            //                  finish receive payload MAX_RXPL bytes
                            //                  result IRQ[7]CRCF
}
static inline bool greater_than_upperlimit_rxHeaderLength(uint8_t rxdlen)
{
    _53h_max_rxpl_TypeDef r_53h;
    r_53h.reg = written_53h_max_rxpl.reg;
  //r_53h.reg = RF_RD08(0x53);
                            //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                            //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                            //if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                            //then   HW treat received rxdlen as MAX_RXPL
                            //          finish receive payload MAX_RXPL bytes
                            //          result IRQ[7]CRCF
                            //though result the same IRQ[7]CRCF, HW L2State is L2BRXNOK    0x1C <== RX bit error @ header[15:8]Length
                            //                                   HW  ignore [7]DISABLED_TXEN, [6]DISABLED_RXEN
                            // different from normal IRQ[7]CRCF, HW L2State is L2BRCRCFail 0x1D <== RX bit error @ payload or crc
                            //                                   HW execute [7]DISABLED_TXEN, [6]DISABLED_RXEN
    return(          r_53h.field.rxpl_en  &&
            rxdlen > r_53h.field.max_rxpl );
}
static inline uint16_t calc_bccntPayloadEnd_by_pacl_transmitLength(LEACL_TypeDef *pacl)
{
    uint16_t bccntPayloadEnd ;
    if (pacl->flag1.field.Tx_aesccm_enabled) {
                bccntPayloadEnd = pacl->transmitHdr.field.transmitLength ;
        /*
        switch( bccntPayloadEnd & 0x03 ) {
        case 0x00: 
                   if (bccntPayloadEnd == 0)
                       bccntPayloadEnd  = 0;
                   else // 4,8,12,...
                     //bccntPayloadEnd -= 2;
                       bccntPayloadEnd  = 2;
                   break;
        case 0x01: 
                   if (bccntPayloadEnd == 1)
                       bccntPayloadEnd  = 1;
                   else // 5,9,13,...
                     //bccntPayloadEnd -= 3;
                       bccntPayloadEnd  = 2;
                   break;
        case 0x02:   //bccntPayloadEnd &= 0xFE;
                       bccntPayloadEnd  = 2;
                   break;
        case 0x03:   //bccntPayloadEnd &= 0xFE;
                       bccntPayloadEnd  = 2;
                   break;
        }//end switch
        */
    }
    else {
             bccntPayloadEnd = pacl->transmitHdr.field.transmitLength;
    }
           //bccntPayloadEnd = (2 +  bccntPayloadEnd) * 8;  // 2: 2 bytes header
             bccntPayloadEnd = 16 + (bccntPayloadEnd << 3); // 2: 2 bytes header
    return ( bccntPayloadEnd );
}
static inline uint16_t calc_bccntPayloadEnd_by_rxdlen(uint8_t rxdlen)
{
    uint16_t bccntPayloadEnd ;
             bccntPayloadEnd = rxdlen;
       if(                     written_53h_max_rxpl.field.rxpl_en  &&
             bccntPayloadEnd > written_53h_max_rxpl.field.max_rxpl )
       {
             bccntPayloadEnd = written_53h_max_rxpl.field.max_rxpl ;
       }
           //bccntPayloadEnd = (2 +  bccntPayloadEnd) * 8;  // 2: 2 bytes header
             bccntPayloadEnd = 16 + (bccntPayloadEnd << 3); // 2: 2 bytes header
    return ( bccntPayloadEnd );
}

static inline void clear_34h_irq1(void)
{
    RF_WT08(0x34, 0xFF);        // IRQ1[7]CRCF     write 1 to clear bit
                                //     [6]RX_DR,   write 1 to clear bit
                                //     [5]TX_DS,   write 1 to clear bit
                                //     [4]MAX_RT,  write 1 to clear bit
                                //     [3]ADDRESS, write 1 to clear bit
                                //     [2]BCMATCH, write 1 to clear bit
                                //     [1]RX_TO,   write 1 to clear bit
                                //     [0]-
}
static inline void write_50h_CRCInit_555555(void)  // for advertising channel PDU
{
//  RF_WT16(0x50, 0x5554);//debug test
    RF_WT16(0x50, 0x5555);              //0x50 CRC4 CRC_INI[ 7: 0] CRC initial value
                                        //0x51 CRC5 CRC_INI[15: 8]
    RF_WT08(0x52, 0x55);                //0x52 CRC6 CRC_INI[23:16]
}
static inline void write_50h_CRCInit(uint8_t *pCrcini) // for connection channel PDU
{
    RF_WT08(0x4F, pCrcini[0]);          //0x50 CRC4 CRC_INI[ 7: 0] CRC initial value
    RF_WT08(0x50, pCrcini[1]);          //0x51 CRC5 CRC_INI[15: 8]
    RF_WT08(0x51, pCrcini[2]);          //0x52 CRC6 CRC_INI[23:16]
}
static inline void write_00h_AccessAddress(uint32_t aa)  // Access Address
{
    address5602_TypeDef id;
    id.byte4321 = aa>>8;
    id.byte0    = aa&0xFF;
    // set pipe 0 address
    strobe_write_pipe_0_ID(id); //<<<<<<<<<<<
}

static volatile _0Ch_bccnt_TypeDef written_BCCNT;
static inline void RF_WT16_0x0C(uint16_t bccnt)
{
    RF_WT16(0x0C, bccnt);                           //0Ch BC_CNT[7:0]
                                                    //0Dh BC_CNT[15:8]
    written_BCCNT.reg = bccnt;
}

static volatile _E0h_EN_B_TIMER_TypeDef written_EN_B_Timer;
static inline void RF_WT08_0xE0(uint8_t en_b_timer)
{
    RF_WT08(0xE0, en_b_timer);                      //E0h [0]  EN_WK_UP    Wake UP      IRQ enable
                                                    //E0h [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //E0h [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //E0h [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //E0h [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //E0h [6:5]-
                                                    //E0h [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
    written_EN_B_Timer.reg = en_b_timer;
}
static inline void disable_anchor_IRQ_if_still_enabled(void)
{
    if (written_EN_B_Timer.field.en_anchor)
    {
        RF_WT08_0xE0( E0h_10_EN_TEST_LO | 0x00);
    }
}

//////////////////////////////////////
void debug_rssi_clear_count(void)
{
	debug_rssi_count_i =0;
}
static unsigned char findmaxValue(unsigned char myArray[], int Array_size)
{
	int i;
	unsigned char maxValue= myArray[0];
	
	for(i=1;i<Array_size;++i)
	{
		if(myArray[i]>maxValue)
		{
			maxValue=myArray[i];
		}
		
	}
	return maxValue;
}

static unsigned char findminValue(unsigned char myArray[], int Array_size)
{
	int i;
	unsigned char minValue= 0xFF;
	
	for(i=1;i<Array_size;++i)
	{
		if(myArray[i]!=0x00)
		{
			if(myArray[i]<minValue)
			{
				minValue=myArray[i];
			}
		}
	}
	return minValue;
}

static void findmaxminValue(unsigned char myArray[], int Array_size, unsigned char *max ,unsigned char *min)
{
	int i;
	*max = myArray[1];
	*min = 0xFF;	
	for(i=1;i<Array_size;i++)
	{
		if(myArray[i]!=0x00)
		{
			if(myArray[i]< *min )
			{
				*min = myArray[i];
			}
			if(myArray[i]> *max )
			{
				*max = myArray[i];
			}
		}
	}
}

////////////////////////////////////////
static void err_print____(unsigned char irq1)
{
    debug_RFLA_pulse_to_trigger_LA(); //debug
    debug_RFLA_pulse_to_trigger_LA(); //debug
    debug_RFLA_pulse_to_trigger_LA(); //debug
    
    uart_puts("err_print__________________");
    uart_putu8(irq1);         uart_puts("_");
    uart_putu8(llfsm.state);//uart_puts("_");
    uart_putchar_n('\n');
}

////////////////////////////////////////


/*
uint8_t vol6_partc_1_SKD_msbyte_to_lsbyte[16] = {   // SKD
    0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,   //SKDs = 0x0213243546576879 (MSO to LSO)
    0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
  //0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC,   //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
  //0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};
uint8_t vol6_partc_1_SKD[16] = {                    // SKD
  //0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,   //SKDs = 0x0213243546576879 (MSO to LSO)
  //0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
    0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC,   //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
    0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};
*/
uint8_t  vol6_partc_1_SKDm[8] = {
  //0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
    0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC    //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
};
uint8_t  vol6_partc_1_SKDs[8] = {
  //0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79    //SKDs = 0x0213243546576879 (MSO to LSO)
    0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};
uint8_t  vol6_partc_1_2_IVm[4] = {     //vol-6 part-C 1.2    IV = DEAFBABEBADCAB24
    0x24,0xAB,0xDC,0xBA                //vol-6 part-C 1.2    IV = DEAFBABEBADCAB24  IVm = 0xBADCAB24 (MSO to LSO)
};
uint8_t  vol6_partc_1_2_IVs[4] = {     //vol-6 part-C 1.2    IV = DEAFBABEBADCAB24
    0xBE,0xBA,0xAF,0xDE                //vol-6 part-C 1.2    IV = DEAFBABEBADCAB24  IVs = 0xDEAFBABE (MSO to LSO)
};
uint8_t vol6_partc_1_2_SK[16] = {
    0x66,0xC6,0xC2,0x27,0x8E,0x3B,0x8E,0x05,0x3E,0x7E,0xA3,0x26,0x52,0x1B,0xAD,0x99  //vol-6 part-C 1.2 SK = 99AD1B5226A37E3E058E3B8E27C2C666
                                                                                                        //   MSbyte to LSbyte & msbit to lsbit
};
uint8_t vol6_partc_1_2_1_plaindata[16] = { // 1.2.  1. LL_START_ENC_RSP1(0x06) (packetCounter 0, M --> S)    LSbyte to MSbyte & msbit to lsbit
    0x06,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00             //B2 = 06000000000000000000000000000000
         //---------------padding--------------------------------------------------------
  //0x06:1-byte payload, LL opcode 0x06 LL_START_ENC_RSP
};
uint8_t vol6_partc_1_2_2_plaindata[16] = { // 1.2.  2. LL_START_ENC_RSP2(0x06) (packetCounter 0, S --> M)    LSbyte to MSbyte & msbit to lsbit
    0x06,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00             //B2 = 06000000000000000000000000000000
         //---------------padding--------------------------------------------------------
  //0x06:1-byte payload, LL opcode 0x06 LL_START_ENC_RSP
};
uint8_t vol6_partc_1_2_3_plaindata[256] = { // 1.2.  3. Data packet1 (packet 1, M --> S)                     LSbyte to MSbyte & msbit to lsbit
    //
    0x17,0x00,0x63,0x64,  0x65,0x66,0x67,0x68,  0x69,0x6A,0x6B,0x6C,  0x6D,0x6E,0x6F,0x70,            //B2 = 1700636465666768696A6B6C6D6E6F70
    0x71,0x31,0x32,0x33,  0x34,0x35,0x36,0x37,  0x38,0x39,0x30,0x01,  0x01,0x01,0x01,0x80             //B3 = 71313233343536373839300000000000
                                                               //-------padding----------
    /*
    0xf7,0x00,0x63,0x64,  0x64,0x65,0x66,0x67,  0x68,0x69,0x6A,0x6B,  0x6C,0x6D,0x6E,0x6F,            //B2
    0x70,0x71,0x72,0x73,  0x74,0x75,0x76,0x77,  0x78,0x79,0x7A,0x7B,  0x7C,0x7D,0x7E,0x7F,            //B3
    0x80,0x81,0x82,0x83,  0x84,0x85,0x86,0x87,  0x88,0x89,0x8A,0x8B,  0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,  0x94,0x95,0x96,0x97,  0x98,0x99,0x9A,0x9B,  0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,  0xA4,0xA5,0xA6,0xA7,  0xA8,0xA9,0xAA,0xAB,  0xAC,0xAD,0xAE,0xAF,
    0xB0,0xB1,0xB2,0xB3,  0xB4,0xB5,0xB6,0xB7,  0xB8,0xB9,0xBA,0xBB,  0xBC,0xBD,0xBE,0xBF,
    0xC0,0xC1,0xC2,0xC3,  0xC4,0xC5,0xC6,0xC7,  0xC8,0xC9,0xCA,0xCB,  0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,  0xD4,0xD5,0xD6,0xD7,  0xD8,0xD9,0xDA,0xDB,  0xDC,0xDD,0xDE,0xDF,
    0xE0,0xE1,0xE2,0xE3,  0xE4,0xE5,0xE6,0xE7,  0xE8,0xE9,0xEA,0xEB,  0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,0xF2,0xF3,  0xF4,0xF5,0xF6,0xF7,  0xF8,0xF9,0xFA,0xFB,  0xFC,0xFD,0xFE,0xFF,
    0x00,0x01,0x02,0x03,  0x04,0x05,0x06,0x07,  0x08,0x09,0x0A,0x0B,  0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,  0x14,0x15,0x16,0x17,  0x18,0x19,0x1A,0x1B,  0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,  0x24,0x25,0x26,0x27,  0x28,0x29,0x2A,0x2B,  0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,  0x34,0x35,0x36,0x37,  0x38,0x39,0x3A,0x3B,  0x3C,0x3D,0x3E,0x3F,
    0x40,0x41,0x42,0x43,  0x44,0x45,0x46,0x47,  0x48,0x49,0x4A,0x4B,  0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,  0x54,0x55,0x56,0x57,  0x58,0x59,0x5A,0x01,  0x01,0x01,0x01,0x80
                                                               //-------padding----------
    */
};
uint8_t vol6_partc_1_2_4_plaindata[256] = { // 1.2.  4. Data packet2 (packet 1, S --> M)                     LSbyte to MSbyte & msbit to lsbit
    //
    0x17,0x00,0x37,0x36,  0x35,0x34,0x33,0x32,  0x31,0x30,0x41,0x42,  0x43,0x44,0x45,0x46,            //B2 = 17003736353433323130414243444546
    0x47,0x48,0x49,0x4A,  0x4B,0x4C,0x4D,0x4E,  0x4F,0x50,0x51,0x01,  0x01,0x01,0x01,0x80             //B3 = 4748494A4B4C4D4E4F50510000000000
                                                               //-------padding----------
    /*
    0xf7,0x00,0x37,0x36,  0x44,0x45,0x46,0x47,  0x48,0x49,0x4A,0x4B,  0x4C,0x4D,0x4E,0x4F,            //B2
    0x50,0x51,0x52,0x53,  0x54,0x55,0x56,0x57,  0x58,0x59,0x5A,0x5B,  0x5C,0x5D,0x5E,0x5F,            //B3
    0x60,0x61,0x62,0x63,  0x64,0x65,0x66,0x67,  0x68,0x69,0x6A,0x6B,  0x6C,0x6D,0x6E,0x6F,
    0x70,0x71,0x72,0x73,  0x74,0x75,0x76,0x77,  0x78,0x79,0x7A,0x7B,  0x7C,0x7D,0x7E,0x7F,
    0x80,0x81,0x82,0x83,  0x84,0x85,0x86,0x87,  0x88,0x89,0x8A,0x8B,  0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,  0x94,0x95,0x96,0x97,  0x98,0x99,0x9A,0x9B,  0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,  0xA4,0xA5,0xA6,0xA7,  0xA8,0xA9,0xAA,0xAB,  0xAC,0xAD,0xAE,0xAF,
    0xB0,0xB1,0xB2,0xB3,  0xB4,0xB5,0xB6,0xB7,  0xB8,0xB9,0xBA,0xBB,  0xBC,0xBD,0xBE,0xBF,
    0xC0,0xC1,0xC2,0xC3,  0xC4,0xC5,0xC6,0xC7,  0xC8,0xC9,0xCA,0xCB,  0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,  0xD4,0xD5,0xD6,0xD7,  0xD8,0xD9,0xDA,0xDB,  0xDC,0xDD,0xDE,0xDF,
    0xE0,0xE1,0xE2,0xE3,  0xE4,0xE5,0xE6,0xE7,  0xE8,0xE9,0xEA,0xEB,  0xEC,0xED,0xEE,0xEF,
    0xF0,0xF1,0xF2,0xF3,  0xF4,0xF5,0xF6,0xF7,  0xF8,0xF9,0xFA,0xFB,  0xFC,0xFD,0xFE,0xFF,
    0x00,0x01,0x02,0x03,  0x04,0x05,0x06,0x07,  0x08,0x09,0x0A,0x0B,  0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,  0x14,0x15,0x16,0x17,  0x18,0x19,0x1A,0x1B,  0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,  0x24,0x25,0x26,0x27,  0x28,0x29,0x2A,0x2B,  0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,  0x34,0x35,0x36,0x37,  0x38,0x39,0x3A,0x01,  0x01,0x01,0x01,0x80
                                                               //-------padding----------
    */
};

void aesccm_Rx_on_the_fly(LEACL_TypeDef *pacl, bool write_iv)
{
    uint32_t *pU32;
    //aesccm TX disable IRQ BCMATCH
    //aesccm RX  enable IRQ BCMATCH
    RF_WT08(0x33, 0x00); //AESCCM       //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
  //
    RF_WT32(0xC0, 0x00001C01 | (pacl->RxPacketCounter.reg <<24) );
  /*
    RF_WT08(0xC0, 0x01);                //AESCCM  [7]  AESCCM_MAC_F
                                        //        [6]  -
                                        //       0[5]  AESCCM_TM_SEL AES-CCM selection for RF test mode. 1h�Gencryption, 0h�Gdecryption
                                        //       0[4]  AESCCM_TM_EN  AES-CCM enable for RF test mode.    1h�Genable,     0h�Gdisable
                                        //        [3:1]-
                                        //       1[0]  AESCCM_EN     AES-CCM enable for RF on the fly.   1h�Genable,     0h�Gdisable
  //RF_WT08(0xC1, 0x1C);//default 0x1C  //H_S0_MASK[7:0] 0xE3  [4]MD,[3]SN,[2]NESN bits masked to 0  for Data Channel PDU S0
  //RF_WT08(0xC2, 0x00);                //(Reserved)
    RF_WT08(0xC3, pacl->RxPacketCounter.reg);       //NONCE0    Octet0 (LSO) of packetCounter 39 bits
  */
    RF_WT32(0xC4, pacl->RxPacketCounter.reg >> 8);  //NONCE1    Octet1       of packetCounter 39 bits
                                                    //NONCE2    Octet2       of packetCounter 39 bits
                                                    //NONCE3    Octet3       of packetCounter 39 bits
                                                    //NONCE4    Octet4 (MSO) of packetCounter 39 bits
                                                    //       [7]directionBit 0:Slave to Master, 1:Master to Slave
  if (write_iv)
  {
    RF_WT32(0xC8, *(pU32 = (uint32_t *)(pacl->encrypt.IVm+0)));
                                                    //NONCE5    Octet0 (LSO) of IV
                                                    //NONCE6
                                                    //NONCE7
                                                    //NONCE8
    RF_WT32(0xCC, *(pU32 = (uint32_t *)(pacl->encrypt.IVs+0)));
                                                    //NONCE9
                                                    //NONCE10
                                                    //NONCE11
                                                    //NONCE12   Octet7 (MSO) of IV
  }
    pdma_ram_bleTx_write_payload(RX_CH0_SRCADDR + 0x0000,
                                 16,//length
                                 pacl->encrypt.SK+0                // Session Key (SK)
                                );
  // PDMA Transfer on CH0
  pdma_ch0_dt_ble (//0x00000000,              // IRQ
                   0x00030000+RX_CH0_SRCADDR  // SRC_ADDR
                   //0x000100B8,              // DES_ADDR    0xB8:DMA_AESW
                   //0x00000004 | (17UL << 16)// Data_Size   17:1+16 times, 4:4 words for an AES block
                                              //                + ++------16*(4 words)
                                              //                +---------SK
                   //0x00000729               // Enable 
                  );
  // PDMA Transfer on CH1
  pdma_ch1_dt_ble (//0x00000000,              // IRQ
                   //0x000100BC,              // SRC_ADDR    0xBC:DMA_AESR
                   0x00030000+RX_CH1_DESADDR  // DES_ADDR
                   //0x00000001 | (64UL << 16)// Data_Size   64:63+1 times, 1:1 word
                                              //                ++ +------MIC 1 word
                                              //                ++--------63*4 = 252 > 251
                   //0x00000789               // Enable
                  );
}

void aesccm_Tx_on_the_fly(LEACL_TypeDef *pacl, bool write_iv, uint16_t length, uint8_t *pch, uint32_t ramStartAddr)
{
    uint32_t *pU32;
    //aesccm TX disable IRQ BCMATCH
    //aesccm RX  enable IRQ BCMATCH
    RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_ON); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
  //
    RF_WT32(0xC0, 0x00001C01 | (pacl->TxPacketCounter.reg <<24) );
  /*
    RF_WT08(0xC0, 0x01);                //AESCCM  [7]  AESCCM_MAC_F
                                        //        [6]  -
                                        //       0[5]  AESCCM_TM_SEL AES-CCM selection for RF test mode. 1h�Gencryption, 0h�Gdecryption
                                        //       0[4]  AESCCM_TM_EN  AES-CCM enable for RF test mode.    1h�Genable,     0h�Gdisable
                                        //        [3:1]-
                                        //       1[0]  AESCCM_EN     AES-CCM enable for RF on the fly.   1h�Genable,     0h�Gdisable
  //RF_WT08(0xC1, 0x1C);//default 0x1C  //H_S0_MASK[7:0] 0xE3  [4]MD,[3]SN,[2]NESN bits masked to 0  for Data Channel PDU S0
  //RF_WT08(0xC2, 0x00);                //(Reserved)
    RF_WT08(0xC3, pacl->TxPacketCounter.reg);       //NONCE0    Octet0 (LSO) of packetCounter 39 bits
  */
    RF_WT32(0xC4, pacl->TxPacketCounter.reg >> 8);  //NONCE1    Octet1       of packetCounter 39 bits
                                                    //NONCE2    Octet2       of packetCounter 39 bits
                                                    //NONCE3    Octet3       of packetCounter 39 bits
                                                    //NONCE4    Octet4 (MSO) of packetCounter 39 bits
                                                    //       [7]directionBit 0:Slave to Master, 1:Master to Slave
  if (write_iv)
  {
    RF_WT32(0xC8, *(pU32 = (uint32_t *)(pacl->encrypt.IVm+0)));
                                                    //NONCE5    Octet0 (LSO) of IV
                                                    //NONCE6
                                                    //NONCE7
                                                    //NONCE8
    RF_WT32(0xCC, *(pU32 = (uint32_t *)(pacl->encrypt.IVs+0)));
                                                    //NONCE9
                                                    //NONCE10
                                                    //NONCE11
                                                    //NONCE12   Octet7 (MSO) of IV
  }
    pdma_ram_bleTx_write_payload(ramStartAddr + 0x0000,
                                 16,//length
                                 pacl->encrypt.SK+0                // Session Key (SK)
                                );
    /*
    pdma_ram_bleTx_write_payload(ramStartAddr + 0x0010,
                                 length
                                 pch+0                              // Plain Text with Padding
                                );
    */
  // PDMA Transfer on CH0
  pdma_ch0_dt_ble (//0x00000000,              // IRQ
                   0x00030000 + ramStartAddr  // SRC_ADDR
                   //0x000100B8,              // DES_ADDR    0xB8:DMA_AESW
                   //0x00000004 | (17UL << 16)// Data_Size   17:1+16 times, 4:4 words for an AES block
                                              //                + ++------16*(4 words)
                                              //                +---------SK
                   //0x00000729               // Enable
                  );
  // PDMA Transfer on CH1
  pdma_ch1_dt_ble (//0x00000000,              // IRQ
                   //0x000100BC,              // SRC_ADDR    0xBC:DMA_AESR
                   0x00030000+TX_CH1_DESADDR  // DES_ADDR
                   //0x00000001 | (64UL << 16)// Data_Size   64:63+1 times, 1:1 word
                                              //                ++ +------MIC 1 word
                                              //                ++--------63*4 = 252 > 251
                   //0x00000789               // Enable
                  );
}

////////////////////////////////////////
void set_txPayload_TRT_T1_adv(uint8_t *pAdvA, uint8_t advDataLength, uint8_t *pAdvData)
{
    // BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1+0,             6, pAdvA   );//send adv_ind, AdvA       6 octets
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1+6, advDataLength, pAdvData);//send adv_ind, AdvData 0-31 octets
}
void set_txHeader_TRT_T1_adv(uint8_t advDataLength)
{
    uint8_t hdr_length;
    ADV_PDU_HDR_S0_TypeDef hdr_s0 =
    {
//      .field.type   = PDUTYPE_ADV_IND,
//      .field.type   = PDUTYPE_ADV_SCAN_IND,
//      .field.type   = PDUTYPE_ADV_NONCONN_IND,
//      .field.type   = PDUTYPE_ADV_DIRECT_IND,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0
    };
           hdr_length = 6+advDataLength;
    switch( leconfig_AdvParam.adv_type )
    {
                                                        //0x00: Connectable and scannable undirected adv (ADV_IND) (default) 
                                                        //0x01: Connectable high duty cycle directed adv (ADV_DIRECT_IND, high duty cycle)
                                                        //0x02: Scannable undirected adv (ADV_SCAN_IND)
                                                        //0x03: Non connectable undirected adv (ADV_NONCONN_IND)
                                                        //0x04: Connectable low duty cycle directed adv (ADV_DIRECT_IND, low duty cycle)
    case 0x00:  hdr_s0.field.type = PDUTYPE_ADV_IND ;         break;
    case 0x01:  hdr_s0.field.type = PDUTYPE_ADV_DIRECT_IND ;  break;
    case 0x02:  hdr_s0.field.type = PDUTYPE_ADV_SCAN_IND ;    break;
    case 0x03:  hdr_s0.field.type = PDUTYPE_ADV_NONCONN_IND ; break;
    case 0x04:  hdr_s0.field.type = PDUTYPE_ADV_DIRECT_IND ;  break;
    }
    txAdvHeaderS0.s0 = hdr_s0.s0; //save at txAdvHeaderS0
                RF_WT08(0x14, hdr_s0.s0);           //HB1 H_S0
                RF_WT08(0x15, hdr_length);          //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    //HB1 H_S0 [3:0]PDU Type
                                                    //           0000    ADV_IND                connectable, undirected
                                                    //           0001    ADV_DIRECT_IND         connectable, directed
                                                    //           0010    ADV_NONCONN_IND        non-connectable, undirected
                                                    //           0011    SCAN_REQ
                                                    //               AUX_SCAN_REQ    2nd
                                                    //           0100    SCAN_RSP
                                                    //           0101    CONNECT_REQ
                                                    //               AUX_CONNECT_REQ    2nd
                                                    //           0110    ADV_SCAN_IND (spec 4.0?? ADV_DISCOVER_IND renamed)
                                                    //           0111    ADV_EXT_IND    1st
                                                    //               AUX_ADV_IND    2nd
                                                    //               AUX_SCAN_RSP    2nd
                                                    //               AUX_SYNC_IND    2nd
                                                    //               AUX_CHAIN_IND    2nd
                                                    //           1000    AUX_CONNECT_RSP    2nd
                                                    //           1001-1111 reserved
                                                    //       [4]  -
                                                    //       [5]  -  (BT4) -
                                                    //               (BT5) ChSel
                                                    //       [6]  TxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
                                                    //       [7]  RxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
}

////////////////////////////////////////
void set_txPayload_RTR_T_connectIND(LEACL_TypeDef *pacl, uint8_t *pRemoteDeviceAddr)
{
    // BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode
    CONN_IND_PAYLOAD_TypeDef txConnIndPayload;
    irq1isr_memory_copy(txConnIndPayload.initA+0,                  leconfig_bdaddr.le_public_AdvA+0, 6);   //InitA   send connectIND
    irq1isr_memory_copy(txConnIndPayload.advA+0,                   pRemoteDeviceAddr+0,              6);   //AdvA    send connectIND
    irq1isr_memory_copy(                pacl->remoteDeviceAddr+0,  pRemoteDeviceAddr+0,              6);   //AdvA    send connectIND
                                    //  pacl->AA=                  calc_syncword_32(rand()); //AA (4 octets) Access Address
                                        pacl->AA=                  calc_syncword_32(0x3488); //AA (4 octets) Access Address
                                    //  pacl->AA=                  0x53222227;               //AA (4 octets) Access Address
    irq1isr_memory_copy((uint8_t *)&txConnIndPayload.llData.AA, (uint8_t *)&pacl->AA, 4);    //AA (4 octets) Access Address
                                        pacl->crcInit[0]=          0x14;                     //CRCInit  (3 octets) initialization value for the CRC calculation
                                        pacl->crcInit[1]=          0x15;                     //CRCInit  (3 octets) initialization value for the CRC calculation
                                        pacl->crcInit[2]=          0x16;                     //CRCInit  (3 octets) initialization value for the CRC calculation
    irq1isr_memory_copy(txConnIndPayload.llData.crcInit+0, pacl->crcInit+0, 3);              //CRCInit  (3 octets) initialization value for the CRC calculation
    txConnIndPayload.llData.winSize=    pacl->currParam_winSize=   5;           //WinSize  (1 octet ) transmitWindowSize
    txConnIndPayload.llData.winOffset=  pacl->currParam_winOffset= 2;//2;       //WinOffset(2 octets) transmitWindowOffset
    txConnIndPayload.llData.interval=   pacl->currParam.interval=  ht_read_hword((uint8_t *)&(leconfig_CreateConnection.conn_interval_min));  //42;    //Interval (2 octets) connInterval       100*1.25ms=125ms     800*1.25ms=1000ms
    txConnIndPayload.llData.latency=    pacl->currParam.latency=   ht_read_hword((uint8_t *)&(leconfig_CreateConnection.slave_latency));      //0;     //Latency  (2 octets) connSlaveLatency
    txConnIndPayload.llData.timeout=    pacl->currParam.timeout=   ht_read_hword((uint8_t *)&(leconfig_CreateConnection.supervision_timeout));//0x0C80;//Timeout  (2 octets) connSupervisionTimeout  unit 10ms,   range: 100ms to 32.0sec (0x000A to 0x0C80)
    txConnIndPayload.llData.chM[0]=     pacl->currChM.chM[0]=      0xFF;        //ChM (5 octets) channel map
    txConnIndPayload.llData.chM[1]=     pacl->currChM.chM[1]=      0xFF;        //ChM (5 octets) channel map
    txConnIndPayload.llData.chM[2]=     pacl->currChM.chM[2]=      0xFF;        //ChM (5 octets) channel map
    txConnIndPayload.llData.chM[3]=     pacl->currChM.chM[3]=      0xFF;        //ChM (5 octets) channel map
    txConnIndPayload.llData.chM[4]=     pacl->currChM.chM[4]=      0x1F;        //ChM (5 octets) channel map
    txConnIndPayload.llData.hop=        pacl->hopIncrement=        8;           //Hop (5 bits)   hopIncrement, 5~16
    txConnIndPayload.llData.sca=        pacl->sca         =        7;           //SCA (3 bits)   masterSCA, worst case Master's sleep clock accuracy
                                                                                    // 0: 251 ppm to 500 ppm
                                                                                    // 1: 151 ppm to 250 ppm
                                                                                    // 2: 101 ppm to 150 ppm
                                                                                    // 3:  76 ppm to 100 ppm
                                                                                    // 4:  51 ppm to  75 ppm
                                                                                    // 5:  31 ppm to  50 ppm
                                                                                    // 6:  21 ppm to  30 ppm
                                                                                    // 7:   0 ppm to  20 ppm
  //pdma_ram_bleTx_write_payload(CH2TXADDR_RTR_TX_CONNECTIND,                           6+6+22, txConnIndPayload.initA+0);//send connect_ind
    pdma_ram_bleTx_write_payload(CH2TXADDR_RTR_TX_CONNECTIND, sizeof(CONN_IND_PAYLOAD_TypeDef), txConnIndPayload.initA+0);//send connect_ind
}
static inline void set_txHeader_RTR_T_connectIND(void)
{
    ADV_PDU_HDR_TypeDef hdr =
    {
        .field.type   = PDUTYPE_CONNECT_IND,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0,
        .field.length = sizeof(CONN_IND_PAYLOAD_TypeDef) //0x22
    //  .field.length = 0x22 //sizeof(CONN_IND_PAYLOAD_TypeDef)
    };
                RF_WT16(0x14, hdr.reg);                 //HB1 H_S0
                                                        //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
}
static inline void set_txPayload_RTR_T_scanREQ(uint8_t *pAdvA)
{
    unsigned char zzz[6+6];
            zzz[0]= leconfig_bdaddr.le_public_AdvA[0];//ScanA
            zzz[1]= leconfig_bdaddr.le_public_AdvA[1];//ScanA
            zzz[2]= leconfig_bdaddr.le_public_AdvA[2];//ScanA
            zzz[3]= leconfig_bdaddr.le_public_AdvA[3];//ScanA
            zzz[4]= leconfig_bdaddr.le_public_AdvA[4];//ScanA
            zzz[5]= leconfig_bdaddr.le_public_AdvA[5];//ScanA
            zzz[6]= pAdvA[0];//AdvA
            zzz[7]= pAdvA[1];//AdvA
            zzz[8]= pAdvA[2];//AdvA
            zzz[9]= pAdvA[3];//AdvA
            zzz[10]=pAdvA[4];//AdvA
            zzz[11]=pAdvA[5];//AdvA
    pdma_ram_bleTx_write_payload(CH2TXADDR_RTR_TX_SCANREQ, /*length*/ 6+6, zzz+0);//RTR event T scan_req <--roger adv_ind
}
static inline void set_txHeader_RTR_T_scanREQ(void)
{
    ADV_PDU_HDR_TypeDef hdr =
    {
        .field.type   = PDUTYPE_SCAN_REQ,
      //.field.type   = PDUTYPE_ADV_IND,//debug make pdu_type wrong
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0,
        .field.length = 6+6
    };
                RF_WT16(0x14, hdr.reg);                 //HB1 H_S0
                                                        //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
}
static inline void set_txPayload_TRT_T2_scanResponse(uint8_t *pAdvA, uint8_t scanResponseDataLength, uint8_t *pScanResponseData)
{
    // BLE mode with only 1 RING RX FIFO and 1 RING TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T2_SCANRSP+0,                      6, pAdvA            );//send scan_rsp, AdvA           6 octets
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T2_SCANRSP+6, scanResponseDataLength, pScanResponseData);//send scan_rsp, ScanRspData 0-31 octets
}
static inline void set_txHeader_TRT_T2_scanResponse(uint8_t scanResponseDataLength)
{
    // BLE mode with only 1 RING RX FIFO and 1 RING TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode
    ADV_PDU_HDR_TypeDef hdr =
    {
        .field.type   = PDUTYPE_SCAN_RSP,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0,
        .field.length = 6+scanResponseDataLength
    };
                RF_WT16(0x14, hdr.reg);             //HB1 H_S0
                                                    //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    //HB1 H_S0 [3:0]PDU Type
                                                    //           0000    ADV_IND                connectable, undirected
                                                    //           0001    ADV_DIRECT_IND         connectable, directed
                                                    //           0010    ADV_NONCONN_IND        non-connectable, undirected
                                                    //           0011    SCAN_REQ
                                                    //               AUX_SCAN_REQ    2nd
                                                    //           0100    SCAN_RSP
                                                    //           0101    CONNECT_REQ
                                                    //               AUX_CONNECT_REQ    2nd
                                                    //           0110    ADV_SCAN_IND (spec 4.0?? ADV_DISCOVER_IND renamed)
                                                    //           0111    ADV_EXT_IND    1st
                                                    //               AUX_ADV_IND    2nd
                                                    //               AUX_SCAN_RSP    2nd
                                                    //               AUX_SYNC_IND    2nd
                                                    //               AUX_CHAIN_IND    2nd
                                                    //           1000    AUX_CONNECT_RSP    2nd
                                                    //           1001-1111 reserved
                                                    //       [4]  -
                                                    //       [5]  -  (BT4) -
                                                    //               (BT5) ChSel
                                                    //       [6]  TxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
                                                    //       [7]  RxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
}

void set_txHeader_Tx_adv_ext_ind(void)
{
    uint8_t hdr_length;
    ADV_PDU_HDR_S0_TypeDef hdr_s0 =
    {
	.field.type = PDUTYPE_EXT_IND,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0
    };
                hdr_length = 14;
   		txAdvHeaderS0.s0 = hdr_s0.s0; //save at txAdvHeaderS0
                RF_WT08(0x14, hdr_s0.s0);           //HB1 H_S0
                RF_WT08(0x15, hdr_length);          //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    //HB1 H_S0 [3:0]PDU Type
                                                    //           0000    ADV_IND                connectable, undirected
                                                    //           0001    ADV_DIRECT_IND         connectable, directed
                                                    //           0010    ADV_NONCONN_IND        non-connectable, undirected
                                                    //           0011    SCAN_REQ
                                                    //               AUX_SCAN_REQ    2nd
                                                    //           0100    SCAN_RSP
                                                    //           0101    CONNECT_REQ
                                                    //               AUX_CONNECT_REQ    2nd
                                                    //           0110    ADV_SCAN_IND (spec 4.0?? ADV_DISCOVER_IND renamed)
                                                    //           0111    ADV_EXT_IND    1st
                                                    //               AUX_ADV_IND    2nd
                                                    //               AUX_SCAN_RSP    2nd
                                                    //               AUX_SYNC_IND    2nd
                                                    //               AUX_CHAIN_IND    2nd
                                                    //           1000    AUX_CONNECT_RSP    2nd
                                                    //           1001-1111 reserved
                                                    //       [4]  -
                                                    //       [5]  -  (BT4) -
                                                    //               (BT5) ChSel
                                                    //       [6]  TxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
                                                    //       [7]  RxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
}

void set_txHeader_Tx_aux_adv_ind(void)
{
    uint8_t hdr_length;
    ADV_PDU_HDR_S0_TypeDef hdr_s0 =
    {
	.field.type = PDUTYPE_EXT_IND,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0
    };
                hdr_length = 23;
   		txAdvHeaderS0.s0 = hdr_s0.s0; //save at txAdvHeaderS0
                RF_WT08(0x14, hdr_s0.s0);           //HB1 H_S0
                RF_WT08(0x15, hdr_length);          //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    //HB1 H_S0 [3:0]PDU Type
                                                    //           0000    ADV_IND                connectable, undirected
                                                    //           0001    ADV_DIRECT_IND         connectable, directed
                                                    //           0010    ADV_NONCONN_IND        non-connectable, undirected
                                                    //           0011    SCAN_REQ
                                                    //               AUX_SCAN_REQ    2nd
                                                    //           0100    SCAN_RSP
                                                    //           0101    CONNECT_REQ
                                                    //               AUX_CONNECT_REQ    2nd
                                                    //           0110    ADV_SCAN_IND (spec 4.0?? ADV_DISCOVER_IND renamed)
                                                    //           0111    ADV_EXT_IND    1st
                                                    //               AUX_ADV_IND    2nd
                                                    //               AUX_SCAN_RSP    2nd
                                                    //               AUX_SYNC_IND    2nd
                                                    //               AUX_CHAIN_IND    2nd
                                                    //           1000    AUX_CONNECT_RSP    2nd
                                                    //           1001-1111 reserved
                                                    //       [4]  -
                                                    //       [5]  -  (BT4) -
                                                    //               (BT5) ChSel
                                                    //       [6]  TxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
                                                    //       [7]  RxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
}
void set_txHeader_Tx_aux_sync_ind(void)
{
    uint8_t hdr_length;
    ADV_PDU_HDR_S0_TypeDef hdr_s0 =
    {
	.field.type = PDUTYPE_EXT_IND,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 1,      // 0:public, 1:random
        .field.rxadd  = 0
    };
                hdr_length = 9;
   		txAdvHeaderS0.s0 = hdr_s0.s0; //save at txAdvHeaderS0
                RF_WT08(0x14, hdr_s0.s0);           //HB1 H_S0
                RF_WT08(0x15, hdr_length);          //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    //HB1 H_S0 [3:0]PDU Type
                                                    //           0000    ADV_IND                connectable, undirected
                                                    //           0001    ADV_DIRECT_IND         connectable, directed
                                                    //           0010    ADV_NONCONN_IND        non-connectable, undirected
                                                    //           0011    SCAN_REQ
                                                    //               AUX_SCAN_REQ    2nd
                                                    //           0100    SCAN_RSP
                                                    //           0101    CONNECT_REQ
                                                    //               AUX_CONNECT_REQ    2nd
                                                    //           0110    ADV_SCAN_IND (spec 4.0?? ADV_DISCOVER_IND renamed)
                                                    //           0111    ADV_EXT_IND    1st
                                                    //               AUX_ADV_IND    2nd
                                                    //               AUX_SCAN_RSP    2nd
                                                    //               AUX_SYNC_IND    2nd
                                                    //               AUX_CHAIN_IND    2nd
                                                    //           1000    AUX_CONNECT_RSP    2nd
                                                    //           1001-1111 reserved
                                                    //       [4]  -
                                                    //       [5]  -  (BT4) -
                                                    //               (BT5) ChSel
                                                    //       [6]  TxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
                                                    //       [7]  RxAdd, contain info specific to the PDU type defined for each advertising channel PDU separately
}
void set_txPayload_Tx_adv_ext_ind(uint8_t *pAdvA,uint8_t AdvDID ,uint8_t AdvSID, uint8_t Channel_index,uint16_t Aux_Offset)
{
  
    ext_adv_data.length = 14;
    ext_adv_data.ExtAdvHeader[0] = 0x0D;	//Extended Header Length | AdvMode 00b Non-connectable Non-scannable
    ext_adv_data.ExtAdvHeader[1] = 0x59;	////Extended Header Flags [0]AdvA [1]TargetA [2]CTEInfo [3]AdvDataInfo (ADI) [4]AuxPtr [5]SyncInfo [6]TxPower [7]Reserved for future use
    ext_adv_data.ExtAdvHeader[2] = pAdvA[0];	//AdvA
    ext_adv_data.ExtAdvHeader[3] = pAdvA[1];
    ext_adv_data.ExtAdvHeader[4] = pAdvA[2];
    ext_adv_data.ExtAdvHeader[5] = pAdvA[3];
    ext_adv_data.ExtAdvHeader[6] = pAdvA[4];
    ext_adv_data.ExtAdvHeader[7] = pAdvA[5];
    ext_adv_data.ExtAdvHeader[8] = (AdvDID&0x00FF); 	//AdvDatainfo DID[0-7]
    ext_adv_data.ExtAdvHeader[9] = (AdvDID&0x0F00>>4)|(AdvSID<<4);	//AdvDatainfo DID[8-11] | SID[0-3]<<4
    ext_adv_data.ExtAdvHeader[10]= (Channel_index | 0x40 | 0x00);  // Channel Index[0-5]|CA[6]|Offset Units[7] 
    								   //CA 0b=51ppm to 500ppm, 1b=0ppm to 50ppm
    								   //Offset Units 0b=30�gs less than 245,700 �gs, 1b= 300�gs
    ext_adv_data.ExtAdvHeader[11]= (Aux_Offset&0x00FF) ;		   // Aux_Offset
    ext_adv_data.ExtAdvHeader[12]= ((Aux_Offset&0x1F00)>>8)| 0x00; 	  // Aux_Offset | Aux PHY 000b LE 1M 	
    ext_adv_data.ExtAdvHeader[13]= 0x05;
    
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1+0,ext_adv_data.length ,ext_adv_data.ExtAdvHeader+0 );//send adv_ind, AdvA       6 octets

}
void set_txPayload_Tx_aux_adv_ind(uint8_t AdvDID , uint8_t AdvSID, uint16_t Offset_base, uint16_t Interval, uint8_t *ChM, uint32_t AA, uint8_t *crcInit, uint16_t PeriodicEventCounter)
{
  
    ext_adv_data.length = 23;
    ext_adv_data.ExtAdvHeader[0] = 0x16;	//Extended Header Length | AdvMode 00b Non-connectable Non-scannable Undirected AUX_ADV_IND
    ext_adv_data.ExtAdvHeader[1] = 0x68;	////Extended Header Flags [0]AdvA [1]TargetA [2]CTEInfo [3]AdvDataInfo (ADI) [4]AuxPtr [5]SyncInfo [6]TxPower [7]Reserved for future use
    ext_adv_data.ExtAdvHeader[2] = (AdvDID&0x00FF); 	//AdvDatainfo DID[0-7]
    ext_adv_data.ExtAdvHeader[3] = (AdvDID&0x0F00>>4)|(AdvSID<<4);	//AdvDatainfo DID[8-11] | SID[0-3]<<4
    ext_adv_data.ExtAdvHeader[4]=  (Offset_base&0x00FF) ;  // offset_base
    ext_adv_data.ExtAdvHeader[5]=  (Offset_base&0x1F00>>8)| 0x00 |0x00;		   // offset_base |offset_units|offset_adjust
    ext_adv_data.ExtAdvHeader[6]=  (Interval&0x00FF) ; 	  // Interval	
    ext_adv_data.ExtAdvHeader[7]=  (Interval&0xFF00)>>8 ;
    ext_adv_data.ExtAdvHeader[8]=  ChM[0];
    ext_adv_data.ExtAdvHeader[9]=  ChM[1];
    ext_adv_data.ExtAdvHeader[10]= ChM[2];
    ext_adv_data.ExtAdvHeader[11]= ChM[3];
    ext_adv_data.ExtAdvHeader[12]= (ChM[4]&0x1F)|0xE0; //ChM|SCA
    							//SCA 7 = 0 ppm to 20 ppm;
    ext_adv_data.ExtAdvHeader[13]= (AA&0x000000FF);
    ext_adv_data.ExtAdvHeader[14]= (AA&0x0000FF00)>>8;
    ext_adv_data.ExtAdvHeader[15]= (AA&0x00FF0000)>>16;
    ext_adv_data.ExtAdvHeader[16]= (AA&0xFF000000)>>24;
    ext_adv_data.ExtAdvHeader[17]= crcInit[0];
    ext_adv_data.ExtAdvHeader[18]= crcInit[1];
    ext_adv_data.ExtAdvHeader[19]= crcInit[2];
    ext_adv_data.ExtAdvHeader[20]= (PeriodicEventCounter&0x00FF);
    ext_adv_data.ExtAdvHeader[21]= (PeriodicEventCounter&0xFF00)>>8;
    ext_adv_data.ExtAdvHeader[22]= 0x05;
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1+0,ext_adv_data.length ,ext_adv_data.ExtAdvHeader+0 );//send adv_ind, AdvA       6 octets
}
void set_txPayload_Tx_aux_sync_ind(uint8_t AdvDID , uint8_t AdvSID, uint8_t Channel_index,uint16_t Aux_Offset, uint8_t CTE_type , uint8_t CTE_times)
{
  
    ext_adv_data.length = 9;
    ext_adv_data.ExtAdvHeader[0] = 0x08;	//Extended Header Length | AdvMode 00b Non-connectable Non-scannable Undirected AUX_ADV_IND
    ext_adv_data.ExtAdvHeader[1] = 0x5C;	////Extended Header Flags [0]AdvA [1]TargetA 1[2]CTEInfo 1[3]AdvDataInfo (ADI) 1[4]AuxPtr [5]SyncInfo 1[6]TxPower [7]Reserved for future use
    ext_adv_data.ExtAdvHeader[2] = (CTE_type<<6)|CTE_times; 	//CTEInfo  [7-6]CTEType [5]RFU [4-0]CTETime  
    ext_adv_data.ExtAdvHeader[3] = (AdvDID&0x00FF); 	//AdvDatainfo DID[0-7]
    ext_adv_data.ExtAdvHeader[4] = (AdvDID&0x0F00>>4)|(AdvSID<<4);	//AdvDatainfo DID[8-11] | SID[0-3]<<4        
    ext_adv_data.ExtAdvHeader[5] = (Channel_index | 0x40 | 0x00);  // Channel Index[0-5]|CA[6]|Offset Units[7] 
    								   //CA 0b=51ppm to 500ppm, 1b=0ppm to 50ppm
    								   //Offset Units 0b=30�gs less than 245,700 �gs, 1b= 300�gs
    ext_adv_data.ExtAdvHeader[6] = (Aux_Offset&0x00FF) ;		   // Aux_Offset
    ext_adv_data.ExtAdvHeader[7] = ((Aux_Offset&0x1F00)>>8)| 0x00; 	  // Aux_Offset | Aux PHY 000b LE 1M     
    ext_adv_data.ExtAdvHeader[8] = 0x05;
    
    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1+0,ext_adv_data.length ,ext_adv_data.ExtAdvHeader+0 );//send adv_ind, AdvA       6 octets
}
void set_txCTEsetting_Tx_aux_sync_ind(uint8_t cte_type, uint8_t cte_length, uint8_t cte_length_of_sp,uint8_t cte_switching_patterns)
{
	if(cte_type == 0x00) //Tx AoA
	{
	  RF_WT08(0x53,0x01);
	  RF_WT08(0x54,cte_length);
	  RF_WT08(0x5C, (RF_RD08(0x5C)&0x7F)|0x00);
	  RF_WT08(0x58, (RF_RD08(0x58)&0x7F)|0x00);
	  RF_WT08(0x59, (RF_RD08(0x59)&0x7F)|0x00);
	  RF_WT08(0x5A, (RF_RD08(0x5A)&0x7F)|0x00);
	  RF_WT08(0x5B, (RF_RD08(0x5B)&0x7F)|0x00);	  	  
	}
}
    CONN_IND_PAYLOAD_TypeDef rxConnIndPayload;        
static inline void read_rxPayload_connectIND(LEACL_TypeDef *pacl)
{
  //CONN_IND_PAYLOAD_TypeDef rxConnIndPayload;
    pdma_ram_bleRx_read_payload(CH2RXADDR_TRT_RX, /*hdr_length*/0x22, rxConnIndPayload.initA+0);//TRT event R  CONNECT_IND
    //
    irq1isr_memory_copy(pacl->remoteDeviceAddr+0,   rxConnIndPayload.initA+0,          6);   // initA[6]    received CONNECT_IND
        ht_write_word ( (uint8_t *)&(pacl->AA                 ), ht_read_word ((uint8_t *)&(rxConnIndPayload.llData.AA       )) );
    irq1isr_memory_copy(pacl->crcInit+0,            rxConnIndPayload.llData.crcInit+0, 3);   // crcInit[3]
        ht_write_byte ( (uint8_t *)&(pacl->currParam_winSize  ), ht_read_byte ((uint8_t *)&(rxConnIndPayload.llData.winSize  )) );
        ht_write_hword( (uint8_t *)&(pacl->currParam_winOffset), ht_read_hword((uint8_t *)&(rxConnIndPayload.llData.winOffset)) );
        ht_write_hword( (uint8_t *)&(pacl->currParam.interval ), ht_read_hword((uint8_t *)&(rxConnIndPayload.llData.interval )) );
        ht_write_hword( (uint8_t *)&(pacl->currParam.latency  ), ht_read_hword((uint8_t *)&(rxConnIndPayload.llData.latency  )) );
        ht_write_hword( (uint8_t *)&(pacl->currParam.timeout  ), ht_read_hword((uint8_t *)&(rxConnIndPayload.llData.timeout  )) );
    irq1isr_memory_copy(pacl->currChM.chM+0,        rxConnIndPayload.llData.chM+0,     5);   // chM[5]
                        pacl->hopIncrement        = rxConnIndPayload.llData.hop ;
                        pacl->sca                 = rxConnIndPayload.llData.sca ;
    //
    calc_usedChannel_byRemapIndex(&(pacl->currChM));
    //
                        pacl->flag1.field.conn_setup_success_syncword_match = 0;   //slave
                        pacl->flag1.field.conn_setup_success_roger_nesn_1 = 0;     //slave
                        pacl->iSlave_ce1stRx_lost_rxto_consecutive_times = 0;
                        pacl->connEventCount=0xFFFF; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                      //pacl->connEventCount=0xC000; //debug the ReferenceConnEventCount and Instant are on different sides of the eventCount wraparound
                        pacl->lastUnmappedChIndex=0;
                        pacl->transmitHdr.field.transmitSeqNum=0;
                        pacl->transmitHdr.field.nextExpectedSeqNum=0;
                        pacl->txw4ack = 0x00;//0x00:none
  //if( rxConnIndPayload.initA[1] == 0x65 ) { //master_DeviceAddr[0]
  //  debug_RFLA_pulse_to_trigger_LA();
  //  debug_RFLA_pulse_to_trigger_LA();
  //}
}

static inline void irq1isr_txllcpdu_pre_setting(llc_buffer_tx2air_TypeDef *cqBuff, LEACL_TypeDef *pacl)
{
            switch ( (uint8_t)(cqBuff->llcpdu_payload.opcode))
            {
            case 0x03: //LL_03_ENC_REQ:
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 1;
                    break;
            case 0x05: //LL_05_START_ENC_REQ:
                    pacl->flag1.field.Tx_aesccm_enabled = 0;//Slave send LL_05_START_ENC_REQ unencrypted, then LL shall be set up to receive an encrypted packet in response
                    pacl->flag1.field.Rx_aesccm_enabled = 1;//Slave send LL_05_START_ENC_REQ unencrypted, then LL shall be set up to receive an encrypted packet in response
                    break;
            case 0x06: //LL_06_START_ENC_RSP:
                    pacl->flag1.field.Tx_aesccm_enabled = 1;//send LL_06_START_ENC_RSP
                    pacl->flag1.field.Rx_aesccm_enabled = 1;//send LL_06_START_ENC_RSP
                if (LEACL_is_role_slave(pacl)) {
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                    break;
            case 0x0A: //LL_0A_PAUSE_ENC_REQ:
                    pacl->flag1.field.Tx_aesccm_enabled = 1;
                    pacl->flag1.field.Rx_aesccm_enabled = 1;
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 1;
                    break;
            case 0x0B: //LL_0B_PAUSE_ENC_RSP:
                if (LEACL_is_role_slave(pacl)) {
                    pacl->flag1.field.Tx_aesccm_enabled = 1;//Slave send LL_0B_PAUSE_ENC_RSP
                    pacl->flag1.field.Rx_aesccm_enabled = 0;//Slave send LL_0B_PAUSE_ENC_RSP
                }
                else { //LEACL_is_role_master(pacl)
                    pacl->flag1.field.Tx_aesccm_enabled = 0;//Master send LL_0B_PAUSE_ENC_RSP
                    pacl->flag1.field.Rx_aesccm_enabled = 0;//Master send LL_0B_PAUSE_ENC_RSP
                }
                    break;
            case 0x0D:  //LL_0D_REJECT_IND:
                if( LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x03) ) { //LL_03_ENC_REQ
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                else
                if( LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x0A) ) { //LL_0A_PAUSE_ENC_REQ
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                    break;
            case 0x11:  //LL_11_REJECT_EXT_IND:
                if( cqBuff->llcpdu_payload.ctrData[0] == 0x03 ) { //LL_03_ENC_REQ         uint8_t rejectOpcode; //ctrData[0]
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                else
                if( cqBuff->llcpdu_payload.ctrData[0] == 0x0A ) { //LL_0A_PAUSE_ENC_REQ   uint8_t rejectOpcode; //ctrData[0]
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                    break;
            }
}
void set_txHeader_txPayload_connectionEventPacket(LEACL_TypeDef *pacl, bool write_iv)
{
    bool aesccm_encrypt_en = (bool)(pacl->flag1.field.Tx_aesccm_enabled);
    // BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode
        if( pacl->roundtrip_times_in_a_ce )
            pacl->roundtrip_times_in_a_ce --;
        if( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 0 ||
            pacl->invalid_crc_consecutive_times_within_ce >= 2) //2:two consecutive packets received with an invalid CRC match within a connection event shall close the event
            pacl->transmitHdr.field.md = 0;
        else
            pacl->transmitHdr.field.md = (pacl->roundtrip_times_in_a_ce != 0);
    
    if     (pacl->txw4ack == 0x00)  //0x00:none
    {
        if( RINGBUF_isNonempty( pacl->llcTxQ_high )  // non empty queue
            && ( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 1 )
          )
        {
                llc_buffer_tx2air_TypeDef *cqBuff;
                cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high) );
                pacl->flag1.field.current_llcTxQ_type = 1;      //0:llcTxQ, 1:llcTxQ_high
                irq1isr_txllcpdu_pre_setting(cqBuff, pacl);
                aesccm_encrypt_en = (bool)(pacl->flag1.field.Tx_aesccm_enabled); //in case modified pacl->flag1.field.Tx_aesccm_enabled
            //
            pacl->transmitHdr.field.transmitLLID       = 3;
          //pacl->transmitHdr.field.nextExpectedSeqNum
          //pacl->transmitHdr.field.transmitSeqNum
          //pacl->transmitHdr.field.md                 = set above
            pacl->transmitHdr.field.rfu                = 0;
            pacl->transmitHdr.field.transmitLength     = cqBuff->length;
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
            if (aesccm_encrypt_en == 0) {
              //pdma_ram_bleTx_write_payload(cqBuff->ram_start_address + 0x0010, //0x0010: first 16 bytes for Session Key (SK)
              //                             cqBuff->length,
              //                              (unsigned char *)&(cqBuff->llcpdu_payload)
              //                            );
                pdma_ch2_bleTx_setting__aesccmEN0(cqBuff->ram_start_address + 0x0010); //0x0010: first 16 bytes for Session Key (SK)
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            else {
                aesccm_Tx_on_the_fly(pacl, write_iv,
                                             cqBuff->length,
                                              (unsigned char *)&(cqBuff->llcpdu_payload),
                                           cqBuff->ram_start_address
                                    );
            }
                pacl->txw4ack = 0x03; //0x03:sending LLC  , waiting ACK...
            if( cqBuff->llcpdu_payload.opcode == LL_02_TERMINATE_IND ) {
                pacl->flag1.field.txLLTerminateInd_w4ack = 0x01; //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
            }
        }
        else
        if( RINGBUF_isNonempty( pacl->llcTxQ )  // non empty queue
            && ( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 1 )
            && ( pacl->flag1.field.stop_tx_dataPhyChannelPdu == 0 )
          )
        {
                llc_buffer_tx2air_TypeDef *cqBuff;
                cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ) );
                pacl->flag1.field.current_llcTxQ_type = 0;          //0:llcTxQ, 1:llcTxQ_high
                irq1isr_txllcpdu_pre_setting(cqBuff, pacl);
                aesccm_encrypt_en = (bool)(pacl->flag1.field.Tx_aesccm_enabled); //in case modified pacl->flag1.field.Tx_aesccm_enabled
            //
            pacl->transmitHdr.field.transmitLLID       = 3;
          //pacl->transmitHdr.field.nextExpectedSeqNum
          //pacl->transmitHdr.field.transmitSeqNum
          //pacl->transmitHdr.field.md                 = set above
            pacl->transmitHdr.field.rfu                = 0;
            pacl->transmitHdr.field.transmitLength     = cqBuff->length;
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
            if (aesccm_encrypt_en == 0) {
              //pdma_ram_bleTx_write_payload(cqBuff->ram_start_address + 0x0010, //0x0010: first 16 bytes for Session Key (SK)
              //                             cqBuff->length,
              //                              (unsigned char *)&(cqBuff->llcpdu_payload)
              //                            );
                pdma_ch2_bleTx_setting__aesccmEN0(cqBuff->ram_start_address + 0x0010); //0x0010: first 16 bytes for Session Key (SK)
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            else {
                aesccm_Tx_on_the_fly(pacl, write_iv,
                                             cqBuff->length,
                                              (unsigned char *)&(cqBuff->llcpdu_payload),
                                           cqBuff->ram_start_address
                                    );
            }
                pacl->txw4ack = 0x03; //0x03:sending LLC  , waiting ACK...
            if( cqBuff->llcpdu_payload.opcode == LL_02_TERMINATE_IND ) {
                pacl->flag1.field.txLLTerminateInd_w4ack = 0x01; //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
            }
        }
        else
        if( RINGBUF_isNonempty( pacl->lldataTxQ )  // non empty queue
            && ( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 1 )
            && ( pacl->flag1.field.stop_tx_dataPhyChannelPdu == 0 )
          )
        {
            uint16_t tdata_length ;
                hc_leacl_data_buffer_toair_TypeDef *pbuff;
                pbuff = (hc_leacl_data_buffer_toair_TypeDef *)( RINGBUF_peek( pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ) );
                tdata_length = pbuff->hci_hdr_length - pbuff->r_index ;
            if (tdata_length > pacl->connEffectiveMaxTxOctets) {
                tdata_length = pacl->connEffectiveMaxTxOctets; //segmentation to connEffectiveMaxTxOctets octets
            }
            if (aesccm_encrypt_en == 1 &&
                tdata_length < (pbuff->hci_hdr_length - pbuff->r_index) //have next segment
               )
            {
                tdata_length &= 0xFFFC; //to make next segment 4-aligned address
            }
            pbuff->r_idx_w4_acked = pbuff->r_index + tdata_length ;
            if(pbuff->air_pb_start)
            pacl->transmitHdr.field.transmitLLID       = 2;   // llid=0b10 L2CAP Start
            else
            pacl->transmitHdr.field.transmitLLID       = 1;   // llid=0b01 L2CAP Continue
          //pacl->transmitHdr.field.nextExpectedSeqNum
          //pacl->transmitHdr.field.transmitSeqNum
          //pacl->transmitHdr.field.md                 = set above
            pacl->transmitHdr.field.rfu                = 0;
            pacl->transmitHdr.field.transmitLength     = tdata_length;
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
            if (aesccm_encrypt_en == 0) {
              //pdma_ram_bleTx_write_payload(pbuff->ram_start_address + 0x0010, //0x0010: first 16 bytes for Session Key (SK)
              //                             pacl->transmitHdr.field.transmitLength,
              //                              (pbuff->octet + pbuff->r_index)
              //                            );
                pdma_ch2_bleTx_setting_l2cap__aesccmEN0(
                                             pbuff->ram_start_address + 0x0010 + pbuff->r_index, //0x0010: first 16 bytes for Session Key (SK)
                                             pacl->transmitHdr.field.transmitLength
                                            );
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            else {
                aesccm_Tx_on_the_fly        (pacl, write_iv,
                                             pacl->transmitHdr.field.transmitLength,
                                             pbuff->octet                      + pbuff->r_index,
                                             pbuff->ram_start_address + 0x0000 + pbuff->r_index
                                            );
            }
            pacl->txw4ack = 0x02; //0x02:sending L2CAP, waiting ACK...
        }
        else
        {
            pacl->transmitHdr.field.transmitLLID       = 1;   // Empty PDU: llid=0b01 L2CAP Continue, Length=0
          //pacl->transmitHdr.field.nextExpectedSeqNum
          //pacl->transmitHdr.field.transmitSeqNum
            pacl->transmitHdr.field.md                 = 0;//md=0: i have no further data <--- send empty
            pacl->transmitHdr.field.rfu                = 0;
            pacl->transmitHdr.field.transmitLength     = 0;
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
          //
            if (aesccm_encrypt_en == 1)
            {
                aesccm_Tx_on_the_fly(pacl, write_iv,
                                           0,//pacl->transmitHdr.field.transmitLength,
                                           0,//uint8_t *pch unused
                                           TX_CH0_SRCADDR_EMPTY_PDU + 0x0000 + 0
                                    );
            }
            else
          //
            {
                pdma_ch2_bleTx_setting__aesccmEN0(TX_CH0_SRCADDR_EMPTY_PDU);//send empty pdu: length=0, LLID=0b01:L2CAP Continuation
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            pacl->txw4ack = 0x01; //0x01:sending empty pdu: length=0, LLID=0b01:L2CAP Continuation
        }
    }
    else if(pacl->txw4ack == 0x01)  //0x01:sending empty pdu: length=0, LLID=0b01:L2CAP Continuation
    {
            pacl->transmitHdr.field.transmitLLID       = 1;   // Empty PDU: llid=0b01 L2CAP Continue, Length=0
          //pacl->transmitHdr.field.nextExpectedSeqNum
          //pacl->transmitHdr.field.transmitSeqNum
            pacl->transmitHdr.field.md                 = 0;//md=0: i have no further data <--- send empty
            pacl->transmitHdr.field.rfu                = 0;
            pacl->transmitHdr.field.transmitLength     = 0;
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
          //
            if (aesccm_encrypt_en == 1)
            {
                aesccm_Tx_on_the_fly(pacl, write_iv,
                                           0,//pacl->transmitHdr.field.transmitLength,
                                           0,//uint8_t *pch unused
                                           TX_CH0_SRCADDR_EMPTY_PDU + 0x0000 + 0
                                    );
            }
            else
          //
            {
                pdma_ch2_bleTx_setting__aesccmEN0(TX_CH0_SRCADDR_EMPTY_PDU);//send empty pdu: length=0, LLID=0b01:L2CAP Continuation
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
    }
    else if(pacl->txw4ack == 0x02)  //0x02:sending L2CAP, waiting ACK...
    {
          //pacl->transmitHdr.field.md                 = set above
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
                hc_leacl_data_buffer_toair_TypeDef *pbuff;
                pbuff = (hc_leacl_data_buffer_toair_TypeDef *)( RINGBUF_peek( pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ) );
            if (aesccm_encrypt_en == 0) {
              //pdma_ram_bleTx_write_payload(pbuff->ram_start_address + 0x0010, //0x0010: first 16 bytes for Session Key (SK)
              //                             pacl->transmitHdr.field.transmitLength,
              //                              (pbuff->octet + pbuff->r_index)
              //                            );
                pdma_ch2_bleTx_setting_l2cap__aesccmEN0(
                                             pbuff->ram_start_address + 0x0010 + pbuff->r_index, //0x0010: first 16 bytes for Session Key (SK)
                                             pacl->transmitHdr.field.transmitLength
                                            );
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            else {
                aesccm_Tx_on_the_fly        (pacl, write_iv,
                                             pacl->transmitHdr.field.transmitLength,
                                             pbuff->octet                      + pbuff->r_index,
                                             pbuff->ram_start_address + 0x0000 + pbuff->r_index
                                            );
            }
    }
    else if(pacl->txw4ack == 0x03)  //0x03:sending LLC  , waiting ACK...
    {
                llc_buffer_tx2air_TypeDef *cqBuff;
          //pacl->transmitHdr.field.md                 = set above
            RF_WT16(0x14, pacl->transmitHdr.reg);   //HB1 H_S0
                                                    //HB2 H_LENGTH
          //RF_WT08(0x16, 0x00);                    //HB3 H_S1  unused at ble
            if (pacl->flag1.field.current_llcTxQ_type == 0) {//0:llcTxQ, 1:llcTxQ_high
                cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ,      TOTAL_NUM_ELEMENTS_llcTxQ) );
            }
            else {
                cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high) );
            }
            if (aesccm_encrypt_en == 0) {
                if( cqBuff != 0 ) {
                  //pdma_ram_bleTx_write_payload(cqBuff->ram_start_address + 0x0010, //0x0010: first 16 bytes for Session Key (SK)
                  //                             pacl->transmitHdr.field.transmitLength,
                  //                              (unsigned char *)&(cqBuff->llcpdu_payload)
                  //                            );
                }
                pdma_ch2_bleTx_setting__aesccmEN0(cqBuff->ram_start_address + 0x0010); //0x0010: first 16 bytes for Session Key (SK)
                RF_WT08(0x33, 0x00 | MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
            }
            else {
                aesccm_Tx_on_the_fly(pacl, write_iv,
                                             pacl->transmitHdr.field.transmitLength,
                                              (unsigned char *)&(cqBuff->llcpdu_payload),
                                           cqBuff->ram_start_address
                                    );
            }
    }
}

/*********************************************************************************************************//**
 * @brief   This function handles EXTI interrupt.
 * @retval  None
 ************************************************************************************************************/
void irq1isr_ht32f87251_llfsm(LEACL_TypeDef *);

typedef union
{
    struct { 

        uint32_t irq08address_w_1eh_80_disabledtxen : 1;        // DISABLED_TXEN=1, DISABLED_RXEN=0   write at irq address
    	uint32_t irq08address_w_leh_40_disabledrxen : 1;        // DISABLED_TXEN=0, DISABLED_RXEN=1   write at irq address
        uint32_t irq08address_w_1eh_00 : 1;                     // DISABLED_TXEN=0, DISABLED_RXEN=0   write at irq address
        uint32_t irq04bcmatch_r_header : 1;
        uint32_t irq04bcmatch_rxPayload666_r_6octet : 1; //must: irq04bcmatch_r_header = 1
        uint32_t irq04bcmatch_rxPayloadEnd_r_6octet : 1; //must: irq04bcmatch_r_header = 1
        uint32_t irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen : 1;
        uint32_t irq04bcmatch_txPayloadEnd_w_1eh_00 : 1;
        uint32_t irq04bcmatch_w_1eh_80_if_rx_slave_md_1 : 1;    // DISABLED_TXEN=1, DISABLED_RXEN=0   write at irq bcmatch
        uint32_t irq80crcf_w_1eh_00 : 1;                        // DISABLED_TXEN=0, DISABLED_RXEN=0   write at irq crcf
        uint32_t irq20txds_w_33h_mask_00 : 1;                   // MASK=00                            write at irq txds
        uint32_t irq20txds_w_0bh_0ah_timeout_200us : 1;         // TIMEOUT= 200us                     write at irq txds
        uint32_t irq20txds_w_0dh_0ch_bccnt_10 : 1;              // BC_CNT=0x10  (2+0)*8: end header   write at irq txds
    } field;
    uint32_t reg;

} IRQ1ISR_TODO_TypeDef;
static volatile IRQ1ISR_TODO_TypeDef irq1isrtodo = { .reg=0 };
typedef union
{
    struct {
        uint16_t type   : 4;
        uint16_t __rfu  : 1;
        uint16_t chsel  : 1;
        uint16_t txadd  : 1;
        uint16_t rxadd  : 1;
        uint16_t length : 8;
    } field_adv;
    struct {
        uint16_t llid   : 2;
        uint16_t nesn   : 1;
        uint16_t sn     : 1;
        uint16_t md     : 1;
        uint16_t ___rfu : 3;
        uint16_t length : 8;
    } field_conn;
    uint16_t reg;

} MIX_PDU_HDR_TypeDef;
static volatile MIX_PDU_HDR_TypeDef irq1isr_r_hb_rvalue ;
static volatile uint16_t            irq1isr_bcmatch_caused_by_BCCNT;
static volatile _34h_IRQ_TypeDef    irq1isr_34h;
static volatile _E1h_IRQ_TypeDef    irq1isr_E1h;

extern volatile bool aes128_irq_occur;

void BC5602_IRQ1_ISR(void)
{
    irq1isr_34h.reg = RF_RD08(0x34); // IRQ1[7]CRCF, [6]RX_DR, [5]TX_DS, [4]MAX_RT, [3]ADDRESS, [2]BCMATCH, [1]RX_TO, [0]-
    //uart_putu8(  irq1isr_34h.reg);
    #if CAR_EN_IRQ1 == 0x00
    if (irq1isr_34h.reg)
    {
    	
        RF_WT08(0x34, irq1isr_34h.reg);
                                // IRQ1[7]CRCF     write 1 to clear bit
                                //     [6]RX_DR,   write 1 to clear bit
                                //     [5]TX_DS,   write 1 to clear bit
                                //     [4]MAX_RT,  write 1 to clear bit
                                //     [3]ADDRESS, write 1 to clear bit
                                //     [2]BCMATCH, write 1 to clear bit
                                //     [1]RX_TO,   write 1 to clear bit
                                //     [0]-
    }

    #endif
        irq1isr_E1h.reg = RF_RD08(0xE1);
        
    if (irq1isr_E1h.reg)
    {
        RF_WT08(0xE1, irq1isr_E1h.reg); //write 1 to clear flags
                                                    //E1h     [7:3]-
                                                    //        [2]  IRQ_ANCHOR  Anchor Point IRQ flag
                                                    //        [1]  IRQ_B_SET   BLE Setup    IRQ flag
                                                    //        [0]  IRQ_WK_UP   Wake UP      IRQ flag
        if (irq1isr_E1h.field.anchor) {
            RF_WT08_0xE0( E0h_10_EN_TEST_LO | 0x00);
                                                    //E0h   0 [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      0 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable
        }
    }
    if ( (AES_RD08(0x0C) & 0x01) == 0x01 )
    {
      //AES_WT08(0x0C, 0x01);
        aes128_irq_occur = 1;
    }
//uart_putu8(irq1isr_34h.reg); uart_putu8(irq1isr_E1h.reg); uart_puts(",");//debug
    
    schedluezzzz = 0;
    if( irq1isr_34h.field.address )   //[3]ADDRESS
    {


        if     ( irq1isrtodo.field.irq08address_w_1eh_80_disabledtxen ) {
            set_disabledTxen1_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
        else if( irq1isrtodo.field.irq08address_w_leh_40_disabledrxen ) {
            set_disabledTxen0_disabledRxen1();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
        else if( irq1isrtodo.field.irq08address_w_1eh_00 ) {
            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
    }
    if( irq1isr_34h.field.bcmatch )   //[2]BCMATCH
    {
        if     ( irq1isrtodo.field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen ) {
            set_disabledTxen0_disabledRxen1();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
        else if( irq1isrtodo.field.irq04bcmatch_txPayloadEnd_w_1eh_00 ) {
            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
        ////
         irq1isr_bcmatch_caused_by_BCCNT = written_BCCNT.reg;
      if(irq1isr_bcmatch_caused_by_BCCNT == BCCNT_0x10 ||              // end header
         irq1isr_bcmatch_caused_by_BCCNT == BCCNT_ENDHEADER_AESCCMON ) //+8:skip aesccm rx pdma get SessionKey at BC_CNT==16
      {
        if( irq1isrtodo.field.irq04bcmatch_r_header ) {
            irq1isrtodo.field.irq04bcmatch_r_header = 0;
            irq1isr_r_hb_rvalue.reg = RF_RD16(0x14);    //14h:HB1 H_S0, 15h:HB2 H_LENGTH
            if( irq1isrtodo.field.irq04bcmatch_w_1eh_80_if_rx_slave_md_1 ) {
                if( irq1isr_r_hb_rvalue.field_conn.md == 1 ) {  // header bit-4 MD MoreData  pacl->receivedHdr.field.md == 1
                    set_disabledTxen1_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                }
            }
            //
            if( irq1isrtodo.field.irq04bcmatch_rxPayload666_r_6octet ) {
                uint16_t bccntPayloadEnd;
                         bccntPayloadEnd = calc_bccntPayloadEnd_by_rxdlen(6);
                RF_WT16_0x0C(bccntPayloadEnd);      //0Ch BC_CNT[7:0]
                                                    //0Dh BC_CNT[15:8]
            }
            else
            if( irq1isrtodo.field.irq04bcmatch_rxPayloadEnd_r_6octet ) {
                uint16_t bccntPayloadEnd;
                         bccntPayloadEnd = calc_bccntPayloadEnd_by_rxdlen(irq1isr_r_hb_rvalue.field_adv.length);
                RF_WT16_0x0C(bccntPayloadEnd);      //0Ch BC_CNT[7:0]
                                                    //0Dh BC_CNT[15:8]
            }
        }
      }
    }
    if( irq1isr_34h.field.txds )     //[5]TX_DS
    {
        if( irq1isrtodo.field.irq20txds_w_33h_mask_00 ) {
            RF_WT08(0x33, 0x00);    //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
        }
        if( irq1isrtodo.field.irq20txds_w_0bh_0ah_timeout_200us ) {
            set_0bh_0ah_timeout_200us();
        }
        if( irq1isrtodo.field.irq20txds_w_0dh_0ch_bccnt_10 ) {
            RF_WT16_0x0C( BCCNT_0x10 );             //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh BC_CNT[15:8]
        }
    }
  //if( irq1isr_34h.field.rxdr )     //[6]RX_DR
  //{
  //}
    if( irq1isr_34h.field.crcf )     //[7]CRCF
    {
        if( irq1isrtodo.field.irq80crcf_w_1eh_00 ) {
            set_disabledTxen0_disabledRxen0();      //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        }
    }
    if( irq1isr_34h.field.rxto )     //[1]RX_TO
    {
        //Event cannot continue if RX_TO occur
            set_disabledTxen0_disabledRxen0();      //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
        //Event disable ANCHOR IRQ
        //Event begin with RX and result in RX_TO
            disable_anchor_IRQ_if_still_enabled();
    }
    ////////////////////////
    irq1isr_ht32f87251_llfsm(llfsm.pacl);
}

////////////////////////////////////////////////////////////
void set_mask00_bccnt10_timeout200us_for_ce_disabledRxen1_backtoRX(LEACL_TypeDef *pacl)
{
        RF_WT08(0x33, 0x00);        //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
    if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
        RF_WT16_0x0C( BCCNT_0x10 ); //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                    //0Dh  BC_CNT[15:8]
    }
    else {
        RF_WT16_0x0C( BCCNT_ENDHEADER_AESCCMON );//+8:skip aesccm rx pdma get SessionKey at BC_CNT==16
    }
        set_0bh_0ah_timeout_200us();//master connection event, or slave connection event
}

////////////////////////////////////////////////////////////
void irq1isr_rxllcpdu_pre_process(llc_buffer_rx_TypeDef *pBuff, LEACL_TypeDef *pacl)
{
        if (pBuff == 0) {
            return;
        }
    switch( pBuff->llcpdu_payload.opcode )
    {
    case 0x02:  //LL_02_TERMINATE_IND:
                if( pacl->receivedHdr.field.receivedLength == sizeof(llcpdu_02_terminate_ind_TypeDef) ) {
                    //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
                    //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
                    pacl->flag1.field.rxLLTerminateInd_txAck = 0x01;
                }
                break;
    case 0x03:  //LL_03_ENC_REQ:
                if( LEACL_is_role_slave(pacl) ) {
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 1;
                }
                break;
    case 0x05:  //LL_05_START_ENC_REQ:
                if( LEACL_is_role_master(pacl) ) {
                    //When Master LL receives LL_START_ENC_REQ it shall send LL_START_ENC_RSP encrypted
                    //                                      and shall be set up to receive an encrypted packet in response
                    pacl->flag1.field.Tx_aesccm_enabled = 1;//Master roger LL_05_START_ENC_REQ
                }
                break;
    case 0x06:  //LL_06_START_ENC_RSP:
                if( LEACL_is_role_slave(pacl) ) {
                    //When Slave LL receives LL_START_ENC_RSP it shall transmit LL_START_ENC_RSP encrypted
                    pacl->flag1.field.Tx_aesccm_enabled = 1;//Slave  roger LL_06_START_ENC_RSP
                }
                else {
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                break;
    case 0x0A:  //LL_0A_PAUSE_ENC_REQ:
                if( LEACL_is_role_slave(pacl) ) {
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 1;
                    //When Slave LL receives LL_PAUSE_ENC_REQ it shall send LL_PAUSE_ENC_RSP encrypted
                    //                                     and shall be set up to receive an unencrypted packet in response
                ////pacl->flag1.field.Tx_aesccm_enabled = 1;//Slave roger LL_0A_PAUSE_ENC_REQ
                  //pacl->flag1.field.Rx_aesccm_enabled = 0;//Slave roger LL_0A_PAUSE_ENC_REQ
                }
                break;
    case 0x0B:  //LL_0B_PAUSE_ENC_RSP:
                if( LEACL_is_role_master(pacl) ) {
                    //When Master LL receive LL_PAUSE_ENC_RSP it shall be set up to send and receive unencrypted
                    pacl->flag1.field.Tx_aesccm_enabled = 0;//Master roger LL_0B_PAUSE_ENC_RSP
                  //pacl->flag1.field.Rx_aesccm_enabled = 0;//Master roger LL_0B_PAUSE_ENC_RSP
                }
                else {
                    //When Slave LL receive LL_PAUSE_ENC_RSP it shall be set up to also send unencrypted
                    pacl->flag1.field.Tx_aesccm_enabled = 0;//Slave roger LL_0B_PAUSE_ENC_RSP
                  //pacl->flag1.field.Rx_aesccm_enabled = 0;//Slave roger LL_0B_PAUSE_ENC_RSP
                }
                break;
    case 0x0D:  //LL_0D_REJECT_IND:
                if( LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x03) ) { //LL_03_ENC_REQ
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                else
                if( LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x0A) ) { //LL_0A_PAUSE_ENC_REQ
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                break;
    case 0x11:  //LL_11_REJECT_EXT_IND:
                if( pBuff->llcpdu_payload.ctrData[0] == 0x03 ) { //LL_03_ENC_REQ         uint8_t rejectOpcode; //ctrData[0]
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                else
                if( pBuff->llcpdu_payload.ctrData[0] == 0x0A ) { //LL_0A_PAUSE_ENC_REQ   uint8_t rejectOpcode; //ctrData[0]
                    pacl->flag1.field.stop_tx_dataPhyChannelPdu = 0;
                }
                break;
    default:
                break;
    }
}

////////////////////////////////////////////////////////////
void irq1isr_ht32f87251_llfsm(LEACL_TypeDef *pacl)
{
    static uint8_t  advA_rx_scanind_advind[6];
    static uint8_t  initA_rx_connectind[6];
    switch(llfsm.state)
    {
    case LL_TRT_CLOSE_EVENT:
        break;
    case LL_RTR_CLOSE_EVENT:
        break;
    case LL_SLA_CLOSE_EVENT:
        break;
    case LL_MAS_CLOSE_EVENT:
        break;
    case LL_DTM_TX_CLOSE_EVENT:
        break;        
    case LL_DTM_RX_CLOSE_EVENT:
    	break;
    case LL_DTM_RX_RXDR_CTE_EVENT:
    	break;	
    case LL_ext_adv_CLOSE_EVENT:
    	break;	
    case LL_TRT_W4_IRQ_WAKEUP:
            if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_TRT_W4_IRQ_B_SET;
            }
        break;
    case LL_TRT_W4_IRQ_B_SET:
            if (irq1isr_E1h.field.b_set)
            {
              //moved to early ST_TRT_SET_T1, to get more time for RF cali
              {
                /*
                change_channel_index++; if(change_channel_index < 37) change_channel_index=37;
                                        if(change_channel_index > 39) change_channel_index=37;
change_channel_index=39;//debug
                    init_0rf_2in1_5602B_set_rfch(RfCenterFreq[change_channel_index]);
                    RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[change_channel_index]); // 2402 MHz BLE channel index 37=0x25= 1 10 0101 ==> 0x53
                */
                /*
                    init_0rf_2in1_5602B_set_rfch( RFCHANNEL );
                  #if   RFCHANNEL == 2
                    RF_WT08(0x49, PKT2_WHT_EN|0x53);            // 2402 MHz BLE channel index 37=0x25= 1 10 0101 ==> 0x53
                  #elif RFCHANNEL == 26
                    RF_WT08(0x49, PKT2_WHT_EN|0x33);            // 2426 MHz BLE channel index 38=0x26= 1 10 0110 ==> 0x33
                  #elif RFCHANNEL == 80
                    RF_WT08(0x49, PKT2_WHT_EN|0x73);            // 2480 MHz BLE channel index 39=0x27= 1 10 0111 ==> 0x73
                  #else
                    RF_WT08(0x49, PKT2_WHT_EN|PKT2_WHT_SEED);
                  #endif
                                                    //PKT2    [7]  WHT_EN, [6:0]WHTSD whitening seed
                */
              }
                write_00h_AccessAddress(P0_ACCESS_ADDR);
                write_50h_CRCInit_555555();
                RF_WT08(0x33, 0x04 | 0x08);         //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //04: [2]MASK_BCMATCH=1 to disable BCMATCH IRQ, no need at send adv_ind
                              //have to irq20txds_w_33h_mask_00 = 1 at llfsm.state = LL_TRT_T1
              //TIMEOUT[15:0] set by set_0bh_0ah_timeout_200us() following
              //RF_WT16(0x0A, (8-1));               //0Ah  TIMEOUT[7:0]  unit 25us  150us tifs + 8us preamble + 32us syncword
              //                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh  BC_CNT[15:8]
                clear_34h_irq1();
                pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1
                
            set_txHeader_TRT_T1_adv          (leconfig_AdvData.length);
            
            set_txPayload_TRT_T1_adv         (leconfig_bdaddr.le_public_AdvA, leconfig_AdvData.length,     (uint8_t *)&(leconfig_AdvData.advData        ));
            set_txPayload_TRT_T2_scanResponse(leconfig_bdaddr.le_public_AdvA, leconfig_ScanRspData.length, (uint8_t *)&(leconfig_ScanRspData.scanRspData));
                
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                RF_WT08_53h_MAX_RXPL( RXPL_EN | MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_TRT_EVENT | E0h_10_EN_TEST_LO | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable
                if( irq1isr_TRT_event_kind == 1 ) // 1:TRT event, 0:T1 only
                {
                    set_0bh_0ah_timeout_200us();        //TRT event R w4_connectind_or_scanreq   TIMEOUT[15:0] 200us
                    set_disabledTxen0_disabledRxen1();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                                            //Rxen1: TRT event R
                }
                else //irq1isr_TRT_event_kind == 0 // 1:TRT event, 0:T1 only
                {
                    set_0bh_0ah_timeout_200us();        //TRT event R w4_connectind_or_scanreq   TIMEOUT[15:0] 200us
                    set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                }
            #if   E0h_80_MASK_ANCHOR_AT_TRT_EVENT == 0x00
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_TRT_W4_IRQ_ANCHOR;
            #elif E0h_80_MASK_ANCHOR_AT_TRT_EVENT == 0x80
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                        .field.irq20txds_w_33h_mask_00 = 1,             // TRT event R, enable all IRQ
                ////done at LL_TRT_W4_IRQ_B_SET
                ////    .field.irq20txds_w_0dh_0ch_bccnt_10 = 1,        // TRT event R
                ////    .field.irq20txds_w_0bh_0ah_timeout_200us = 1,   // TRT event R
                    }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_TRT_T1;    pacl->flag1.field.roger_connectind = 0;
            #endif
                strobe_TX(); //send adv_ind
            }
        break;
    case LL_TRT_W4_IRQ_ANCHOR:
            if (irq1isr_E1h.field.anchor) {
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                        .field.irq20txds_w_33h_mask_00 = 1,             // TRT event R, enable all IRQ
                ////done at LL_TRT_W4_IRQ_B_SET
                ////    .field.irq20txds_w_0dh_0ch_bccnt_10 = 1,        // TRT event R
                ////    .field.irq20txds_w_0bh_0ah_timeout_200us = 1,   // TRT event R
                    }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_TRT_T1;    pacl->flag1.field.roger_connectind = 0;
            }
        break;
    case LL_TRT_T1:
            {
              //if( irq1isr_34h.field.address ) {
              //}
              //if( irq1isr_34h.field.bcmatch ) {
              //}
                if( irq1isr_34h.field.txds ) {
                    disable_anchor_IRQ_if_still_enabled(); // TRT event T1 txds, if event begin with disable ANCHOR IRQ
                    if( irq1isr_TRT_event_kind == 1 ) // 1:TRT event, 0:T1 only
                    {
                        pdma_ch2_bleRx_setting__aesccmEN0(CH2RXADDR_TRT_RX);           //TRT event R
                        {
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_r_header = 1,             ///TRT event R
                                .field.irq04bcmatch_rxPayloadEnd_r_6octet = 1,///TRT event R
                                .field.irq80crcf_w_1eh_00 = 1,                // TRT event R, result crcf cannot send scan_response
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        //strobe_RX() not need, HW auto dispatch RX by "DISABLED_RXEN=1"
                        llfsm.state = LL_TRT_RX;
                    }
                    else //irq1isr_TRT_event_kind == 0 // 1:TRT event, 0:T1 only
                    {
                        llfsm.state = LL_TRT_CLOSE_EVENT;
                    }
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_TRT_RX:
            {
                static volatile ADV_PDU_HDR_TypeDef r_req_advpdu_hdr;
                //
                if( irq1isr_34h.field.rxto ) {
                            llfsm.state = LL_TRT_CLOSE_EVENT; //rxto @ TRT event R w4_connectind_or_scanreq
                    break;
                }//[1]RX_TO
                if( irq1isr_34h.field.crcf ) {
                        if( greater_than_upperlimit_rxHeaderLength(r_req_advpdu_hdr.field.length) )
                        {
                            llfsm.state = LL_TRT_CLOSE_EVENT; //crcf @ TRT event R w4_connectind_or_scanreq
                        }
                        else
                        {
                            llfsm.state = LL_TRT_CLOSE_EVENT; //crcf @ TRT event R w4_connectind_or_scanreq
                        }
                    break;
                }//[7]CRCF
                if( irq1isr_34h.field.address ) {
                  /*moved from .address to .rxdr to read RF_RD32(0xF4) RF_RD16(0xF8)
                    //SW cannot disable ADDRESS IRQ @ LL_TRT_RX
                    //HW need           ADDRESS IRQ enabled to latch locnt/fine for  RF_RD32(0xF4) RF_RD16(0xF8)
                    pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS advertiser roger connect_ind
                    pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS advertiser roger connect_ind
                  */
                }//[3]ADDRESS
                if( irq1isr_34h.field.bcmatch ) {
                    if( irq1isr_bcmatch_caused_by_BCCNT == BCCNT_0x10 ) { //end header
                        r_req_advpdu_hdr.reg = irq1isr_r_hb_rvalue.reg; //HB1 H_S0, HB2 H_LENGTH
                        /*
                        if( r_req_advpdu_hdr.field.length < 6 ) // at least 6 octets
                        {
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_RTR_CLOSE_EVENT; //bcmatch length < 6 @ connect_ind/scan_req 
                        }
                        else
                        */
                        if( r_req_advpdu_hdr.field.type == PDUTYPE_SCAN_REQ 
                         && r_req_advpdu_hdr.field.length == 0x0C
                          )
                        {
                            set_disabledTxen1_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                            set_txHeader_TRT_T2_scanResponse(leconfig_ScanRspData.length);
                        }
                        else
                        if( r_req_advpdu_hdr.field.type == PDUTYPE_CONNECT_IND 
                         && r_req_advpdu_hdr.field.length == 0x22 // sizeof(CONN_IND_PAYLOAD_TypeDef)
                          )
                        {
                            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        }
                        else
                        {
                            // in case miss remote's syncword 8E89BED6
                            //      && roger other's syncword 8E89BED6 just before 625us RX_TO    (notice that use the same syncword 8E89BED6)
                            //abort rx, because valid only SCAN_REQ and CONNECT_IND
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_TRT_CLOSE_EVENT;
                        }
                    }
                    else //irq1isr_bcmatch_caused_by_BCCNT == end payload
                    {
                      #if   DV_ADDR_0_ADDRESS ==         0x39   //39h~3Eh
                        initA_rx_connectind[0] = RF_RD08(0x39); //device address
                        initA_rx_connectind[1] = RF_RD08(0x3A); //device address
                        initA_rx_connectind[2] = RF_RD08(0x3B); //device address
                        initA_rx_connectind[3] = RF_RD08(0x3C); //device address
                        initA_rx_connectind[4] = RF_RD08(0x3D); //device address
                        initA_rx_connectind[5] = RF_RD08(0x3E); //device address
                      #elif DV_ADDR_0_ADDRESS ==         0x3A   //3Ah~3Fh:2022oct11
                        uint16_t *pU16;
                        *(pU16 = (uint16_t *)(initA_rx_connectind+0)) = RF_RD16(0x3A); //device address
                        *(pU16 = (uint16_t *)(initA_rx_connectind+2)) = RF_RD16(0x3C); //device address
                        *(pU16 = (uint16_t *)(initA_rx_connectind+4)) = RF_RD16(0x3E); //device address
                      #endif
                    }
                }//[2]BCMATCH
                if( irq1isr_34h.field.rxdr )
                {
// BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
// BLE mode no need to check STATUS[0]RX_EMPTY  unused in BLE mode
                  //moved from .address to .rxdr to read RF_RD32(0xF4) RF_RD16(0xF8)
                    //SW cannot disable ADDRESS IRQ @ LL_TRT_RX
                    //HW need           ADDRESS IRQ enabled to latch locnt/fine for  RF_RD32(0xF4) RF_RD16(0xF8)
                    pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS advertiser roger connect_ind
                    pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS advertiser roger connect_ind
                  //
                        //////////////
                        if( r_req_advpdu_hdr.field.type == PDUTYPE_SCAN_REQ )
                        {
                            pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T2_SCANRSP);   //TRT event T2 scan_rsp
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                            uint16_t rsp_bccntPayloadEnd;
                                     rsp_bccntPayloadEnd = calc_bccntPayloadEnd_by_rxdlen(6+leconfig_ScanRspData.length);
                            RF_WT16_0x0C(rsp_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                               //0Dh BC_CNT[15:8]
                                IRQ1ISR_TODO_TypeDef zzz =
                                {
                                    .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1,   //TRT event T2 scan_rsp, then event is complete
                                }; irq1isrtodo.reg = zzz.reg;
                          //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                            irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_TRT_RX, r_req_advpdu_hdr.reg);//TRT event R read received scan_req
                            llfsm.state = LL_TRT_T2_SCANRSP;
                        }
                        else
                        if( r_req_advpdu_hdr.field.type == PDUTYPE_CONNECT_IND )
                        {
                            read_rxPayload_connectIND(pacl);
                            pacl->flag1.field.roger_connectind = 1;
                            llfsm.state = LL_TRT_CLOSE_EVENT; //rxdr @ TRT event R connectind
                        }
                        else
                        {
                            irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_TRT_RX, r_req_advpdu_hdr.reg);//TRT event R
                            llfsm.state = LL_TRT_CLOSE_EVENT;
                        }
                }//[6]RX_DR
                if( irq1isr_34h.field.txds ||
                    irq1isr_34h.field.maxrt ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_TRT_T2_SCANRSP:
            {
                if( irq1isr_34h.field.address ) {
                }
                if( irq1isr_34h.field.bcmatch ) {
                }
                if( irq1isr_34h.field.txds ) {
                    llfsm.state = LL_TRT_CLOSE_EVENT;
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_SLA_W4_IRQ_WAKEUP:
            if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_SLA_W4_IRQ_B_SET;
            }
        break;
    case LL_SLA_W4_IRQ_B_SET:
            if (irq1isr_E1h.field.b_set)
            {
              //moved to early ST_SLA_SETUP_TXWINDOW_SET_RX, ST_SLA_ANCHOR_SET_RX, to get more time for RF cali
              {
              /*
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
              */
              }
                RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[pacl->channelIndex]);   //PKT2    [7]  WHT_EN, [6:0]WHTSD whitening seed
                write_00h_AccessAddress(pacl->AA);
                write_50h_CRCInit((uint8_t *)pacl->crcInit+0);
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                RF_WT08_53h_MAX_RXPL( CONN_RXPL_EN | CONN_MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
            if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh BC_CNT[15:8]
            }
            else {
                RF_WT16_0x0C( BCCNT_ENDHEADER_AESCCMON );//+8:skip aesccm rx pdma get SessionKey at BC_CNT==16
            }
                set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                RF_WT08(0x33, 0x00);                //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                clear_34h_irq1();
                pacl->invalid_crc_consecutive_times_within_ce = 0;      //slave w4 conn ce anchor point
                pacl->roundtrip_times_in_a_ce = MAX_ROUNDTRIPS_CE;      //slave w4 conn ce anchor point
            if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
                pdma_ch2_bleRx_setting__aesccmEN0(RX_CH1_DESADDR);      //slave w4 conn ce anchor point
            }
            else {
                aesccm_Rx_on_the_fly(pacl,1);                           //slave w4 conn ce anchor point
            }
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_SLAVE_CE | E0h_10_EN_TEST_LO | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable
            #if   E0h_80_MASK_ANCHOR_AT_SLAVE_CE == 0x00
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_SLA_W4_IRQ_ANCHOR;
            #elif E0h_80_MASK_ANCHOR_AT_SLAVE_CE == 0x80
                IRQ1ISR_TODO_TypeDef zzz =
                {
                    .field.irq08address_w_1eh_80_disabledtxen = 1, // Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                    .field.irq04bcmatch_r_header = 1,              // Slave RX
                    .field.irq80crcf_w_1eh_00 = 0,             // =0: Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_SLA_RX_M2S;   first_roundtrip_in_a_ce = 1;
            #endif
                strobe_RX(); //slave
            }
        break;
    case LL_SLA_W4_IRQ_ANCHOR:
            if (irq1isr_E1h.field.anchor) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                    .field.irq08address_w_1eh_80_disabledtxen = 1, // Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                    .field.irq04bcmatch_r_header = 1,              // Slave RX
                    .field.irq80crcf_w_1eh_00 = 0,             // =0: Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_SLA_RX_M2S;   first_roundtrip_in_a_ce = 1;
            }
        break;
    case LL_SLA_RX_M2S:
            {
//debug
//if(irq1isr_34h==0x40 && pacl->receivedHdr.field.receivedLLID==3) irq1isr_34h=0x80;//[7]CRCF RX CRC check fail    debug, intentionally CRCF
                if( irq1isr_34h.field.rxto ) {
                    //spec: (1) Failure to receive a packet, or
                    //      (2) two consecutive packets received with an invalid CRC match
                    //      within a connection event, shall close the event
                            pacl->debug_consecutive_times_rxto ++;
                        if (pacl->debug_consecutive_times_rxto >= 5) {
                        }
                        if (first_roundtrip_in_a_ce) {
                            if ((pacl->pNewUpdate != 0) &&
                                (pacl->pNewUpdate->iSlave_updateState == 0x01) ) //not yet rx matched address
                            {
                                pacl->pNewUpdate->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger ++;
                            }
                            else {
                                pacl->iSlave_ce1stRx_lost_rxto_consecutive_times ++;
                            }
                        }
                        llfsm.state = LL_SLA_CLOSE_EVENT;
                    break;
                }//[1]RX_TO
                if( irq1isr_34h.field.crcf ) {
// todo: crc fail, then how to use ->receivedHdr.field.md
//pacl->receivedHdr.field.md=0;
                        pacl->invalid_crc_consecutive_times_within_ce ++; //slave rx m2s, result crc fail
                    if( greater_than_upperlimit_rxHeaderLength(pacl->receivedHdr.field.receivedLength) )
                    {
                        llfsm.state = LL_SLA_CLOSE_EVENT;
                    }
                    else
                    {
                        set_txHeader_txPayload_connectionEventPacket(pacl,0);//slave send s2m, @crcf
                                                                        //0:no need to write_iv in the same CE
                        {
                        //
                            uint16_t sce_bccntPayloadEnd;
                                     sce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                            RF_WT16_0x0C(sce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                               //0Dh BC_CNT[15:8]
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                        //
                        if( pacl->invalid_crc_consecutive_times_within_ce >= 2) {
                                //spec: (1) Failure to receive a packet, or
                                //      (2) two consecutive packets received with an invalid CRC match
                                //      within a connection event, shall close the event
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1               //after tx s2m,   no further roundtrip
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        else
                        if( pacl->transmitHdr.field.md == 1 || //spec: Slave should listen after sending its packet
                            pacl->receivedHdr.field.md == 1 ) {//spec: Slave should listen after sending its packet
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1  //after tx s2m, have further roundtrip
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        else {
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1               //after tx s2m,   no further roundtrip
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        }
                      //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                        llfsm.state = LL_SLA_TX_S2M;
                    }
                    break;
                }//[7]CRCF
                if( irq1isr_34h.field.address ) {
                    // Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                    {
                    //moved to .bcmatch
                    //header 16 bits only 8 us @ 2 Mbps DataRate
                    //                    8 us too short for ADDRESS ISR @ HCLK 32MHz 16MHz
                      /*
                        if (first_roundtrip_in_a_ce) {
                            pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS slave roger CE first(anchor) connPacket
                            pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS slave roger CE first(anchor) connPacket
                        }
                      */
                    }
                        disable_anchor_IRQ_if_still_enabled(); // Slave event, if event disable ANCHOR IRQ
                }//[3]ADDRESS
                if( irq1isr_34h.field.bcmatch ) {
                        //end header
                        pacl->receivedHdr.reg = irq1isr_r_hb_rvalue.reg;  //HB1 H_S0, HB2 H_Length    slave
                    {
                    //moved here .bcmatch from .address
                    //header 16 bits only 8 us @ 2 Mbps DataRate
                    //                    8 us too short for ADDRESS ISR @ HCLK 32MHz 16MHz
                      //
                        if (first_roundtrip_in_a_ce) {
                            pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS slave roger CE first(anchor) connPacket
                            pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS slave roger CE first(anchor) connPacket
                        }
                      //
                            pacl->iSlave_ce1stRx_lost_rxto_consecutive_times = 0;
                            pacl->debug_consecutive_times_rxto = 0;
                            pacl->flag1.field.conn_setup_success_syncword_match = 1; // slave recieve m2s
                            // The first packet received, regardless of a valid CRC match (i.e., only the access code matches)
                            //     in the Connection State by the slave determines the anchor point for the first connection event
                        if ((pacl->pNewUpdate != 0) &&
                            (pacl->pNewUpdate->iSlave_updateState == 0x01) ) //not yet rx matched address
                        {
                            pacl->currParam.interval = pacl->pNewUpdate->interval; //slave copy pNewUpdate->interval to currParam
                            pacl->currParam.latency  = pacl->pNewUpdate->latency ; //slave copy pNewUpdate->latency  to currParam
                            pacl->currParam.timeout  = pacl->pNewUpdate->timeout ; //slave copy pNewUpdate->timeout  to currParam
                                                       pacl->pNewUpdate->iSlave_updateState = 0x02; //already ever rx matched address
                            //Conn Update procedure is complete when the instant has passed, and the new CE parameters have been applied
                        }
                    }
                }//[2]BCMATCH
                if( irq1isr_34h.field.rxdr ) {
                        bool isnew;
// BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
// BLE mode no need to check STATUS[0]RX_EMPTY  unused in BLE mode
                        pacl->invalid_crc_consecutive_times_within_ce = 0;  //slave rx m2s, result rxdr
                        if( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 0 &&
                            pacl->receivedHdr.field.nesn == 1 )
                        {
                            // Slave shall be active in every CE until it receives a packet from Master with NESN set to 1.
                            //       From then on Slave may use "slave latency"
                            // LL/CON/ADV/BV-03-C [Master Missing Slave Packets]
                            pacl->flag1.field.conn_setup_success_roger_nesn_1 = 1; //slave rx m2s, result rxdr
                        }
                        if( pacl->transmitHdr.field.transmitSeqNum != pacl->receivedHdr.field.nesn) {
                            pacl->transmitHdr.field.transmitSeqNum ++ ; // transmitSeqNum ++
                            if (pacl->flag1.field.Tx_aesccm_enabled && (pacl->transmitHdr.field.transmitLength != 0)) { //encrypt non-zero length
                                pacl->TxPacketCounter.field.packetCounter ++;  //slave
                            }
                            if( pacl->txw4ack == 0x03)       //0x03:sending LLC  , waiting ACK...
                            {
                                llc_buffer_tx2air_TypeDef *cqBuff;
                                if( pacl->flag1.field.current_llcTxQ_type == 0 ) { //0:llcTxQ, 1:llcTxQ_high
                                    cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ) );
                                    llcBuffer_tx2air_free( cqBuff );
                                    RINGBUF_incrementPopIndex( pacl->llcTxQ );
                                }
                                else {
                                    cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high) );
                                    llcBuffer_tx2air_free( cqBuff );
                                    RINGBUF_incrementPopIndex( pacl->llcTxQ_high );
                                }
                                //
                                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x01 ) { //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
                                    pacl->flag1.field.txLLTerminateInd_w4ack  = 0x02;    //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                                }
                            }
                            else 
                            if( pacl->txw4ack == 0x02 )     //0x02:sending L2CAP, waiting ACK...
                            {
                                hc_leacl_data_buffer_toair_TypeDef *pBuffe;
                                if( RINGBUF_isNonempty( pacl->lldataTxQ ) ) // non empty queue
                                {
                                    pBuffe = (hc_leacl_data_buffer_toair_TypeDef *)( RINGBUF_peek( pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ) );
                                    pBuffe->air_pb_start=0;
                                    pBuffe->r_index = pBuffe->r_idx_w4_acked ; //update <--- ACKed
                                if( pBuffe->r_index >= pBuffe->hci_hdr_length )
                                {
                                    hcAclDataBuffer_toair_free( pBuffe );
                                    RINGBUF_incrementPopIndex( pacl->lldataTxQ ); // ridx ++   pop Q
                                    pacl->HcAclDataBUFF2air_finished_count[0] ++;
                                }
                                }
                            }
                            pacl->txw4ack = 0x00;//0x00:none
                        }
                        if( pacl->transmitHdr.field.nextExpectedSeqNum == pacl->receivedHdr.field.receivedSeqNum) {
                            if( pacl->receivedHdr.field.receivedLLID == 0 ) {
                                pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                isnew=1;
                            }
                            else
                            if( pacl->receivedHdr.field.receivedLLID == 3 ) { // 3: LL Control PDU
                                if( pacl->receivedHdr.field.receivedLength == 0 ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                }
                                else {
                                    if( irq1isr_llcBuffer_rx_isAvailable() ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                    }
                                    else {
                                        isnew=0;
                                    }
                                }
                            }
                            else { // 1,2: LL Data PDU
                                if( pacl->receivedHdr.field.receivedLength == 0 ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                }
                                else {
                                    if( irq1isr_hcAclDataBuffer_tohost_isAvailable() ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                    }
                                    else {
                                        isnew=0;
                                    }
                                }
                            }
                        }
                        else {
                            isnew=0;
                        }
                        ////first set s->m packet
                        if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x02 ) //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                        {
                            //LL/CON/SLA/BV-11-C [Slave sending termination]
                            //The initiating Link Layer shall send LL_02_TERMINATE_IND PDUs until an acknowledgment is received
                            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                            llfsm.state = LL_SLA_CLOSE_EVENT;
                        }
                        else
                        {
                            set_txHeader_txPayload_connectionEventPacket(pacl,0);//slave send s2m, @rxdr
                                                                            //0:no need to write_iv in the same CE
                            {
                            //
                                uint16_t sce_bccntPayloadEnd;
                                         sce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                                RF_WT16_0x0C(sce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                                   //0Dh BC_CNT[15:8]
                                RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                              //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                            //
                              //if( pacl->invalid_crc_consecutive_times_within_ce >= 2)
                              //{
                              //    never occur, here slave rxdr
                              //}
                              //else
                                if( pacl->transmitHdr.field.md == 1 || //spec: Slave should listen after sending its packet
                                    pacl->receivedHdr.field.md == 1 ) {//spec: Slave should listen after sending its packet
                                    IRQ1ISR_TODO_TypeDef zzz =
                                    {
                                        .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1  //after tx s2m, have further roundtrip
                                    }; irq1isrtodo.reg = zzz.reg;
                                }
                                else {
                                    IRQ1ISR_TODO_TypeDef zzz =
                                    {
                                        .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1               //after tx s2m,   no further roundtrip
                                    }; irq1isrtodo.reg = zzz.reg;
                                }
                            }
                          //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                            llfsm.state = LL_SLA_TX_S2M;
                        }
                        ////then read rxfifo
                        if (isnew == 1)
                        {
                                if( pacl->flag1.field.Rx_aesccm_enabled && (pacl->receivedHdr.field.receivedLength != 0) ) { //encrypt non-zero length
                                    pacl->RxPacketCounter.field.packetCounter ++;   //slave
                                }
                            if( pacl->receivedHdr.field.receivedLLID == 2 ||    //1,2: LL Data PDU
                                pacl->receivedHdr.field.receivedLLID == 1 )     //1,2: LL Data PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength != 0 ) {
                                    irq1isr_read_rxfifo_to_lldataRxQ(pacl);
                                }
                            }
                            else
                            if( pacl->receivedHdr.field.receivedLLID == 3 )     // 3: LL Control PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength != 0 ) {
                                    llc_buffer_rx_TypeDef *pBuff;
                                    pBuff = irq1isr_read_rxfifo_to_llcRxQ(pacl); // save received LLC PDU
                                    irq1isr_rxllcpdu_pre_process(pBuff, pacl);
                                }
                            }
                            else { // pacl->receivedHdr.field.receivedLLID == 0
                            }
                        }
                }//[6]RX_DR
                if( irq1isr_34h.field.txds ||
                    irq1isr_34h.field.maxrt ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_SLA_TX_S2M:
            {
                if( irq1isr_34h.field.address ) {
                        set_0bh_0ah_timeout_200us();            //slave connection event, R LL_SLA_RX_M2S   TIMEOUT[15:0] 200us   further roundtrip
                }
                if( irq1isr_34h.field.bcmatch ) { //irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen
                        set_mask00_bccnt10_timeout200us_for_ce_disabledRxen1_backtoRX(pacl);
                }
                if( irq1isr_34h.field.txds ) {
                    if( pacl->flag1.field.rxLLTerminateInd_txAck == 0x01 )
                    {
                        //LL/CON/SLA/BV-12-C [Slave accepting termination] 
                        set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        pacl->flag1.field.rxLLTerminateInd_txAck = 0x02;//0x02:finish send ACK for rx LL_02_TERMINATE_IND
                        llfsm.state = LL_SLA_CLOSE_EVENT;
                    }
                    else
                    if( disabledRxen_is_set_1() ) // slave CE use disabledRxen_is_set_1() to see already decided "yes or not continue event"
                    {
                      // DISABLED_RXEN == 1, LL_SLA_RX_M2S already decided to continue event CE further roundtrip
                        {
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq08address_w_1eh_80_disabledtxen = 1, // Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                                .field.irq04bcmatch_r_header = 1,              // Slave RX
                                .field.irq80crcf_w_1eh_00 = 0,             // =0: Slave RX syncword match, then even though CRC check fail, Slave still can send S->M packet
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
                            pdma_ch2_bleRx_setting__aesccmEN0(RX_CH1_DESADDR); //slave conn event, R LL_SLA_RX_M2S   further roundtrip
                        }
                        else {
                            aesccm_Rx_on_the_fly(pacl,0);                      //slave conn event, R LL_SLA_RX_M2S   further roundtrip
                                                    //0:no need to write_iv in the same CE
                        }
                    ////moved to if( irq1isr_34h.field.bcmatch ) // irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen
                    ////set_mask00_bccnt10_timeout200us_for_ce_disabledRxen1_backtoRX(pacl);
                      //strobe_RX() not need, HW auto dispatch RX by "DISABLED_RXEN=1"
                        llfsm.state = LL_SLA_RX_M2S;   first_roundtrip_in_a_ce = 0;
                    }
                    else
                    {
                      // DISABLED_RXEN == 0, LL_SLA_RX_M2S already decided to close event CE
                        llfsm.state = LL_SLA_CLOSE_EVENT;
                    }
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_RTR_TX_SCANREQ:
            {
                if( irq1isr_34h.field.address ) {
                }
                if( irq1isr_34h.field.bcmatch ) {
                }
                if( irq1isr_34h.field.txds ) {
                        pdma_ch2_bleRx_setting__aesccmEN0(CH2RXADDR_RTR_R2_SCANRSP);          //RTR event R2 scan_response
                        {
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_r_header = 1,   //RTR event R2 scan_response
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                        RF_WT08(0x33, 0x08);    //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                      //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ, no need at RTR R2 SCANRSP
                  //strobe_RX() not need, HW auto dispatch RX by "DISABLED_RXEN=1"
                    llfsm.state = LL_RTR_R2_SCANRSP;
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_RTR_R2_SCANRSP:
            {
                static volatile ADV_PDU_HDR_TypeDef header_rx_scanrsp;
                static          uint8_t             advA_rx_scanrsp[6];
                //
                if( irq1isr_34h.field.rxto ) {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //rxto @ RTR event R2 w4_scan_response
                    break;
                }
                if( irq1isr_34h.field.crcf ) {
                        if( greater_than_upperlimit_rxHeaderLength(header_rx_scanrsp.field.length) )
                        {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //crcf @ RTR event R2 w4_scan_response
                        }
                        else
                        {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //crcf @ RTR event R2 w4_scan_response
                        }
                    break;
                }//[7]CRCF
              //if( irq1isr_34h.field.address ) {  [3]MASK_ADDRESS=1 to disable ADDRESS IRQ, no need at RTR R2 SCANRSP
              //}
                if( irq1isr_34h.field.bcmatch ) {
                    if( irq1isr_bcmatch_caused_by_BCCNT == BCCNT_0x10 ) { //end header
                        header_rx_scanrsp.reg = irq1isr_r_hb_rvalue.reg; //HB1 H_S0, HB2 H_LENGTH
                        //
                        set_disabledTxen0_disabledRxen0();  //RTR event R2 scan_response, then event is complete
                        //
                        if( header_rx_scanrsp.field.length < 6 ) // at least AdvA 6 octets
                        {
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_RTR_CLOSE_EVENT; //bcmatch length < 6 @ RTR event R2 w4_scan_response 
                        }
                        else
                        //
                        if( header_rx_scanrsp.field.type != PDUTYPE_SCAN_RSP )
                        {
                            /* type wrong, abort rx
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_RTR_CLOSE_EVENT;
                            */
                        }
                    }
                }
                if( irq1isr_34h.field.rxdr ) {
// BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
// BLE mode no need to check STATUS[0]RX_EMPTY  unused in BLE mode
                      #if   DV_ADDR_0_ADDRESS ==     0x39   //39h~3Eh
                        advA_rx_scanrsp[0] = RF_RD08(0x39); //device address
                        advA_rx_scanrsp[1] = RF_RD08(0x3A); //device address
                        advA_rx_scanrsp[2] = RF_RD08(0x3B); //device address
                        advA_rx_scanrsp[3] = RF_RD08(0x3C); //device address
                        advA_rx_scanrsp[4] = RF_RD08(0x3D); //device address
                        advA_rx_scanrsp[5] = RF_RD08(0x3E); //device address
                      #elif DV_ADDR_0_ADDRESS ==     0x3A   //3Ah~3Fh:2022oct11
                        uint16_t *pU16;
                        *(pU16 = (uint16_t *)(advA_rx_scanrsp+0)) = RF_RD16(0x3A); //device address
                        *(pU16 = (uint16_t *)(advA_rx_scanrsp+2)) = RF_RD16(0x3C); //device address
                        *(pU16 = (uint16_t *)(advA_rx_scanrsp+4)) = RF_RD16(0x3E); //device address
                      #endif
                    if( header_rx_scanrsp.field.type == PDUTYPE_SCAN_RSP &&
                        advA_rx_scanrsp[0] == advA_rx_scanind_advind[0] &&
                        advA_rx_scanrsp[1] == advA_rx_scanind_advind[1] &&
                        advA_rx_scanrsp[2] == advA_rx_scanind_advind[2] &&
                        advA_rx_scanrsp[3] == advA_rx_scanind_advind[3] &&
                        advA_rx_scanrsp[4] == advA_rx_scanind_advind[4] &&
                        advA_rx_scanrsp[5] == advA_rx_scanind_advind[5] )
                    {
                        irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_RTR_R2_SCANRSP, header_rx_scanrsp.reg);//RTR event R2
                    }
                    else
                    {
irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_RTR_R2_SCANRSP, header_rx_scanrsp.reg);//debug   //RTR event R2
                    }
                        llfsm.state = LL_RTR_CLOSE_EVENT; //rxdr @ RTR event R2 w4_scan_response
                }//[6]RX_DR
                if( irq1isr_34h.field.txds ||
                    irq1isr_34h.field.maxrt ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_RTR_W4_IRQ_WAKEUP:
            if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_RTR_W4_IRQ_B_SET;
            }
        break;
    case LL_RTR_W4_IRQ_B_SET:
            if (irq1isr_E1h.field.b_set)
            {
              //moved to early ST_RTR_SET_R1, to get more time for RF cali
              {
                /*
                    init_0rf_2in1_5602B_set_rfch( RFCHANNEL );
                  #if   RFCHANNEL == 2
                    RF_WT08(0x49, PKT2_WHT_EN|0x53);            // 2402 MHz BLE channel index 37=0x25= 1 10 0101 ==> 0x53
                  #elif RFCHANNEL == 26
                    RF_WT08(0x49, PKT2_WHT_EN|0x33);            // 2426 MHz BLE channel index 38=0x26= 1 10 0110 ==> 0x33
                  #elif RFCHANNEL == 80
                    RF_WT08(0x49, PKT2_WHT_EN|0x73);            // 2480 MHz BLE channel index 39=0x27= 1 10 0111 ==> 0x73
                  #else
                    RF_WT08(0x49, PKT2_WHT_EN|PKT2_WHT_SEED);
                  #endif
                                                    //PKT2    [7]  WHT_EN, [6:0]WHTSD whitening seed
                */
              }
                write_00h_AccessAddress(P0_ACCESS_ADDR);
                write_50h_CRCInit_555555();
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                RF_WT08_53h_MAX_RXPL( RXPL_EN | MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
                // Scanner set rx adv_ind timeout
                RF_WT16(0x0A, 125000/25);           //0Ah  TIMEOUT[7:0]  unit 25us   maximum 0xFFFF * 25us = 65535 * 25us = 1638375us = 1.638 sec
                                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh  BC_CNT[15:8]
                set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                                       //1Eh=00
                                       //RTR event R1 syncword match, ISR assume NOT respond send scan_req/connect_ind
                                       //          background set DISABLED_TXEN=1 if need to send scan_req/connect_ind
                RF_WT08(0x33, 0x00);                //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                clear_34h_irq1();
                pdma_ch2_bleRx_setting__aesccmEN0(CH2RXADDR_RTR_R1);//RTR event R1
                
            set_txPayload_RTR_T_connectIND(pacl, leconfig_CreateConnection.peer_address+0);
            set_txPayload_RTR_T_scanREQ   (      leconfig_CreateConnection.peer_address+0);
                
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_RTR_EVENT | E0h_10_EN_TEST_LO | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable
            #if   E0h_80_MASK_ANCHOR_AT_RTR_EVENT == 0x00
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_RTR_W4_IRQ_ANCHOR;
            #elif E0h_80_MASK_ANCHOR_AT_RTR_EVENT == 0x80
                RF_WT08(0x33, 0x08);    //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ, no need at RTR R1
                IRQ1ISR_TODO_TypeDef zzz =
                {
                    .field.irq04bcmatch_r_header = 1,             ///RTR event R1
                    .field.irq04bcmatch_rxPayload666_r_6octet = 1,///RTR event R1
                //  .field.irq04bcmatch_rxPayloadEnd_r_6octet = 1,///RTR event R1
                    .field.irq80crcf_w_1eh_00 = 1,                // RTR event R1, result crcf cannot send scan_req/connect_ind
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_RTR_R1;    pacl->flag1.field.sent_connectind = 0;//debug
            #endif
                strobe_RX(); //scan
            }
        break;
    case LL_RTR_W4_IRQ_ANCHOR:
            if (irq1isr_E1h.field.anchor) {
                RF_WT08(0x33, 0x08);    //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ, no need at RTR R1
                IRQ1ISR_TODO_TypeDef zzz =
                {
                    .field.irq04bcmatch_r_header = 1,             ///RTR event R1
                    .field.irq04bcmatch_rxPayload666_r_6octet = 1,///RTR event R1
                //  .field.irq04bcmatch_rxPayloadEnd_r_6octet = 1,///RTR event R1
                    .field.irq80crcf_w_1eh_00 = 1,                // RTR event R1, result crcf cannot send scan_req/connect_ind
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_RTR_R1;    pacl->flag1.field.sent_connectind = 0;//debug
            }
        break;
    case LL_RTR_R1:
            {
                static volatile ADV_PDU_HDR_TypeDef header_rx_scanind_or_advind;
                //
                if( irq1isr_34h.field.rxto ) {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //rxto @ RTR event R1 w4_advind
                    break;
                }//[1]RX_TO
                if( irq1isr_34h.field.crcf ) {
//irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_RTR_R1, header_rx_scanind_or_advind.reg);//debug
                        if( greater_than_upperlimit_rxHeaderLength(header_rx_scanind_or_advind.field.length) )
                        {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //crcf @ RTR event R1 w4_advind
                        }
                        else
                        {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //crcf @ RTR event R1 w4_advind
                        }
                    break;
                }//[7]CRCF
              //if( irq1isr_34h.field.address ) {   [3]MASK_ADDRESS=1 to disable ADDRESS IRQ, no need at RTR R1
              //moved to .bcmatch     disable_anchor_IRQ_if_still_enabled(); // RTR event R1, if RTR event disable ANCHOR IRQ
              //}//[3]ADDRESS
                if( irq1isr_34h.field.bcmatch ) {
                            disable_anchor_IRQ_if_still_enabled(); // RTR event R1, if RTR event disable ANCHOR IRQ
                    if( irq1isr_bcmatch_caused_by_BCCNT == BCCNT_0x10 ) //end header
                    {
                            header_rx_scanind_or_advind.reg = irq1isr_r_hb_rvalue.reg; //HB1 H_S0, HB2 H_LENGTH
                        //
                        if( header_rx_scanind_or_advind.field.length < 6 ) // at least AdvA 6 octets
                        {
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_RTR_CLOSE_EVENT;
                        }
                        else
                        //
                        if( header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_IND ||
                            header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_SCAN_IND )
                        {
                            set_disabledTxen1_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        }
                        else
                        if( header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_NONCONN_IND )
                        {
                            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        }
                        else
                        {
                            //maybe receive scan_req...
                            //abort? no, keep rx to crc
                        }
                        //
                    }
                    else //irq1isr_bcmatch_caused_by_BCCNT == end payload
                    {
                        /*
                        if( header_rx_scanind_or_advind.field.length < 6 ) // at least AdvA 6 octets
                        {
                            set_abort_rx();
                            RF_WT16_0x0C( BCCNT_0x10 );         //0Ch BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                                //0Dh BC_CNT[15:8]
                            llfsm.state = LL_RTR_CLOSE_EVENT;
                        }
                        else
                        //
                        if( header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_IND ||
                            header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_SCAN_IND )
                        {
                            set_disabledTxen1_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        }
                        else
                        if( header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_NONCONN_IND )
                        {
                            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        }
                        else
                        {
                            //maybe receive scan_req...
                            //abort? no, keep rx to crc
                        }
                        */
                        /////////////////////
                      #if   DV_ADDR_0_ADDRESS ==            0x39   //39h~3Eh
                        advA_rx_scanind_advind[0] = RF_RD08(0x39); //device address
                        advA_rx_scanind_advind[1] = RF_RD08(0x3A); //device address
                        advA_rx_scanind_advind[2] = RF_RD08(0x3B); //device address
                        advA_rx_scanind_advind[3] = RF_RD08(0x3C); //device address
                        advA_rx_scanind_advind[4] = RF_RD08(0x3D); //device address
                        advA_rx_scanind_advind[5] = RF_RD08(0x3E); //device address
                      #elif DV_ADDR_0_ADDRESS ==            0x3A   //3Ah~3Fh:2022oct11
                        uint16_t *pU16;
                        *(pU16 = (uint16_t *)(advA_rx_scanind_advind+0)) = RF_RD16(0x3A); //device address
                        *(pU16 = (uint16_t *)(advA_rx_scanind_advind+2)) = RF_RD16(0x3C); //device address
                        *(pU16 = (uint16_t *)(advA_rx_scanind_advind+4)) = RF_RD16(0x3E); //device address
                      #endif
                    }
                }//[2]BCMATCH
                if( irq1isr_34h.field.rxdr ) {
                        if( (header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_IND) &&
                            (leconfig_ScanParam.le_scan_type == 0x01) //0x00: Passive Scanning. No scanning PDUs shall be sent (default)
                                                                      //0x01: Active scanning. Scanning PDUs may be sent
                          )
                        {
                        //
                        if( advA_rx_scanind_advind[0] == leconfig_CreateConnection.peer_address[0] &&
                            advA_rx_scanind_advind[1] == leconfig_CreateConnection.peer_address[1] &&
                            advA_rx_scanind_advind[2] == leconfig_CreateConnection.peer_address[2] &&
                            advA_rx_scanind_advind[3] == leconfig_CreateConnection.peer_address[3] &&
                            advA_rx_scanind_advind[4] == leconfig_CreateConnection.peer_address[4] &&
                            advA_rx_scanind_advind[5] == leconfig_CreateConnection.peer_address[5] )
                        //
                      //if(1) //debug
                        //
                        {
                        //
                            set_txHeader_RTR_T_connectIND();
                            pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_RTR_TX_CONNECTIND);        // RTR event T connect_ind
                            RF_WT08(0x33, 0x00); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                        // cannot disable ADDRESS IRQ @ LL_RTR_TX_CONNECTIND
                                        //HW need         ADDRESS IRQ enabled to latch locnt/fine for RF_RD32(0xF4) RF_RD16(0xF8)
                                        //SW then pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo =  RF_RD32(0xF4)
                                        //        pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase   =  RF_RD16(0xF8)
                            RF_WT16_0x0C( (2+0x22)*8 ); //0Ch BC_CNT[7:0]  (2+0x22)*8: BCMATCH end payload of connect_ind
                                                        //0Dh BC_CNT[15:8]    0x22: sizeof(CONN_IND_PAYLOAD_TypeDef)
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1,            // RTR event T connect_ind, then event is complete
                            }; irq1isrtodo.reg = zzz.reg;
                          //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                            llfsm.state = LL_RTR_TX_CONNECTIND;
                        /*
                            set_txHeader_RTR_T_scanREQ();
                            pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_RTR_TX_SCANREQ);           // RTR event T scan_req
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                            RF_WT16_0x0C( (2+12)*8 );       //0Ch BC_CNT[7:0]    (2+12)*8: next BCMATCH is end payload of scan_req
                                                            //0Dh BC_CNT[15:8]
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,   // RTR event R2 scan_rsp
                                .field.irq20txds_w_33h_mask_00 = 1,                 // RTR event R2 scan_rsp
                                .field.irq20txds_w_0dh_0ch_bccnt_10 = 1,            // RTR event R2 scan_rsp
                                .field.irq20txds_w_0bh_0ah_timeout_200us = 1,       // RTR event R2 scan_rsp
                            }; irq1isrtodo.reg = zzz.reg;
                          //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                            llfsm.state = LL_RTR_TX_SCANREQ ;
                        */
                        }
                        else
                        {
                            set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                            llfsm.state = LL_RTR_CLOSE_EVENT;
                        }
                        } ///////////////////////////////////end type PDUTYPE_ADV_IND
                        else
                        if( (header_rx_scanind_or_advind.field.type == PDUTYPE_ADV_SCAN_IND) &&
                            (leconfig_ScanParam.le_scan_type == 0x01) //0x00: Passive Scanning. No scanning PDUs shall be sent (default)
                                                                      //0x01: Active scanning. Scanning PDUs may be sent
                          )
                        {
                            set_txHeader_RTR_T_scanREQ();
                            pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_RTR_TX_SCANREQ);                    // RTR event T scan_req
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                            RF_WT16_0x0C( (2+12)*8 );       //0Ch BC_CNT[7:0]    (2+12)*8: next BCMATCH is end payload of scan_req
                                                            //0Dh BC_CNT[15:8]
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,   // RTR event R2 scan_rsp
                                .field.irq20txds_w_33h_mask_00 = 1,                 // RTR event R2 scan_rsp
                                .field.irq20txds_w_0dh_0ch_bccnt_10 = 1,            // RTR event R2 scan_rsp
                                .field.irq20txds_w_0bh_0ah_timeout_200us = 1,       // RTR event R2 scan_rsp
                            }; irq1isrtodo.reg = zzz.reg;
                          //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                            llfsm.state = LL_RTR_TX_SCANREQ ;
                        } ///////////////////////////////////end type PDUTYPE_ADV_SCAN_IND
                        else
                        {
                            llfsm.state = LL_RTR_CLOSE_EVENT; //rxdr @ RTR event R1 w4_advind, pkt type not used
                        }
                    //
// BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
// BLE mode no need to check STATUS[0]RX_EMPTY  unused in BLE mode
                        irq1isr_read_rxfifo_to_advRxQ(CH2RXADDR_RTR_R1, header_rx_scanind_or_advind.reg);//RTR event R1
                }//[6]RX_DR
                if( irq1isr_34h.field.txds ||
                    irq1isr_34h.field.maxrt ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_RTR_TX_CONNECTIND:
            {
              //if( irq1isr_34h.field.address ) {
              //}
              //if( irq1isr_34h.field.bcmatch ) {
              //}
                if( irq1isr_34h.field.txds ) {
                    //SW cannot disable ADDRESS IRQ @ LL_RTR_TX_CONNECTIND
                    //HW need           ADDRESS IRQ enabled to latch locnt/fine for  RF_RD32(0xF4) RF_RD16(0xF8)
                    //SW then read, ok read later at .txds
                    pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS Scanner send connect_ind
                    pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS Scanner send connect_ind
pacl->connEventCount=0xFFFF; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
//pacl->connEventCount=0xC000; //debug the ReferenceConnEventCount and Instant are on different sides of the eventCount wraparound
pacl->lastUnmappedChIndex=0;
pacl->transmitHdr.field.transmitSeqNum=0;
pacl->transmitHdr.field.nextExpectedSeqNum=0;
pacl->txw4ack=0x00;//0x00:none
pacl->flag1.field.conn_setup_success_syncword_match = 0; //master  todo:spec master send connectind, then can send send_04_hciEvent_3e_01_LE_connection_complete()
pacl->flag1.field.conn_setup_success_roger_nesn_1 = 0;   //master
                    calc_usedChannel_byRemapIndex(&(pacl->currChM));
                    //
                    pacl->flag1.field.sent_connectind = 1;//debug
                    llfsm.state = LL_RTR_CLOSE_EVENT;
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_MAS_W4_IRQ_WAKEUP:
            if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_MAS_W4_IRQ_B_SET;
            }
        break;
    case LL_MAS_W4_IRQ_B_SET:
            if (irq1isr_E1h.field.b_set)
            {
              //moved to early ST_MAS_SETUP_TXWINDOW_SET_TX, ST_MAS_ANCHOR_SET_TX, to get more time for RF cali
              {
              /*
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
              */
              }
                RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[pacl->channelIndex]);   //PKT2    [7]  WHT_EN, [6:0]WHTSD whitening seed
                write_00h_AccessAddress(pacl->AA);
                write_50h_CRCInit((uint8_t *)pacl->crcInit+0);
                pacl->invalid_crc_consecutive_times_within_ce = 0;      //master send m2s ce anchor point
                pacl->roundtrip_times_in_a_ce = MAX_ROUNDTRIPS_CE;      //master send m2s ce anchor point
                set_txHeader_txPayload_connectionEventPacket(pacl,1);   //master send m2s ce anchor point
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                RF_WT08_53h_MAX_RXPL( CONN_RXPL_EN | CONN_MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
              //TIMEOUT[15:0] set by set_0bh_0ah_timeout_200us() @ LL_MAS_TX_M2S
              //RF_WT16(0x0A, (8-1));               //0Ah  TIMEOUT[7:0]  unit 25us  150us tifs + 8us preamble + 32us syncword
              //                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                uint16_t mce_bccntPayloadEnd;
                         mce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                RF_WT16_0x0C(mce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                   //0Dh BC_CNT[15:8]
                clear_34h_irq1();
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_MASTER_CE | E0h_10_EN_TEST_LO | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable
                set_0bh_0ah_timeout_200us();        // master CE event rx s2m
                set_disabledTxen0_disabledRxen1();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                                // master RX s2m  after send m2s
            #if   E0h_80_MASK_ANCHOR_AT_MASTER_CE == 0x00
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_MAS_W4_IRQ_ANCHOR;
            #elif E0h_80_MASK_ANCHOR_AT_MASTER_CE == 0x80
                IRQ1ISR_TODO_TypeDef zzz =
                {
                   .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,// master CE event rx s2m
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_MAS_TX_M2S;   first_roundtrip_in_a_ce = 1;
            #endif
                strobe_TX(); //master send m2s
            }
        break;
    case LL_MAS_W4_IRQ_ANCHOR:
            if (irq1isr_E1h.field.anchor) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                   .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,// master CE event rx s2m
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_MAS_TX_M2S;   first_roundtrip_in_a_ce = 1;
            }
        break;
    case LL_MAS_TX_M2S:
            {
              //if( irq1isr_34h.field.address ) {
              //}
                if( irq1isr_34h.field.bcmatch ) { // irq1isrtodo.field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen
                    if (first_roundtrip_in_a_ce) {
                        pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.field.cntlo = RF_RD32(0xF4); //F4h~F7h: CNT_LO_ADDR     irq[3]ADDRESS master send CE first(anchor) connPacket
                        pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase   = RF_RD16(0xF8); //F8h~F9h: CNT_LO_1US_ADDR irq[3]ADDRESS master send CE first(anchor) connPacket
                    }
                        set_mask00_bccnt10_timeout200us_for_ce_disabledRxen1_backtoRX(pacl);
                }
                if( irq1isr_34h.field.txds ) {
                    disable_anchor_IRQ_if_still_enabled(); // master CE event txds, if event begin with disable ANCHOR IRQ
                    if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
                        pdma_ch2_bleRx_setting__aesccmEN0(RX_CH1_DESADDR);  //master CE event, R LL_MAS_RX_S2M
                    }
                    else {
                        aesccm_Rx_on_the_fly(pacl,0);                       //master CE event, R LL_MAS_RX_S2M
                                                //0:no need to write_iv in the same CE
                    }
                    ////////
                    if( pacl->flag1.field.rxLLTerminateInd_txAck == 0x01 )
                    {
                        //LL/CON/MAS/BV-09-C [Master Accepting Termination] 
                        set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        pacl->flag1.field.rxLLTerminateInd_txAck = 0x02;//0x02:finish send ACK for rx LL_02_TERMINATE_IND
                        llfsm.state = LL_MAS_CLOSE_EVENT;
                    }
                    else
                    if( pacl->invalid_crc_consecutive_times_within_ce >= 2 )
                    {
                            //spec: (1) Failure to receive a packet, or
                            //      (2) two consecutive packets received with an invalid CRC match
                            //      within a connection event, shall close the event
                            //5602b master must finish the dummy sending, 
                            //because cannot stop HW running [7]DISABLED_TXEN=1
                            llfsm.state = LL_MAS_CLOSE_EVENT;
                    }
                    else
                    {
                        if( pacl->transmitHdr.field.md == 1 )
                            ////->receivedHdr.field.md == 0 ) //slave's md unknown here, known later at LL_MAS_RX_S2M irq1 bcmatch
                        {
                          //spec: Master may continue the connection event
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq08address_w_1eh_80_disabledtxen = 1,      //Master RX
                                .field.irq04bcmatch_r_header = 1,                   //Master RX
                            };
                            if( pacl->invalid_crc_consecutive_times_within_ce == (2-1) ) {
                                zzz.field.irq80crcf_w_1eh_00 = 1; //if   next rx result in crcf --> occur 2 consecutive crcf
                                                                  //then must close event
                                                                  //     within the switching time DLY_TXS + DLY_TIFS
                                                                  //                    total time DLY_TXS + DLY_TIFS + L2BTXSET, where L2BTXSET is fixed (DLY_VCOCAL + CalibrationTime) but cannot set CalibrationTime by a control register
                                                                  //     can reset DISABLED_TXEN=0 to disable following TX
                            }
                                irq1isrtodo.reg = zzz.reg;
                        }
                        else //pacl->transmitHdr.field.md == 0
                        {
                          //spec: Master shall not send another packet, closing the connection event
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq08address_w_1eh_00 = 1,                   //Master RX
                                .field.irq04bcmatch_r_header = 1,                   //Master RX
                                .field.irq04bcmatch_w_1eh_80_if_rx_slave_md_1 = 1,  //Master RX, slave's md unknown here, known later at LL_MAS_RX_S2M irq1 bcmatch
                            };
                            if( pacl->invalid_crc_consecutive_times_within_ce == (2-1) ) {
                                zzz.field.irq80crcf_w_1eh_00 = 1; //if   next rx result in crcf --> occur 2 consecutive crcf
                                                                  //then must close event
                                                                  //     within the switching time DLY_TXS + DLY_TIFS
                                                                  //                    total time DLY_TXS + DLY_TIFS + L2BTXSET, where L2BTXSET is fixed (DLY_VCOCAL + CalibrationTime) but cannot set CalibrationTime by a control register
                                                                  //     can reset DISABLED_TXEN=0 to disable following TX
                            }
                                irq1isrtodo.reg = zzz.reg;
                        }
                        ////moved to if( irq1isr_34h.field.bcmatch ) // irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen
                        ////set_mask00_bccnt10_timeout200us_for_ce_disabledRxen1_backtoRX(pacl);
                          //strobe_RX() not need, HW auto dispatch RX by "DISABLED_RXEN=1"
                            llfsm.state = LL_MAS_RX_S2M;
                    }
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
    case LL_MAS_RX_S2M:
            {
                if( irq1isr_34h.field.rxto ) {
                    //spec: (1) Failure to receive a packet, or
                    //      (2) two consecutive packets received with an invalid CRC match
                    //      within a connection event, shall close the event
                            pacl->debug_consecutive_times_rxto ++;
                        if( pacl->debug_consecutive_times_rxto >= 5 ) {
                        }
                        llfsm.state = LL_MAS_CLOSE_EVENT;
                    break;
                }//[1]RX_TO
                if( irq1isr_34h.field.crcf ) {
// todo: crc fail, then how to use ->receivedHdr.field.md
//pacl->receivedHdr.field.md=0;
                        pacl->invalid_crc_consecutive_times_within_ce ++; //master CE event rx s2m, result crc fail
                    if( greater_than_upperlimit_rxHeaderLength(pacl->receivedHdr.field.receivedLength) )
                    {
                        llfsm.state = LL_MAS_CLOSE_EVENT;
                    }
                    else
                    if( disabledTxen_is_set_1() )   // master final set DISABLED_TXEN = 1  at BCMATCH IRQ
                                                    // irq04bcmatch_w_1eh_80_if_rx_slave_md_1 = 1, //slave's md unknown here, known later at LL_MAS_RX_S2M irq1 bcmatch
                    {
                      // DISABLED_TXEN == 1, already decided to continue event CE further roundtrip
                      if( pacl->invalid_crc_consecutive_times_within_ce >= 2 )
                      {
                        //spec: (1) Failure to receive a packet, or
                        //      (2) two consecutive packets received with an invalid CRC match
                        //      within a connection event, shall close the event
                        //5602b master must finish the dummy sending, 
                        //because cannot stop HW running [7]DISABLED_TXEN=1
                        set_txHeader_txPayload_connectionEventPacket(pacl,0);//master send m2s
                                                                        //0:no need to write_iv in the same CE
                        {
                        //
                            uint16_t mce_bccntPayloadEnd;
                                     mce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                            RF_WT16_0x0C(mce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                               //0Dh BC_CNT[15:8]
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                        //
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_00 = 1,
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                      //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                        llfsm.state = LL_MAS_TX_M2S;   first_roundtrip_in_a_ce = 0;
                      }
                      else
                      {
                        set_txHeader_txPayload_connectionEventPacket(pacl,0);//master send m2s
                                                                        //0:no need to write_iv in the same CE
                        {
                        //
                            uint16_t mce_bccntPayloadEnd;
                                     mce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                            RF_WT16_0x0C(mce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                               //0Dh BC_CNT[15:8]
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                        //
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,   // master CE event rx s2m
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                      //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                        llfsm.state = LL_MAS_TX_M2S;   first_roundtrip_in_a_ce = 0;
                      }
                    }
                    else
                    {
                      // DISABLED_TXEN == 0, already decided to close event CE

//if( pacl->invalid_crc_consecutive_times_within_ce >= 2 )
//debug_RFLA_pulse_to_trigger_LA(); //debug
                        llfsm.state = LL_MAS_CLOSE_EVENT;
                    }
                    break;
                }//[7]CRCF
                if( irq1isr_34h.field.address ) {
                            pacl->flag1.field.conn_setup_success_syncword_match = 1; // master recieve s2m
                            pacl->debug_consecutive_times_rxto = 0;
                }//[3]ADDRESS
                if( irq1isr_34h.field.bcmatch ) {
                        //end header
                        pacl->receivedHdr.reg = irq1isr_r_hb_rvalue.reg;  //HB1 H_S0, HB2 H_LENGTH     master
                }//[2]BCMATCH
                if( irq1isr_34h.field.rxdr ) {
                        bool isnew;
// BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
// BLE mode no need to check STATUS[0]RX_EMPTY  unused in BLE mode
                        pacl->invalid_crc_consecutive_times_within_ce = 0;  //master CE event rx s2m, result rxdr
                        if( pacl->flag1.field.conn_setup_success_roger_nesn_1 == 0 &&
                            pacl->receivedHdr.field.nesn == 1 )
                        {
                            pacl->flag1.field.conn_setup_success_roger_nesn_1 = 1; //master CE event rx s2m, result rxdr
                        }
                        if( pacl->transmitHdr.field.transmitSeqNum != pacl->receivedHdr.field.nesn) {
                            pacl->transmitHdr.field.transmitSeqNum ++ ; // transmitSeqNum ++
                            if (pacl->flag1.field.Tx_aesccm_enabled && (pacl->transmitHdr.field.transmitLength != 0)) { //encrypt non-zero length
                                pacl->TxPacketCounter.field.packetCounter ++;  //master
                            }
                            if( pacl->txw4ack == 0x03)       //0x03:sending LLC  , waiting ACK...
                            {
                                llc_buffer_tx2air_TypeDef *cqBuff;
                                if( pacl->flag1.field.current_llcTxQ_type == 0 ) { //0:llcTxQ, 1:llcTxQ_high
                                    cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ) );
                                    llcBuffer_tx2air_free( cqBuff );
                                    RINGBUF_incrementPopIndex( pacl->llcTxQ );
                                }
                                else {
                                    cqBuff = (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek(pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high) );
                                    llcBuffer_tx2air_free( cqBuff );
                                    RINGBUF_incrementPopIndex( pacl->llcTxQ_high );
                                }
                                //
                                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x01 ) { //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
                                    pacl->flag1.field.txLLTerminateInd_w4ack  = 0x02;    //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                                }
                            }
                            else 
                            if( pacl->txw4ack == 0x02 )     //0x02:sending L2CAP, waiting ACK...
                            {
                                hc_leacl_data_buffer_toair_TypeDef *pBuffe;
                                if( RINGBUF_isNonempty( pacl->lldataTxQ ) ) // non empty queue
                                {
                                    pBuffe = (hc_leacl_data_buffer_toair_TypeDef *)( RINGBUF_peek( pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ) );
                                    pBuffe->air_pb_start=0;
                                    pBuffe->r_index = pBuffe->r_idx_w4_acked ; //update <--- ACKed
                                if( pBuffe->r_index >= pBuffe->hci_hdr_length )
                                {
                                    hcAclDataBuffer_toair_free( pBuffe );
                                    RINGBUF_incrementPopIndex( pacl->lldataTxQ ); // ridx ++   pop Q
                                    pacl->HcAclDataBUFF2air_finished_count[0] ++;
                                }
                                }
                            }
                            pacl->txw4ack = 0x00;//0x00:none
                        }
                        if( pacl->transmitHdr.field.nextExpectedSeqNum == pacl->receivedHdr.field.receivedSeqNum) {
                            if( pacl->receivedHdr.field.receivedLLID == 2 ||    //1,2: LL Data PDU
                                pacl->receivedHdr.field.receivedLLID == 1 )     //1,2: LL Data PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength == 0 ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                }
                                else {
                                    if( irq1isr_hcAclDataBuffer_tohost_isAvailable() ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                    }
                                    else {
                                        isnew=0;
                                    }
                                }
                            }
                            else
                            if( pacl->receivedHdr.field.receivedLLID == 3 )     // 3: LL Control PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength == 0 ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                }
                                else {
                                    if( irq1isr_llcBuffer_rx_isAvailable() ) {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                                    }
                                    else {
                                        isnew=0;
                                    }
                                }
                            }
                            else     // pacl->receivedHdr.field.receivedLLID == 0
                            {
                                        pacl->transmitHdr.field.nextExpectedSeqNum++; // nextExpectedSeqNum ++
                                        isnew=1;
                            }
                        }
                        else {
                            isnew=0;
                        }
                    ////////////////////
                    if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x02 ) //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                    {
                        //LL/CON/MAS/BV-08-C [Master Sending Termination]
                        //The initiating Link Layer shall send LL_02_TERMINATE_IND PDUs until an acknowledgment is received
                        set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
                        llfsm.state = LL_MAS_CLOSE_EVENT;
                    }
                    else
                    if( disabledTxen_is_set_1() )   // master final set DISABLED_TXEN = 1  at BCMATCH IRQ
                                                    // irq04bcmatch_w_1eh_80_if_rx_slave_md_1 = 1, //slave's md unknown here, known later at LL_MAS_RX_S2M irq1 bcmatch
                    {
                      // DISABLED_TXEN == 1, already decided to continue event CE further roundtrip
                        set_txHeader_txPayload_connectionEventPacket(pacl,0);//master send m2s
                                                                        //0:no need to write_iv in the same CE
                        {
                        //
                            uint16_t mce_bccntPayloadEnd;
                                     mce_bccntPayloadEnd = calc_bccntPayloadEnd_by_pacl_transmitLength(pacl);
                            RF_WT16_0x0C(mce_bccntPayloadEnd); //0Ch BC_CNT[7:0]
                                                               //0Dh BC_CNT[15:8]
                            RF_WT08(0x33, 0x08); //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                                          //08: [3]MASK_ADDRESS=1 to disable ADDRESS IRQ
                        //
                            IRQ1ISR_TODO_TypeDef zzz =
                            {
                                .field.irq04bcmatch_txPayloadEnd_w_1eh_40_disabledrxen = 1,   // master CE event rx s2m
                            }; irq1isrtodo.reg = zzz.reg;
                        }
                      //strobe_TX() not need, 150us later, HW auto dispatch TX by "DISABLED_TXEN=1"
                        llfsm.state = LL_MAS_TX_M2S;   first_roundtrip_in_a_ce = 0;
                    }
                    else
                    {
                      // DISABLED_TXEN == 0, already decided to close event CE
                        llfsm.state = LL_MAS_CLOSE_EVENT; // todo debug: two consecutive packets received with an invalid CRC match
                    }
                    ////////////////////
                        if (isnew == 1)
                        {
                                if( pacl->flag1.field.Rx_aesccm_enabled && (pacl->receivedHdr.field.receivedLength != 0) ) { //encrypt non-zero length
                                    pacl->RxPacketCounter.field.packetCounter ++;   //master
                                }
                            if( pacl->receivedHdr.field.receivedLLID == 2 ||    //1,2: LL Data PDU
                                pacl->receivedHdr.field.receivedLLID == 1 )     //1,2: LL Data PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength != 0 ) {
                                    irq1isr_read_rxfifo_to_lldataRxQ(pacl);
                                }
                            }
                            else
                            if( pacl->receivedHdr.field.receivedLLID == 3 )     // 3: LL Control PDU
                            {
                                if( pacl->receivedHdr.field.receivedLength != 0 ) {
                                    llc_buffer_rx_TypeDef *pBuff;
                                    pBuff = irq1isr_read_rxfifo_to_llcRxQ(pacl); // save received LLC PDU
                                    irq1isr_rxllcpdu_pre_process(pBuff, pacl);
                                }
                            }
                            else { // pacl->receivedHdr.field.receivedLLID == 0
                            }
                        }
                }//[6]RX_DR
                if( irq1isr_34h.field.txds ||
                    irq1isr_34h.field.maxrt ) {
err_print____(irq1isr_34h.reg);
                }
            }
        break;
       case LL_DTM_TX_W4_IRQ_ANCHOR:
            {
                if (irq1isr_E1h.field.anchor) {                	                                                
                    IRQ1ISR_TODO_TypeDef zzz =
                  {
                  }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_DTM_TX_ADVERTISING;
               }
            }
        break;   
       case LL_DTM_TX_ADVERTISING:
       	    {
                if( irq1isr_34h.field.address ) {              }
              //  if( irq1isr_34h.field.bcmatch ) {                }
                if( irq1isr_34h.field.txds ) {
                        llfsm.state = LL_DTM_TX_CLOSE_EVENT;
                }
                if( irq1isr_34h.field.rxto  ||
                    irq1isr_34h.field.maxrt ||
                    irq1isr_34h.field.rxdr  ||
                    irq1isr_34h.field.crcf ) {
			err_print____(irq1isr_34h.reg);
                }
        break;       	    	
            }     
       case LL_DTM_RX_SCAN:
           {
           	unsigned char debug_rssi_id_ok_min;
           	unsigned char debug_rssi_id_ok_max;
           	unsigned char debug_rssi_gain_sel_min;
           	unsigned char debug_rssi_gain_sel_max;
                    //uart_puts("rx_scan");
                if( irq1isr_34h.field.rxto ) {
                	debug_RFLA_pulse_to_trigger_LA();
                	debug_RFLA_pulse_to_trigger_LA();
   			debug_RFLA_pulse_to_trigger_LA();
    			debug_RFLA_pulse_to_trigger_LA();
                    llfsm.state = LL_DTM_RX_CLOSE_EVENT; //rxto @ W4_ADVIND
                    //uart_puts("rxto");
                    break;
                }//[1]RX_TO
                if( irq1isr_34h.field.crcf ) { 
                	
                   // debug_RFLA_pulse_to_trigger_LA(); 
                    debug_leTestEnd_evt.num_crcf++;                 	                      	 
                    //uart_putchar(0x88); 
		//irq1isr_read_rxfifo_to_advRxQ(0x0200, header_rx_scanind_or_advind.reg);//debug
		                   // uart_puts("crcf");
		    llfsm.state = LL_DTM_RX_CLOSE_EVENT;
                    break;
                }//[7]CRCF
                if( irq1isr_34h.field.address ) {
                	//debug_RFLA_pulse_to_trigger_LA();
                	
                	#if Debug_COUNT_CRCF == 2
                	debug_rssi_id_ok[debug_rssi_count_i] = RF_RD08(0x48);
                	if(debug_rssi_id_ok[debug_rssi_count_i]>0x60){debug_RFLA_pulse_to_trigger_LA();}
                	debug_rssi_gain_sel[debug_rssi_count_i] = RF_RD08(0x62);
                	/*
	                	uart_putchar(0x04);         
	                	uart_putchar(0xFF);        
	                	uart_putchar(0x03); 
	                	uart_putchar(debug_power);               		            	
	                	uart_putchar(debug_rssi_gain_sel[debug_rssi_count_i]);
	                	uart_putchar(debug_rssi_gain_sel[debug_rssi_count_i]); 
	                	*/       	
                	if(debug_rssi_count_i==100)
                	{
                		/*
                		findmaxminValue(debug_rssi_id_ok+0,100,&debug_rssi_id_ok_max,&debug_rssi_id_ok_min);
                		findmaxminValue(debug_rssi_gain_sel+0,100,&debug_rssi_gain_sel_max,&debug_rssi_gain_sel_min);
	                	uart_putchar(0x04);         
	                	uart_putchar(0xFF);        
	                	uart_putchar(0x05); 
	                	uart_putchar(pacl->debug_power);               		            	
	                	uart_putchar(debug_rssi_id_ok_max);
	                	uart_putchar(debug_rssi_id_ok_min);
	                	uart_putchar(debug_rssi_gain_sel_max);
	                	uart_putchar(debug_rssi_gain_sel_min);  
	                	*/
	                	findmaxminValue(debug_rssi_id_ok+0,100,&debug_rssi_id_ok_max,&debug_rssi_id_ok_min);
                		findmaxminValue(debug_rssi_gain_sel+0,100,&debug_rssi_gain_sel_max,&debug_rssi_gain_sel_min);
                		/*
	                	uart_putchar(0x04);         
	                	uart_putchar(0xFF);        
	                	uart_putchar(0x05); 
	                	uart_putchar(debug_power);               		            	
	                	uart_putchar(debug_rssi_id_ok_max);
	                	uart_putchar(debug_rssi_id_ok_min);
	                	uart_putchar(debug_rssi_gain_sel_max);
	                	uart_putchar(debug_rssi_gain_sel_min); 
	                	 */
	                	debug_rssi_id_ok_note_max[debug_power] = debug_rssi_id_ok_max; 
	                	debug_rssi_id_ok_note_min[debug_power] = debug_rssi_id_ok_min; 
	                	debug_rssi_gainsel_note_max [debug_power] = debug_rssi_gain_sel_max;
				debug_rssi_gainsel_note_min [debug_power] = debug_rssi_gain_sel_min;

	                	 
        		}
        		debug_rssi_count_i++;
                	#endif
                	

                //		                    uart_puts("address");
                }//[3]ADDRESS
              //  if( irq1isr_34h.field.bcmatch ) {     
                //		                    uart_puts("bcmatch");        
              //  }//[2]BCMATCH
                if( irq1isr_34h.field.rxdr ) {
                	//uart_putchar(0x01);                    
                		               //     uart_puts("rxdr");     		  
                    leTestEnd_evt.num_packets++;
                    debug_leTestEnd_evt.num_packets++;                    
                    if(pacl->dtm_rx_aod_mode == 0x01)
            	    {     
            	    // read_ctepld_and_formatting(pacl->cte_sample_count,iq_sample+0);  
            	    // send_04_hciEvent_3e_15_LE_connectionless_iq_report(pacl->channelIndex,pacl->cte_type,pacl->cte_sample_count,iq_sample+0);              	
		      llfsm.state = LL_DTM_RX_RXDR_CTE_EVENT;
		    }	
		    else
		    {
		      llfsm.state = LL_DTM_RX_CLOSE_EVENT;
		    }
		    
	            break;
                }//[6]RX_DR
                if( irq1isr_34h.field.txds)
                {
                	debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();
                }
                if( irq1isr_34h.field.maxrt)
                {
                	debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();
                }
                if( irq1isr_34h.field.bcmatch)
                {
                	debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();
                }
                /*
                if( irq1isr_34h.field.txds || irq1isr_34h.field.maxrt ||irq1isr_34h.field.bcmatch) { 
                	debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();debug_RFLA_pulse_to_trigger_LA();               	
                	                		                  //  uart_puts("err");   
			//uart_putchar(irq1isr_34h.reg);
			//err_print____(irq1isr_34h.reg);
                }*/
            }	
       break;
    case LL_ext_adv_ADV_EXT_IND_W4_IRQ_WAKEUP:
    {    	
          if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_B_SET;
            }
    	   break;
    }   
    case LL_ext_adv_ADV_EXT_IND_W4_IRQ_B_SET:
    {    	
            if (irq1isr_E1h.field.b_set)
            {
                write_00h_AccessAddress(P0_ACCESS_ADDR);
                write_50h_CRCInit_555555();
                RF_WT08(0x33, 0x04 | 0x08);         //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //04: [2]MASK_BCMATCH=1 to disable BCMATCH IRQ, no need at send adv_ind
                              //have to irq20txds_w_33h_mask_00 = 1 at llfsm.state = LL_TRT_T1
              //TIMEOUT[15:0] set by set_0bh_0ah_timeout_200us() following
              //RF_WT16(0x0A, (8-1));               //0Ah  TIMEOUT[7:0]  unit 25us  150us tifs + 8us preamble + 32us syncword
              //                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh  BC_CNT[15:8]
                clear_34h_irq1();
                if(change_channel_index == 37){compute_aux_offset= 0x01F4;}	//625*24 = 15000 = 30*500 = 30*0x01F4 us
        	else if(change_channel_index ==38){compute_aux_offset= 0x0177;} //625*18 = 11250 = 30*375 = 30*0x0177 us
        	else if(change_channel_index ==39){compute_aux_offset= 0x00FA;} //625*12 =  7500 = 30*250 = 30*0x00FA us
                pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1                
                set_txHeader_Tx_adv_ext_ind();
                set_txPayload_Tx_adv_ext_ind       (leconfig_bdaddr.le_public_AdvA,pacl->ADInfo_DID,pacl->aclConnHandle,pacl->channelIndex,compute_aux_offset );
           
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                 RF_WT08_53h_MAX_RXPL( RXPL_EN | MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
              	  RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable

                   // set_0bh_0ah_timeout_200us();        //TRT event R w4_connectind_or_scanreq   TIMEOUT[15:0] 200us
                    set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-                                                                             
            #if E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x80 
           
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;
              	    llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_TX_DS;	    
            #elif E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x00
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;   
              	    llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_ANCHOR;                             	    
            #endif
              strobe_TX();                 
            }
        break;
	}
    case LL_ext_adv_ADV_EXT_IND_W4_IRQ_ANCHOR:
     {      
            if (irq1isr_E1h.field.anchor) {	
                    IRQ1ISR_TODO_TypeDef zzz =
                  {
                  }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_TX_DS;
            }
        break;   
     }     
    case LL_ext_adv_ADV_EXT_IND_W4_IRQ_TX_DS:
     {

            if(irq1isr_34h.field.txds) {

                llfsm.state = LL_ext_adv_CLOSE_EVENT;            	
        	}
        break;	
     } 
    case LL_ext_adv_AUX_ADV_IND_W4_IRQ_WAKEUP:
    {    	
          if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_B_SET;
            }
    	   break;
    }   
    case LL_ext_adv_AUX_ADV_IND_W4_IRQ_B_SET:
    {    	
            if (irq1isr_E1h.field.b_set)
            {
                write_00h_AccessAddress(P0_ACCESS_ADDR);
                write_50h_CRCInit_555555();
                RF_WT08(0x33, 0x04 | 0x08);         //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //04: [2]MASK_BCMATCH=1 to disable BCMATCH IRQ, no need at send adv_ind
                              //have to irq20txds_w_33h_mask_00 = 1 at llfsm.state = LL_TRT_T1
              //TIMEOUT[15:0] set by set_0bh_0ah_timeout_200us() following
              //RF_WT16(0x0A, (8-1));               //0Ah  TIMEOUT[7:0]  unit 25us  150us tifs + 8us preamble + 32us syncword
              //                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh  BC_CNT[15:8]
                clear_34h_irq1();
 
                        	
                pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1
                set_txHeader_Tx_aux_adv_ind();        	
                set_txPayload_Tx_aux_adv_ind         (pacl->ADInfo_DID,pacl->aclConnHandle,0x00FA,0x0050,pacl->currChM.chM+0,pacl->AA, pacl->crcInit+0,pacl->periodicEventCount);           
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                 RF_WT08_53h_MAX_RXPL( RXPL_EN | MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
              	 RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable

                   // set_0bh_0ah_timeout_200us();        //TRT event R w4_connectind_or_scanreq   TIMEOUT[15:0] 200us
                    set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-                                                                             
            #if E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x80 
           
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;
              	    llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_TX_DS;	    
            #elif E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x00
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;   
              	    llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_ANCHOR;                             	    
            #endif
              strobe_TX();                 
            }
        break;
	}
    case LL_ext_adv_AUX_ADV_IND_W4_IRQ_ANCHOR:
     {      
            if (irq1isr_E1h.field.anchor) {	
                    IRQ1ISR_TODO_TypeDef zzz =
                  {
                  }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_TX_DS;
            }
        break;   
     } 
    case LL_ext_adv_AUX_ADV_IND_W4_IRQ_TX_DS:
     {
            if(irq1isr_34h.field.txds) {
                llfsm.state = LL_ext_adv_CLOSE_EVENT;            	
        	}
        break;	
     }
    case LL_ext_adv_AUX_SYNC_IND_W4_IRQ_WAKEUP:
    {    	
          if (irq1isr_E1h.field.wk_up) {
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_B_SET;
            }
    	   break;
    }   
    case LL_ext_adv_AUX_SYNC_IND_W4_IRQ_B_SET:
    {    	
            if (irq1isr_E1h.field.b_set)
            {
                RF_WT08(0x33, 0x04 | 0x08);         //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                              //04: [2]MASK_BCMATCH=1 to disable BCMATCH IRQ, no need at send adv_ind
                              //have to irq20txds_w_33h_mask_00 = 1 at llfsm.state = LL_TRT_T1
              //TIMEOUT[15:0] set by set_0bh_0ah_timeout_200us() following
              //RF_WT16(0x0A, (8-1));               //0Ah  TIMEOUT[7:0]  unit 25us  150us tifs + 8us preamble + 32us syncword
              //                                    //0Bh  TIMEOUT[15:8] unit 25us
                RF_WT16_0x0C( BCCNT_0x10 );         //0Ch  BC_CNT[7:0]    (2+0)*8: next BCMATCH is end header
                                                    //0Dh  BC_CNT[15:8]
                clear_34h_irq1();
                        	
                pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1
                set_txHeader_Tx_aux_sync_ind();        	
                set_txPayload_Tx_aux_sync_ind         (pacl->ADInfo_DID,pacl->aclConnHandle,pacl->mappedChannelIndex[1],compute_aux_offset,0x00,0x14);
                set_txCTEsetting_Tx_aux_sync_ind(0x00, 0x14, 0x00,0x00);           
                RF_WT08(0x1A, 0x00);                //WL0     [7]  -
                                                    //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                                    //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                                    //        [4]  Enable Filtering on type_0000 ADV_IND
                                                    //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                                    //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                                    //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                                    //        [0]  Enable Filtering on type_0011 SCAN_REQ
                 RF_WT08_53h_MAX_RXPL( RXPL_EN | MAX_RXPL);
                                                    //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                                    //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                                    //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                                    //        then   HW treat received rxdlen as MAX_RXPL
                                                    //                  finish receive payload MAX_RXPL bytes
                                                    //                  result IRQ[7]CRCF
              	 RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT | 0x04);
                                                    //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                                    //        [6:5]-
                                                    //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                                    //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                                    //      1 [2]  EN_ANCHOR   Anchor Point IRQ enable
                                                    //      0 [1]  EN_B_SET    BLE Setup    IRQ enable
                                                    //      0 [0]  EN_WK_UP    Wake UP      IRQ enable

                   // set_0bh_0ah_timeout_200us();        //TRT event R w4_connectind_or_scanreq   TIMEOUT[15:0] 200us
                    set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-                                                                             
            #if E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x80            
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;
              	    llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_TX_DS;	    
            #elif E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT == 0x00
                    IRQ1ISR_TODO_TypeDef zzz =
                    {
                    }; irq1isrtodo.reg = zzz.reg;   
              	    llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_ANCHOR;                             	    
            #endif
              strobe_TX();                 
            }
        break;
	}
    case LL_ext_adv_AUX_SYNC_IND_W4_IRQ_ANCHOR:
     {      
            if (irq1isr_E1h.field.anchor) {	
                    IRQ1ISR_TODO_TypeDef zzz =
                  {
                  }; irq1isrtodo.reg = zzz.reg;
                llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_TX_DS;
            }
        break;   
     } 
    case LL_ext_adv_AUX_SYNC_IND_W4_IRQ_TX_DS:
     {
            if(irq1isr_34h.field.txds) {
                llfsm.state = LL_ext_adv_CLOSE_EVENT;            	
        	}
        break;	
     }                                
    }//end switch (llfsm.state)
}

bool irq1isr_TRT_advertising_event_is_closed(void)
{
    return ( llfsm.state == LL_TRT_CLOSE_EVENT );
}
bool irq1isr_RTR_scanning_event_is_closed(void)
{
    return ( llfsm.state == LL_RTR_CLOSE_EVENT );
}
bool irq1isr_DTM_TX_event_is_closed(void)
{
    return ( llfsm.state == LL_DTM_TX_CLOSE_EVENT );
}
bool irq1isr_DTM_RX_event_is_closed(void)
{
    return ( llfsm.state == LL_DTM_RX_CLOSE_EVENT );
}
bool irq1isr_DTM_RX_RXDR_CTE_event_is_closed(void)
{
    return ( llfsm.state == LL_DTM_RX_RXDR_CTE_EVENT );
}

bool irq1isr_EXT_TX_event_is_closed(void)
{
    return ( llfsm.state == LL_ext_adv_CLOSE_EVENT );
}
bool irq1isr_slave_connection_event_is_closed(void)
{
    return ( llfsm.state == LL_SLA_CLOSE_EVENT );
}
bool irq1isr_master_connection_event_is_closed(void)
{
    return ( llfsm.state == LL_MAS_CLOSE_EVENT );
}
void irq1isr_open_RTR_scanning_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor)
{
  //disable_A9_interrupts();
    uint8_t en_wk_up;
    BLETIMER_CNT_LO_625US_TypeDef tmp___;
                 tmp___.reg = anchor.cntLoIn625us.reg - 2;          //.reg: sub include bit31 ____z   in case wrap around
             if (tmp___.reg > (RF_RD16(0xF0)+1)) en_wk_up = 0x01;   //E0h[0] EN_WK_UP Wake UP IRQ enable
             else                                en_wk_up = 0x00;
                RF_WT32(0xE4,              tmp___.field.cntlo);     //E4h~E7h: NUM_WK_UP
                 tmp___.reg = anchor.cntLoIn625us.reg - 1;          //.reg: sub include bit31 ____z   in case wrap around
                RF_WT32(0xE8,              tmp___.field.cntlo);     //E8h~Ebh: NUM_B_SET
                RF_WT32(0xEC, anchor.cntLoIn625us.field.cntlo);     //Ech~Efh: NUM_ANCHOR
           //if (anchor.phaseIn1us.field.phase == 1) {
           //   RF_WT16(0xE2, 2);                                   //E2h~E3h: NUM_FINE
           //}
           //else {
                RF_WT16(0xE2, anchor.phaseIn1us.field.phase);       //E2h~E3h: NUM_FINE
           //}
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_RTR_EVENT | 0x0A | en_wk_up);
                                        //E0h    0[7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
            if (en_wk_up == 0x01)
                llfsm.state = LL_RTR_W4_IRQ_WAKEUP;
            else
                llfsm.state = LL_RTR_W4_IRQ_B_SET;
  //enable_A9_interrupts();
}
void irq1isr_open_TRT_advertising_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor, uint8_t type)
{
  //disable_A9_interrupts();
    uint8_t en_wk_up;
    BLETIMER_CNT_LO_625US_TypeDef tmp___;
                 tmp___.reg = anchor.cntLoIn625us.reg - 2;          //.reg: sub include bit31 ____z   in case wrap around
             if (tmp___.reg > (RF_RD16(0xF0)+1)) en_wk_up = 0x01;   //E0h[0] EN_WK_UP Wake UP IRQ enable
             else                                en_wk_up = 0x00;
                RF_WT32(0xE4,              tmp___.field.cntlo);     //E4h~E7h: NUM_WK_UP
                 tmp___.reg = anchor.cntLoIn625us.reg - 1;          //.reg: sub include bit31 ____z   in case wrap around
                RF_WT32(0xE8,              tmp___.field.cntlo);     //E8h~Ebh: NUM_B_SET
                RF_WT32(0xEC, anchor.cntLoIn625us.field.cntlo);     //Ech~Efh: NUM_ANCHOR
           //if (anchor.phaseIn1us.field.phase == 1) {
           //   RF_WT16(0xE2, 2);                                   //E2h~E3h: NUM_FINE
           //}
           //else {
                RF_WT16(0xE2, anchor.phaseIn1us.field.phase);       //E2h~E3h: NUM_FINE
           //}
                if( type == PDUTYPE_ADV_IND      ||
                    type == PDUTYPE_ADV_SCAN_IND )
                {
                    irq1isr_TRT_event_kind = 1; // 1:TRT event, 0:T1 only
                }
                else
                {
                    irq1isr_TRT_event_kind = 0; // 1:TRT event, 0:T1 only
                }
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_TRT_EVENT | 0x0A | en_wk_up);
                                        //E0h    0[7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
            if (en_wk_up == 0x01)
                llfsm.state = LL_TRT_W4_IRQ_WAKEUP;
            else
                llfsm.state = LL_TRT_W4_IRQ_B_SET;
  //enable_A9_interrupts();
}
void irq1isr_open_slave_connection_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor)
{
  //disable_A9_interrupts();
    uint8_t en_wk_up;
    BLETIMER_CNT_LO_625US_TypeDef tmp___;
                 tmp___.reg = anchor.cntLoIn625us.reg - 2;          //.reg: sub include bit31 ____z   in case wrap around
             if (tmp___.reg > (RF_RD16(0xF0)+1)) en_wk_up = 0x01;   //E0h[0] EN_WK_UP Wake UP IRQ enable
             else                                en_wk_up = 0x00;
                RF_WT32(0xE4,              tmp___.field.cntlo);     //E4h~E7h: NUM_WK_UP
                 tmp___.reg = anchor.cntLoIn625us.reg - 1;          //.reg: sub include bit31 ____z   in case wrap around
                RF_WT32(0xE8,              tmp___.field.cntlo);     //E8h~Ebh: NUM_B_SET
                RF_WT32(0xEC, anchor.cntLoIn625us.field.cntlo);     //Ech~Efh: NUM_ANCHOR
           //if (anchor.phaseIn1us.field.phase == 1) {
           //   RF_WT16(0xE2, 2);                                   //E2h~E3h: NUM_FINE
           //}
           //else {
                RF_WT16(0xE2, anchor.phaseIn1us.field.phase);       //E2h~E3h: NUM_FINE
           //}
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_SLAVE_CE | 0x0A | en_wk_up);
                                        //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
            if (en_wk_up == 0x01)
                llfsm.state = LL_SLA_W4_IRQ_WAKEUP;
            else
                llfsm.state = LL_SLA_W4_IRQ_B_SET;
  //enable_A9_interrupts();
}
void irq1isr_open_master_connection_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor)
{
  //disable_A9_interrupts();
    uint8_t en_wk_up;
    BLETIMER_CNT_LO_625US_TypeDef tmp___;
                 tmp___.reg = anchor.cntLoIn625us.reg - 2;          //.reg: sub include bit31 ____z   in case wrap around
             if (tmp___.reg > (RF_RD16(0xF0)+1)) en_wk_up = 0x01;   //E0h[0] EN_WK_UP Wake UP IRQ enable
             else                                en_wk_up = 0x00;
                RF_WT32(0xE4,              tmp___.field.cntlo);     //E4h~E7h: NUM_WK_UP
                 tmp___.reg = anchor.cntLoIn625us.reg - 1;          //.reg: sub include bit31 ____z   in case wrap around
                RF_WT32(0xE8,              tmp___.field.cntlo);     //E8h~Ebh: NUM_B_SET
                RF_WT32(0xEC, anchor.cntLoIn625us.field.cntlo);     //Ech~Efh: NUM_ANCHOR
           //if (anchor.phaseIn1us.field.phase == 1) {
           //   RF_WT16(0xE2, 2);                                   //E2h~E3h: NUM_FINE
           //}
           //else {
                RF_WT16(0xE2, anchor.phaseIn1us.field.phase);       //E2h~E3h: NUM_FINE
           //}
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_MASTER_CE | 0x0A | en_wk_up);
                                        //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
            if (en_wk_up == 0x01)
                llfsm.state = LL_MAS_W4_IRQ_WAKEUP;
            else
                llfsm.state = LL_MAS_W4_IRQ_B_SET;
  //enable_A9_interrupts();
}

void irq1isr_open_DTM_advertising_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor)
{
  //disable_A9_interrupts();
   
                RF_WT32(0xEC, anchor.cntLoIn625us.reg);     //Ech~Efh: NUM_ANCHOR
                RF_WT16(0xE2, anchor.phaseIn1us.reg);       //E2h~E3h: NUM_FINE
           //}           	                
               RF_WT08_0xE0( 0x0C  );   
               // RF_WT08_0xE0( 0x0C  );         
                                        //E0h    0[7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
                strobe_TX();                   //send adv_ind	
                llfsm.state = LL_DTM_TX_W4_IRQ_ANCHOR;
               
  //enable_A9_interrupts();
}

void irq1isr_open_DTM_scanning_event(LEACL_TypeDef *pacl)
{
 //disable_A9_interrupts();
                                        //E0h    0[7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                
                IRQ1ISR_TODO_TypeDef zzz =
                {
			.field.irq08address_w_leh_40_disabledrxen = 0,	
					
                }; 
                irq1isrtodo.reg = zzz.reg;
                llfsm.pacl  = pacl;
                llfsm.state = LL_DTM_RX_SCAN;
                
  //enable_A9_interrupts();
}

void irq1isr_open_EXT_advertising_event(LEACL_TypeDef *pacl, BLETIMER_TypeDef anchor, uint8_t type)
{
  //disable_A9_interrupts();
    uint8_t en_wk_up;
    BLETIMER_CNT_LO_625US_TypeDef tmp___;
                 tmp___.reg = anchor.cntLoIn625us.reg - 2;          //.reg: sub include bit31 ____z   in case wrap around
             if (tmp___.reg > (RF_RD16(0xF0)+1)) en_wk_up = 0x01;   //E0h[0] EN_WK_UP Wake UP IRQ enable
             else                                en_wk_up = 0x00;
                RF_WT32(0xE4,              tmp___.field.cntlo);     //E4h~E7h: NUM_WK_UP
                 tmp___.reg = anchor.cntLoIn625us.reg - 1;          //.reg: sub include bit31 ____z   in case wrap around
                RF_WT32(0xE8,              tmp___.field.cntlo);     //E8h~Ebh: NUM_B_SET
                RF_WT32(0xEC, anchor.cntLoIn625us.field.cntlo);     //Ech~Efh: NUM_ANCHOR
           //if (anchor.phaseIn1us.field.phase == 1) {
           //   RF_WT16(0xE2, 2);                                   //E2h~E3h: NUM_FINE
           //}
           //else {
                RF_WT16(0xE2, anchor.phaseIn1us.field.phase);       //E2h~E3h: NUM_FINE
           //}
		
                RF_WT08_0xE0( E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT | 0x0A |en_wk_up);
                                        //E0h    0[7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        1[3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        0[2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        1[1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
                IRQ1ISR_TODO_TypeDef zzz =
                {
                }; irq1isrtodo.reg = zzz.reg;
                
                llfsm.pacl  = pacl; 
                if(type == PDUTYPE_ADV_EXT_IND)
                {                 
          	 if (en_wk_up == 0x01)
                 {  
                 	llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_WAKEUP;
                 }
           	 else
           	 {
              	   llfsm.state = LL_ext_adv_ADV_EXT_IND_W4_IRQ_B_SET;                
		 }
		}
		else if(type == PDUTYPE_AUX_ADV_IND)
		{
          	 if (en_wk_up == 0x01)
                 {  
                 	llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_WAKEUP;
                 }
           	 else
           	 {
              	        llfsm.state = LL_ext_adv_AUX_ADV_IND_W4_IRQ_B_SET;                
		 }			
		}
		else if(type == PDUTYPE_AUX_SYNC_IND)
		{
          	 if (en_wk_up == 0x01)
                 {  
                 	llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_WAKEUP;
                 }
           	 else
           	 {
              	        llfsm.state = LL_ext_adv_AUX_SYNC_IND_W4_IRQ_B_SET;                
		 }			
		}		
  //enable_A9_interrupts();
}
