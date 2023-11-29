/*********************************************************************************************************//**
 * @file    bc5602_host.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
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
 * <h2><center>Copyright (C) 2021 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcpy,memset   ==>   ht_memory_copy    ht_memory_set
#include <stdbool.h>                         //bool

#include "hwlib/socal/socal.h"

#include "bc5602b_host.h"
#include "bc5602b_irq1_ISR.h"
#include "ble_soc.h"
#include "strobe_cmd.h"
#include "pdma.h"                            //pdma_ram_bleTx_write_payload()
#include "usart.h"                           // uart_puts, uart_putu8
#include "bch_5602.h"
#include "htqueue.h"
#include "mac.h"                             //ADV_PDU_HDR_TypeDef
#include "aes128.h"
#include "advrxq.h"
#include "leconfig.h"
#include "leacl.h"
#include "llc.h"
#include "lldata.h"
#include "hcmsg.h"
#include "hc.h"
#include "csa.h"
//#include "FreeRTOS.h"
//#include "task.h"               //taskENTER_CRITICAL()
//#include "queue.h"

/* Private variables ---------------------------------------------------------------------------------------*/
extern CONN_IND_PAYLOAD_TypeDef rxConnIndPayload;
static unsigned char iq_sample[164];
static unsigned char debug_ctepld[1024];

unsigned char debug_rssi_id_ok_note_max[110];
unsigned char debug_rssi_id_ok_note_min[110];
unsigned char debug_rssi_gainsel_note_max[110];
unsigned char debug_rssi_gainsel_note_min[110];


/* Global functions ----------------------------------------------------------------------------------------*/
void init_0rf_5602B(void);
void init_0rf_5602G2TC8_v8(uint8_t phy);
void init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00(void);
void init_1ad_2in1_5602G2_SAR_ADC_10bits(void);
bool debug_send_m2s_1(LEACL_TypeDef *); //debug

void ht_memory_copy(uint8_t *dst, uint8_t *src, uint32_t len)
{
    while (len) {
        ht_write_byte( dst, *src);
        dst ++; src ++;
        len --;
    }
}
void ht_memory_set(uint8_t *dst, uint8_t v, uint32_t len)
{
    while (len) {
        ht_write_byte( dst, v);
        dst ++;
        len --;
    }
}
uint32_t ht_memory_compare(uint8_t *s1, uint8_t *s2, uint32_t count)
{
    uint32_t difference=0;
    while (count) {
        if ( ht_read_byte(s1) != ht_read_byte(s2) ) {
            difference ++;
        }
        s1 ++; s2 ++;
        count --;
    }
    return difference;
}

static inline void debug_RFLA_pulse_to_trigger_LA(void)
{
    RF_WT08(0x100, 0x1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t        state;
  //uint32_t        delay_offset;
} bc5602b_hcfsm_holtek_TypeDef;
static volatile bc5602b_hcfsm_holtek_TypeDef hcfsm;

volatile uint32_t  schedluezzzz=0;
extern ADV_PDU_HDR_S0_TypeDef txAdvHeaderS0;
volatile uint32_t  TRT_delay_offset;
volatile uint32_t  EXT_ADV_Interval_offset;

extern volatile uint8_t change_channel_index;
extern volatile uint8_t debug_channel_index;
volatile uint8_t  debug_channel_index = 37;
volatile uint8_t  debug_counter = 0;
extern const    uint8_t RfCenterFreq[50];  // index by channelIndex
extern const    uint8_t WhiteningSeed[50]; // index by channelIndex

//////////////////////////////////////////////////////
void delay_zzzz(void)
{
    uint32_t z,w;
//  for(z=0;z<10000;z++) {w++;}
    for(z=0;z<100000;z++) {w++;}
}

void disable_A9_interrupts(void);
void enable_A9_interrupts(void);
void kidd_taskENTER_CRITICAL(void)
{
//  alt_write_byte(0xFFFED180+12,  0x01);                 // ICDICER    Clear-enable bits,           12*8=96,96+0= 96    :interruptID of RFIRQ 5602
//  disable_A9_interrupts();
//  taskENTER_CRITICAL();
}
void kidd_taskEXIT_CRITICAL(void)
{
//  alt_write_byte(0xFFFED100+12,  0x01);                 // ICDISER    Set-enable bits,             12*8=96,96+0= 96    :interruptID of RFIRQ 5602
//  enable_A9_interrupts();
//  taskEXIT_CRITICAL();
}

/********************************************************************************/
//static uint8_t master_DeviceAddr[6] = {0xF2,0xF1,0xe2,0x23,0x7F,0x33     };
  static uint8_t master_DeviceAddr[6] = {0x65,0x92,0x9F,0x96,0x68,0xC2     };//0xC268969F9265   cc51822
//static uint8_t master_DeviceAddr[6] = {0x33,0xDD,0x33,0xDD,0x33,0xDD     };//

  //static uint8_t  slave_DeviceAddr[6] = {0x31,0xDD,0x30,0x31,0x32,     0x33};
 static uint8_t slave_DeviceAddr[6] = {0x33,0x31,0xAE,0xEA,0xBE,0xEF     };// 
//static uint8_t  slave_DeviceAddr[6] = {0x31,0xDD,0x30,0x31,0x32,0xC0|0x33};
//static uint8_t  slave_DeviceAddr[6] = {0x31,0xDD,0x31,0xDD,0x31,0xC0|0xDD};
//static uint8_t  slave_DeviceAddr[6] = {0x33,0xDD,0x33,0xDD,0x33,0xC0|0xDD};//for nRF sniffer to capture and know master's DeviceAddress
//  static uint8_t  slave_DeviceAddr[6] = {0x33,0x33,0x33,0x33,0x33,0xC0|0x33};

//btle.advertising_address == dd:31:dd:31:dd:31
//btle.advertising_address == dd:33:dd:33:dd:33
//btle.length != 0


void channel_selection_algorithm_1_leacl_currChannel(LEACL_TypeDef *pacl)
{
    uint8_t unmapIndex;
        //unmappedChannel = (lastUnmappedChannel + hopIncrement) mod 37
        pacl->unmappedChIndex = (pacl->lastUnmappedChIndex + pacl->hopIncrement) % 37 ;
        pacl->lastUnmappedChIndex = pacl->unmappedChIndex ;
    //-----------------------
    if ((pacl->pNewChM != 0) &&
        (pacl->pNewChM->instant == pacl->connEventCount) )
    {
        ht_memory_copy( (uint8_t *)&(pacl->currChM), (uint8_t *)(pacl->pNewChM), sizeof(LLCHANNELMAP_TypeDef) );
               #if    __MALLOC_METHOD__  == 1
                    free( (void *)pacl->pNewChM );
               #elif  __MALLOC_METHOD__  == 2
               vPortFree( (void *)pacl->pNewChM );
               #endif
                                  pacl->pNewChM = 0;
        //Channel Map Update procedure is complete when the instant has passed and the new channel map has been applied to the ACL
        if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_01_CHANNEL_MAP_IND)) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon the Channel Map Update instant has passed
        }
        if (LLC_RESPOND_PROCEDURE_is_llcOP(pacl, LL_01_CHANNEL_MAP_IND)) {
            LLC_RESPOND_PROCEDURE_end_complete(pacl);  //complete RespondProcedure, clear state upon the Channel Map Update instant has passed
        }
    }
    //-----------------------
    unmapIndex = pacl->unmappedChIndex ;
    if (pacl->currChM.chM[unmapIndex>>3] & (1<<(unmapIndex&0x07)) )
    {
        pacl->channelIndex = unmapIndex ;
        pacl->channel      =(unmapIndex<=10) ? (4+(unmapIndex<<1)) : (6+(unmapIndex<<1)) ;
    }
    else
    {
        uint8_t remappingIndex;
        //remappingIndex = unmappedChannel mod numUsedChannels
        remappingIndex = unmapIndex % pacl->currChM.numUsedChannels ;
        
        pacl->channelIndex  = pacl->currChM.usedChIndex_byRemapIndex[remappingIndex];
        pacl->channel       = (pacl->channelIndex<=10) ? (4+(pacl->channelIndex<<1)) : (6+(pacl->channelIndex<<1)) ;
    }
};

#if RTC_CLK_FROM_LSE == 1
  static volatile uint32_t cem_ppm=0;//30;
  static volatile uint32_t ces_ppm=0;//30;
#else
  static volatile uint32_t cem_ppm=500;
  static volatile uint32_t ces_ppm=500;
#endif


/********************************************************************************/
static inline uint32_t ppm_x_sinceLastAnchorIn1250us_to_winWideningInUs(uint32_t ppm, uint32_t sincelast_unit1250us)
{
    //   us = sincelast_unit1250us * 1250 * ppm / 1000000
    //      = sincelast_unit1250us * 1.25 * ppm / 1000
    uint32_t us;
    us = sincelast_unit1250us * ppm ;
    us = us + (us >> 2); // >>2: 0.25
    us = us / 1000;
    return us ;
}
uint32_t ppm_x_sinceLastAnchorIn1250us_to_slaveRXTO25us(uint32_t ppm, uint32_t sincelast_unit1250us)
{
    volatile float fff;
    uint32_t cnt;
    fff  = sincelast_unit1250us ;
    fff *= ppm ;
    fff /= (25*1000000/(1250*2)) ;    //   25: RX_TO in unit 25us
                                      //    2: double because widening_before and widening_afer
    cnt = fff ;
    cnt += 1 ;   // +1 from remainder
    /*
    uart_putchar_n('\n');
    uart_puts(" uint32_t ppm=");       uart_putu16(ppm>>16);  uart_putu16(ppm);
    uart_puts(" uint16_t sincelast="); uart_putu16(sincelast);
    uart_puts(" -> RXTO ");       uart_putu16(cnt);
    uart_putchar_n('\n');
    */
    return cnt ;
}

/********************************************************************************/


/*
static unsigned char reverse_order_foo(unsigned char a)
{
	unsigned char b;
	switch(a&0x0F) {
		case 0x00: b=0x00; break;
		case 0x01: b=0x08; break;
		case 0x02: b=0x04; break;
		case 0x03: b=0x0C; break;
		case 0x04: b=0x02; break;
		case 0x05: b=0x0A; break;
		case 0x06: b=0x06; break;
		case 0x07: b=0x0E; break;
		case 0x08: b=0x01; break;
		case 0x09: b=0x09; break;
		case 0x0A: b=0x05; break;
		case 0x0B: b=0x0D; break;
		case 0x0C: b=0x03; break;
		case 0x0D: b=0x0B; break;
		case 0x0E: b=0x07; break;
		case 0x0F: b=0x0F; break;
	}
	return b;
}
static unsigned char reverse_order(unsigned char a)
{
	unsigned char b;
	b=reverse_order_foo(a&0x0F);
	b<<=4;
	b |= reverse_order_foo(a>>4);
	return b;
}

static void calc_whitening_seed(void)
{
	unsigned char idx, zzz;
    for(idx=0; idx<=0x3F; idx++)
    {
        uart_puts("0x"); uart_putu8(idx); uart_puts(",");
    }
        uart_putchar_n('\n');
    for(idx=0; idx<=0x3F; idx++)
    {
        zzz = 0x40+idx ;
        zzz <<= 1;
        zzz = reverse_order(zzz);
        uart_puts("0x"); uart_putu8(zzz); uart_puts(",");
    }
        uart_putchar_n('\n');
}
*/

//============================================================
#if   PWR_SAVING_MODE == 0   // none
//
#else
static void wait_UR0TxBuf_empty_n_LineStatusTransmitterEmpty(void)
{
                    while ( !IS_BUFFER_EMPTY(USR1TxReadIndex, USR1TxWriteIndex) ||
                            !USART_GetFlagStatus(HTCFG_USART,USART_FLAG_TXC) )  // Transmit Complete, 1:Both the TX FIFO and TSR register are empty
                    {
                    }
}
#endif


//////////////////////////////////////////////////////
/**
 *  @ingroup SPI_Commands_label
 */
static inline unsigned char read_40h_omst(void)
{
	/* OMST: Operation mode state
	0h: Deep Sleep Mode
	2h: Light Sleep Mode
	4h: TX Mode
	5h: RX Mode
	6h: Calibration Mode
	*/
    unsigned char reg40h;
    reg40h = RF_RD08(0x40);             //40h
                                        //      [7]  PWR_SOFT
                                        //      [6:4]OMST
                                        //      [3]  ACAL_EN
                                        //      [2]  RTX_EN
                                        //      [1]  RTX_SEL
                                        //      [0]  SX_EN
//uart_puts("40h="); uart_putu8(reg40h); uart_putchar_n('\n');
    return reg40h;
}
/**
 *  @ingroup SPI_Commands_label
 */
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


/**
 *  @ingroup SPI_Commands_label
 */
static inline void RF_BC5602B_dn_dk_configure(uint16_t frequ) // 2402~2480
{
    float fn;
    uint8_t dn;
    uint32_t dk;
    fn = frequ;
  #if   _FRONTEND_RF_XCLK_MHZ_ == 16
    fn /= (float)(2*16);  // 16:XO_Freq 16MHz      2:Div       frequ: 2426 mean 2426 MHz
  #elif _FRONTEND_RF_XCLK_MHZ_ == 32
    fn /= (float)(2*32);  // 16:XO_Freq 16MHz      2:Div       frequ: 2426 mean 2426 MHz
  #endif
    dn = (uint8_t)fn;
    fn -= (float)dn;
    fn *= (float)(1048576.0);// 2^20=1048576
    dk = (uint32_t)fn;
//bank 2
    SPI_BYTE(0x22);
    SPI_WT(0x22, dn);                           //SX1    D_N[6:0]
    SPI_WT(0x23,  dk & 0xFF);                   //SX2    D_K[7:0]
    SPI_WT(0x24, (dk >> 8) & 0xFF);             //SX3    D_K[15:8]
    SPI_WT(0x25, 0xA0 | ((dk >>16) & 0x0F));    //SX4    D_K[19:16]  [5]TX_MORE_EN    [7]DKN_TEST: DKN value selection, 0:DKN value from RF_CH, 1:DKN value from SX1,2,3,4
//uart_puts(" DN="); uart_putu8(dn);
//uart_puts(" DK="); uart_putu8(dk>>16); uart_putu16(dk); 
//uart_putchar_n('\n');
}
void init_0rf_1adc_dtm_mode(uint8_t phy)
{
	init_0rf_5602G2TC8_v8(phy);
}

void init_0rf_2in1_5602B_set_rfch(uint8_t ch) //5602B RF
{
    SPI_CS(0);//0:RF
//ch=RFCHANNEL;//debug
  //
#if RF_SEL == 0
    SPI_WT(0x07, ch);               //RFCH            2402M:02h,  2426M:1Ah,  2441M:29h,  2480M:50h
#elif RF_SEL == 1 
    SPI_WT(0x10, ch);               //RFCH            2402M:02h,  2426M:1Ah,  2441M:29h,  2480M:50h
#endif    
//bank 2
    SPI_BYTE(0x22);
  //SPI_WT(0x37, 0x11);             //CA1      [7]RTXAC_EN, [4:2]DLY_VCOCAL, [1]CALBUF_EN,   [0]VCOCAL_EN=1 to cali VCO
    SPI_WT(0x20, 0x08);             //OM       [3]ACAL_EN cali VCO and RC
    while ( (SPI_RD(0x20) & 0x08) == 0x08 ) {;} //OM[3]ACAL_EN flag Read Only
  /*
    RF_BC5602B_dn_dk_configure(2400+ch);
//bank 2
//  SPI_BYTE(0x22);
    if      (ch <= 16)
    SPI_WT(0x2C, 0x01+(3<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else if (ch <= 32)
    SPI_WT(0x2C, 0x01+(4<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else if (ch <= 48)
    SPI_WT(0x2C, 0x01+(5<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else if (ch <= 64)
    SPI_WT(0x2C, 0x01+(6<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else if (ch <= 80)
    SPI_WT(0x2C, 0x01+(7<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else if (ch <= 96)
    SPI_WT(0x2C, 0x01+(8<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
    else
    SPI_WT(0x2C, 0x01+(9<<1));       //VC2     [4:1]VCO_DFC  [0]DFC_OW       2402M: 07    2426M:09    2441M:0B     2480M:0F
  */
  SPI_BYTE(0x20);
}

  static address5602_TypeDef syncID_p0={P0_SYNCWORD, P0_LSBYTE};
void init_syncID(void)
{
    //set pipe 0 address
        strobe_write_pipe_0_ID(syncID_p0); //<<<<<<<<<<<

    //check
    {
        address5602_TypeDef id;
        
        
        strobe_read_pipe_0_ID(&id);
        
        
        uart_putchar_n('\n');
        uart_puts("read pipe 0 ID get ");
        uart_putu8(id.byte4321>>24);
        uart_putu8(id.byte4321>>16);
        uart_putu8(id.byte4321>>8);
        uart_putu8(id.byte4321);
        uart_putu8(id.byte0);
        uart_putchar_n('\n');
        
        /*
                RF_WT08(0x00, 0x29);    //P0B0
                RF_WT08(0x01, 0x41);    //P0B1
                RF_WT08(0x02, 0x76);    //P0B2
                RF_WT08(0x03, 0x71);    //P0B3
                RF_WT08(0x04, 0x71);    //P0B4
                        strobe_read_pipe_0_ID(&id);
                        
                                uart_putchar_n('\n');
        uart_puts("read pipe 0 ID get ");
        uart_putu8(id.byte4321>>24);
        uart_putu8(id.byte4321>>16);
        uart_putu8(id.byte4321>>8);
        uart_putu8(id.byte4321);
        uart_putu8(id.byte0);
        uart_putchar_n('\n');
        */
    }
}

void set_txHeader_txPayload_DTM_tx_packet(uint8_t packet_payload, uint8_t test_data_length, uint8_t *txTestData)
{
    	
    // BLE mode with only 1 RX FIFO and 1 TX FIFO,  no need to check fifostatus RF_RD08(0x35)
    // BLE mode no need to check STATUS[5]TX_FULL   unused in BLE mode

    pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1,  test_data_length, txTestData   );//send adv_ind, AdvA       6 octets
   /* 
    ADV_PDU_HDR_TypeDef hdr =
    {
	.field.type   = packet_payload,
        .field.rfu    = 0,
        .field.chsel  = 0,
        .field.txadd  = 0,
        .field.rxadd  = 0,
        .field.length = test_data_length
    };
    hdr.field.type = packet_payload ;
    */
   // txTestHeader.reg = hdr.reg; //save at txAdvHeader
              RF_WT08(0x14, packet_payload);
              RF_WT08(0x15, test_data_length);
              //  RF_WT16(0x14,  hdr.reg);             //HB1 H_S0
                                                    //HB2 H_LENGTH
              //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
                                                    
}
void set_txHeader_txPayload_DTM_tx_aoa_packet(uint8_t packet_payload, uint8_t test_data_length, uint8_t *txTestData, uint8_t cte_type, uint8_t cte_length)
{
	pdma_ram_bleTx_write_payload(CH2TXADDR_TRT_T1,  test_data_length, txTestData   );//send adv_ind, AdvA       6 octets
       
        RF_WT08(0x14, packet_payload|0x20);				  //HB1 H_S0
        RF_WT08(0x15, test_data_length); 				  //HB2 H_LENGTH
        RF_WT08(0x16,(cte_type<<6)|(cte_length));             //HB3 H_S1  used at cteinfo field


}

void init_2fpga(void)
{
    strobe_software_reset();            //1Fh     [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
    delay_zzzz();
//delay_unit625us(2);
    strobe_pdb_0_deepsleep();                   //PDB [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
    strobe_pdb_1_lightsleep_wait_xclk_ready();  //PDB [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
    
  //RF_WT08(0x17, 0x07);                //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
    RF_WT08(0x18, 0x80);                //HB4     [7:4]L_S0     bits of S0
                                        //        [3:0]L_S1     bits of S1
    RF_WT08(0x19, 0x08);                //HB5     [3:0]L_LENGTH bits of LENGTH
                                        //                                           L_S0  L_LENGTH  L_S1
                                        //                                              0      6       3   ==> 24L01 (ENAA=1)
                                        //                                              0      0       0   ==> 24L01 (ENAA=0)
                                        //                                              8      8       0   ==> BLE
//strobe
    init_syncID();

  //RF_WT08(0x0A, 0x__);                //TIMEOUT[7:0]  unit 25us
  //RF_WT08(0x0B, 0x__);                //TIMEOUT[15:8] unit 25us
  //RF_WT08(0x0C, 0x__);                //BC_CNT[7:0]
  //RF_WT08(0x0D, 0x__);                //BC_CNT[15:8]
  if( _HOST_TESTMODE_ == 1 ) // 0x00:master, 0x01:slave
  {
    RF_WT08(0x0E, master_DeviceAddr[0]);//WL1     White list ADDRESS[7:0]   master_DeviceAddr[0]
    RF_WT08(0x0F, master_DeviceAddr[1]);//WL2     White list ADDRESS[15:8]  master_DeviceAddr[1]
    RF_WT08(0x10, master_DeviceAddr[2]);//WL3     White list ADDRESS[23:16] master_DeviceAddr[2]
    RF_WT08(0x11, master_DeviceAddr[3]);//WL4     White list ADDRESS[31:24] master_DeviceAddr[3]
    RF_WT08(0x12, master_DeviceAddr[4]);//WL5     White list ADDRESS[39:32] master_DeviceAddr[4]
    RF_WT08(0x13, master_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] master_DeviceAddr[5]
//  RF_WT08(0x13, 0xC0|master_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] master_DeviceAddr[5]
  }
  else
  {
    RF_WT08(0x0E, slave_DeviceAddr[0]); //WL1     White list ADDRESS[7:0]   slave_DeviceAddr[0]
    RF_WT08(0x0F, slave_DeviceAddr[1]); //WL2     White list ADDRESS[15:8]  slave_DeviceAddr[1]
    RF_WT08(0x10, slave_DeviceAddr[2]); //WL3     White list ADDRESS[23:16] slave_DeviceAddr[2]
    RF_WT08(0x11, slave_DeviceAddr[3]); //WL4     White list ADDRESS[31:24] slave_DeviceAddr[3]
    RF_WT08(0x12, slave_DeviceAddr[4]); //WL5     White list ADDRESS[39:32] slave_DeviceAddr[4]
    RF_WT08(0x13, slave_DeviceAddr[5]); //WL6     White list ADDRESS[47:40] slave_DeviceAddr[5]
//  RF_WT08(0x13, 0xC0|slave_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] slave_DeviceAddr[5]
  }
  //RF_WT08(0x14, 0x__);                //HB1 H_S0
  //RF_WT08(0x15, 0x__);                //HB2 H_LENGTH
  //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
    RF_WT08(0x17, 0x07);                //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
    RF_WT08(0x18, 0x80);                //HB4     [7:4]L_S0     bits of S0
                                        //        [3:0]L_S1     bits of S1
    RF_WT08(0x19, 0x08);                //HB5     [3:0]L_LENGTH bits of LENGTH
                                        //                                           L_S0  L_LENGTH  L_S1
                                        //                                              0      6       3   ==> 24L01 (ENAA=1)
                                        //                                              0      0       0   ==> 24L01 (ENAA=0)
                                        //                                              8      8       0   ==> BLE
    RF_WT08(0x1A, 0x00);   //White list //FILTEREN[7]  -
                                        //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                        //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                        //        [4]  Enable Filtering on type_0000 ADV_IND
                                        //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                        //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                        //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                        //        [0]  Enable Filtering on type_0011 SCAN_REQ
    RF_WT08(0x1B, 0x60|0x02);           //1Bh     [6:4]PIPEAP_SEL: pipe selection PRX write Ack Payload  0h:pipe0, 1h:pipe1, 2h:pipe2, 3h:pipe3 4h:pipe4, 5h:pipe5, 6h:idle
                                        //        [1:0]TXFIFO_SEL: 0h:txfifo_ack, 1h:txfifo_no_ack, 2h:idle
  ////////////////
  //RF_WT08(0x1C, 0x01);                //PDB     [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
  //done above strobe_pdb_1_lightsleep_wait_xclk_ready()
  ////////////////
    RF_WT08(0x1D, RFCHANNEL);           //RF_CH   [7]-,   [6:0]RF_CH
    RF_WT08(0x1E, 0x00);                //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
//common bank
   
    RF_WT08(0x30, CFG1_AGC_EN | 0x00);  //CFG1
                                        //      [1:0]BANK[1:0] control register Bank select, 00:bank0, 01 : bank1, 10 : bank2, 11 : bank3
                                        //      [3:2]-
                                        //      [4]DIR_EN   direct mode enable
                                        //      [5]PN9_EN   Enable TX output PN9 sequence
                                        //      [6]AGC_EN=0
                                        //      [7]-
  //RF_WT08(0x31, 0x90);                //RC1
                                        //      [7]  PWRON
                                        //      [6]  FSYCK_RDY  FSYCK ready flag
                                        //      [5]  XCLK_RDY   XCLK  ready flag
                                        //      [4]  XCLK_EN    Enable XCLK path to baseband functional block
                                        //      [3:2]FSYCK_DIV  FSYCK dividend from XCLK. 0h¡G1/1, 1h¡G1/2, 2h¡G1/4, 3h¡G1/8
                                        //      [1]  FSYCK_EN   FSYCK clock enable
                                        //      [0]-
    RF_WT08(0x32, CAR_EN_IRQ1|0x00);    //RC2
  //RF_WT08(0x32, CAR_EN_IRQ1|0x30);    //RC2
                                //only BLE Mode [7]  CAR_EN  Clear after Read enable       (note: CAR_EN used only for BLE Mode)
                                //only BLE Mode [6]  BCC_OW  0:reading BC_CNT get prev written BC_CNT, 1:reading BC_CNT get RF real value BC_CNT
                                        //      [5]  PSAVE1  XCLK clock gating enable for power saving mode 1
                                        //      [4]  PSAVE   XCLK clock gating enable for power saving mode 0
                                        //    RO[3:1](RO)RX_P_NO
                                        //    RO[0]  (RO)TX_FULL
    RF_WT08(0x33, 0x00);                //MASK  [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO,  [0]-
                                        //      [7]  MASK_CRCF
                                        //      [6]  MASK_RX
                                        //      [5]  MASK_TX
                                        //      [4]  MASK_MAX_RT
                                        //      [3]  MASK_ADDRESS
                                        //      [2]  MASK_BCMATCH
                                        //      [1]  MASK_RX_TO
                                        //      [0]  -
    clear_34h_irq1();                   //IRQ1  [7]CRCF, [6]RX_DR, [5]TX_DS, [4]MAX_RT, [3]ADDRESS, [2]BCMATCH, [1]RX_TO, [0]-
                                        //      [7]  CRCF
                                        //      [6]  RX_DR
                                        //      [5]  TX_DS
                                        //      [4]  MAX_RT
                                        //      [3]  ADDRESS
                                        //      [2]  BCMATCH
                                        //      [1]  RX_TO
                                        //      [0]-
////RF_WT08(0x35, 0x__);(Read Only)     //fifoSTATUS  (RO read only)
                                        //        [7:6]-
                                        //        [5]  TX_FULL
                                        //        [4]  TX_EMPTY
                                        //        [3:2]-
                                        //        [1]  RX_FULL
                                        //        [0]  RX_EMPTY
////RF_WT08(0x36, 0x__);(Read Only)     //RXDLEN  [7:6]-,  [5:0](RO)RXDLEN
    RF_WT08(0x37, 0x80);                //RT1 (04_SETUP_RETR)
                                        //        [7:4]ARD: auto retransmit delay
                                        //        [3:0]ARC: auto retransmit maximum times allowed
////RF_WT08(0x38, 0x__);(Read Only)     //RT2 (08_OBSERVE_TX)(Read-Only)
                                        //        [7:4](RO)CNT_PLOS
                                        //        [3:0](RO)CNT_ARC
//bank 0
  //RF_WT08(0x40, 0x__);                //OM
    RF_WT08(0x41, 0x2A);                //41h
                                        //        [7]  STEP_EN    Phase step enable. 0:disable, 1:enable
                                        //        [6]  AMBLE2     Preamble length selection   0:Preamble one byte,  1:Preamble two bytes
                                        //       1[5]  SKIP_PRE   Skip preamble detect in BLE mode
                                        //        [4]  MDCG_EN    Match detect clock-gating enable for DCLK2. 0h¡GDisable, 1h¡GEnable
                                        //        [3:0]RSSI_CTHD  RSSI threshold for carrier detection in -dBm   => (RSSI_CTHD * 2 + 1) + 74 = RSSI threshold for carrier detection
    RF_WT08(0x42, PKT1_CRCLEN |AW | D_RATE);      //42h
                                        //        [7:6]CRC_LENGTH   00:disabled, 01:CRC8, 10:CRC16, 11:CRC24
                                        //        [5:4]AW   11:5 bytes, 10:4 bytes, 01:3 bytes, 00:-
                                        //             Note¡Gsyncword length is force to 4 bytes In BLE mode
                                        //        [3]  -
                                        //        [2:0]D_RATE 0h:250Kbps, 1h:1Mbps, 2h:2Mbps, 4h:TX LE coded S=8, RX is auto switch by CI
                                        //                                                    5h:TX LE coded S=2, RX is auto switch by CI
                                        //                    others:illegal
    RF_WT08(0x43, 0x30);                //43h     [7:0]FD_HOLD
    RF_WT08(0x44, 0x80);                //44h     [7:0]PHASE_DSTEP
    RF_WT08(0x45, PKT1_CRC_ADD_EN|0x00);            //45h (unused in BLE mode)
  //RF_WT08(0x45, P5ACTIVE | P4ACTIVE | P3ACTIVE | P2ACTIVE | P1ACTIVE | P0ACTIVE); // (unused in BLE mode)
                                        //        [7]  CRC_ADD_EN     0:sync not include, 1:sync include
                                        //        [6]  CRC_TEST       Enable TX test format, CRC is 0x555555 for LE RF-PHY conformance testing using Direct Test Mode
                                        //        [5]  P5ACTIVE (unused in BLE mode)
                                        //        [4]  P4ACTIVE (unused in BLE mode)
                                        //        [3]  P3ACTIVE (unused in BLE mode)
                                        //        [2]  P2ACTIVE (unused in BLE mode)
                                        //        [1]  P1ACTIVE (unused in BLE mode)
                                        //        [0]  P0ACTIVE (unused in BLE mode)
  //RF_WT08(0x46, 0x__);(Read Only)     //STA1
                                        //      RO[7]  CD_FLAG Carrier Detection flag
                                        //        [6:3]-
                                        //      RO[2:0]OMST
  //RF_WT08(0x47, 0x__);(Read Only)     //RSSI2 RO[7:0]RSSI_NEGDB (RO read only)
  //RF_WT08(0x48, 0x__);(Read Only)     //RSSI3 RO[7:0]RSSI_ID_OK (RO read only)
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
    RF_WT08(0x4A, (0<<3)|0x03);
                                        //PKT3    [7]  WHT_PCF_EN=0
                                        //        [6:3]DLY_TIFS  add delay in us, Rx to Tx , switch time is DLY_TXS + DLY_TIFS
                                        //        [2:0]DLY_CONTI RF continue mode delay time  TX: 0:8uS, 1:16uS, 2:32uS, 3:64uS, 4:96uS, 5:128uS, 6:192uS, 7:256uS
                                        //                                                    RX: 0:1uS, 1: 4uS, 2: 8uS, 3:12uS, 4:16uS, 5: 24uS, 6: 40uS, 7:128uS
    RF_WT08(0x4B, 0x05);                //PKT5    [7]  -
                                        //        [6:4]DLY_RXS             0:4us, 1: 8us, 2:12us, 3:16us, 4:20us, 5:32us, 6: 64us, 7:100us
                                        //        [3]  - 
                                        //        [2:0]DLY_TXS  nRF24 mode 0:10us,1:20us, 2:30us, 3:40us, 4:50us, 5:60us, 6: 70us, 7: 80us
                                        //                        BLE mode 0:40us,1:50us, 2:60us, 3:70us, 4:80us, 5:90us, 6:100us, 7:110us
  #if   PKT1_CRCLEN == 0x40
    RF_WT08(0x4C, 0x07);                //CRC1        CRC8  = X^8 + X^2 + X + 1
  #elif PKT1_CRCLEN == 0x80
    RF_WT08(0x4C, 0x21);                //CRC1        CRC16 = X^16 + X^12 + X^5 + 1
    RF_WT08(0x4D, 0x10);                //CRC2
  #elif PKT1_CRCLEN == 0xC0
    RF_WT08(0x4C, 0x5B);                //CRC1        CRC24  x24 + x10 + x9 + x6 + x4 + x3 + x + 1
    RF_WT08(0x4D, 0x06);                //CRC2
    RF_WT08(0x4E, 0x00);                //CRC3
  #endif
    RF_WT08(0x4F, 0x55);                //CRC4 CRC_INI[ 7: 0] CRC initial value 
    RF_WT08(0x50, 0x55);                //CRC5 CRC_INI[15: 8] CRC initial value 
    RF_WT08(0x51, 0x55);                //CRC6 CRC_INI[23:16] CRC initial value 
  //RF_WT08(0x52, 0x__);                //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                        //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                        //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                        //        then   HW treat received rxdlen as MAX_RXPL
                                        //                  finish receive payload MAX_RXPL bytes
                                        //                  result IRQ[7]CRCF
  //RF_WT08(0x53, 0x__);                //CTE     [7:0]CTE_TYPE
  //RF_WT08(0x54, 0x__);                //CTE     [7:0]CTE_LENGTH

//bank 1
    RF_WT08(0x60, 0x6C);                //AGC1    [1:0]AGC_CMP_THD    AGC comparison number threshold, 0:continuos AGC until SYNCWORD is detected, 1~3:comparison number thrshold
                                        //        [3:2]ZONE_OFFSET
                                        //        [6:4]OFFSET_SEL
                                        //        [7]  MPT_0DB_EN     Force max point = 0dB enable
    RF_WT08(0x61, 0x04);                //AGC2
                                        //        [2:0]IFDET_OK_THD   IF detection OK threshold
                                        //        [7:3]OFFSET         RSSI Calculation Offset Setting
  //RF_WT08(0x62, 0x__);                //AGC3(RO read only)
                                        //        [2:0]AGC_ST         AGC state machine's state, 0x7:AGC_complete
                                        //        [3]-
                                        //        [7:4]GAIN_SEL       gain curve select
    RF_WT08(0x63, 0x30);                //AGC4
                                        //        [7:0]GAIN_STB[7:0]
    RF_WT08(0x64, 0x03);                //FCF1
                                        //        [7:4]MOD_INDEX2[3:0]
                                        //        [3:0]MOD_INDEX1[3:0]
  //RF_WT08(0x65, 0x44);                //FCF2
    RF_WT08(0x65, 0xCC);                //FCF2
                                        //        [7:0]FSCALE[7:0]     todo
//  RF_WT08(0x66, 0x4C);//IF 600K 16M xtal //FCF3        RX's Fscale(change IF) = IF * 2^20 / (2 * Fxtal) = (600000 * 2^20)/(2*16000000) = 19660.8   19660=0x4CCC
    RF_WT08(0x66, 0x1C);//IF 600K 32M xtal //FCF3        RX's Fscale(change IF) = IF * 2^20 / (2 * Fxtal) = (600000 * 2^20)/(2*32000000) =  9830.4    9830=0x2666   1CCC
                                        //        [7]-
                                        //        [6:0]FSCALE[14:8]    todo
    RF_WT08(0x67, 0x00);                //FCF4
                                        //        [7]  RXFD_EN        RX Frequency Deviation enable
                                        //        [6]  FMOD_TS
                                        //        [5:0]-
    RF_WT08(0x6A, 0x35);                //DA1     [7]TPCALF    [6]TPCAC_EN     [5:4]TPCAL_TC  [3:1]DLY_TPCAL   [0]TPCAL_EN
  #if   D_RATE == 0 // 250 kbps
    RF_WT08(0x71, 0x0C);                //DA8     [3]  TPCDNK_BP=1 if need ch hopping
                                        //        [2]  DAC_I_OW=1
    RF_WT08(0x72, 0x00);                //DA9     [7:0]DAC_I[7:0]
    RF_WT08(0x73, 0x02);                //DA10    [1:0]DAC_I[9:8]    [7:2]-
  #elif D_RATE == 1 // 1 Mbps
    // tx cannot set DA8[2]DAC_I_OW=1
    RF_WT08(0x71, 0x08);                //DA8     [3]  TPCDNK_BP=1 if need ch hopping
                                        //        [2]  DAC_I_OW=0
  //RF_WT08(0x72, 0x00);                //DA9     [7:0]DAC_I[7:0]
  //RF_WT08(0x73, 0x02);                //DA10    [1:0]DAC_I[9:8]    [7:2]-
  #elif D_RATE == 2 // 2 Mbps
    // tx cannot set DA8[2]DAC_I_OW=1
    RF_WT08(0x71, 0x08);                //DA8     [3]  TPCDNK_BP=1 if need ch hopping
                                        //        [2]  DAC_I_OW=0
  //RF_WT08(0x72, 0x00);                //DA9     [7:0]DAC_I[7:0]
  //RF_WT08(0x73, 0x02);                //DA10    [1:0]DAC_I[9:8]    [7:2]-
  #endif
  #if   D_RATE == 0 // 250 kbps
    //
  #elif D_RATE == 1 // 1 Mbps
    RF_WT08(0x3D, 0x57);                //DA20
    RF_WT08(0x3E, 0x1D);                //DA21    [7]-  [6]DAC_R_CHCEN:DAC read channel enable    [5:0]DAC_R_CHCS: DAC read channel selection
  #elif D_RATE == 2 // 2 Mbps
    RF_WT08(0x3D, 0x57);                //DA20
    RF_WT08(0x3E, 0x1D);                //DA21    [7]-  [6]DAC_R_CHCEN:DAC read channel enable    [5:0]DAC_R_CHCS: DAC read channel selection
  #endif

//bank 2
    RF_WT08(0x80, 0x57);                //RXG
                                        //        [1:0]GAIN_LNA[1:0] 00:ultralow gain, 01:low gain, 10:middle gain, 11:high gain
                                        //        [2]  GAIN_MX       0:low gain, 1:high gain
                                        //        [4:3]GAIN_BPF[1:0] 00:12dB, 01:18dB, 10:24dB, 11:24dB
                                        //        [7:5]GAIN_PGA[2:0] 000:0dB, 001:6dB, 010:12dB, 011:18dB, 100:24dB, 101:30dB, 110/111:36dB
  //RF_WT08(0x81, 0x__);                //SX1     [7]  -
                                        //        [6:0]DN[ 6: 0]
  //RF_WT08(0x82, 0x__);                //SX2     [7:0]DK[ 7: 0]
  //RF_WT08(0x83, 0x__);                //SX3     [7:0]DK[15: 8]
    RF_WT08(0x84, 0x00);                //SX4     [3:0]DK[19:16]
                                        //        [4]  -
                                        //        [5]  TX_MORE_EN
                                        //        [6]  -
                                        //        [7]  DKN_TEST         0:DKN value from RF_CH,  1:DKN value from register table
    RF_WT08(0x85, 0xC1);                //CP1
    RF_WT08(0x86, 0xFF);                //CP2     [3:0]CX_M
                                        //        [7:4]IB_CP_M
  //RF_WT08(0x87, 0xCB);//2021may11     //CP3     [0]  ALPF_EN
  //RF_WT08(0x87, 0xA2);//2021          //CP3     [0]  ALPF_EN
    RF_WT08(0x87, 0x01);//BLE           //CP3     [0]  ALPF_EN
                                        //        [2:1]PZ
                                        //        [4:3]P3RD
                                        //        [7:5]DLY_SYN[2:0] 000:16us, 001:20us, 010:24us, 011:28us, 100:32us, 101:36us, 110:40us, 111:100us
    RF_WT08(0x88, 0x03);                //OD1     [5]  RESET_DSM: Reset DSM modulator
//  RF_WT08(0x89, 0x41);                //OD2     [7:6]DLY_SXPD   00:55us, 01:70us, 10:90us, 11:127us
    RF_WT08(0x89, 0x01);//debug         //OD2     [7:6]DLY_SXPD   00:55us, 01:70us, 10:90us, 11:127us

  //RF_WT08(0x92, 0xAE);                //92h     [0]  PAD_EN     TX PA driver enable
                                        //        [3:1]DRVL       Setting for TX low gain control stage, the setting of DRVG [3:0] should be 4'b0000
                                        //        [7:4]DRVG       Setting for TX gain control stage, the setting of DRVL [2:0] should be 3'b111
  //RF_WT08(0x93, 0x90);                //93h     [7:6]PAG        Gain setting for PA
                                        //        [5]  TXC_OW     Manual setting TX preamp and driver stage of PA
                                        //        [4:3]TXPRPG     Gain setting for driver stage of PA
                                        //        [2:0]TXPG       Gain setting for TX preamp stage
  //RF_WT08(0x94, 0x16);                //94h     [7]  FRAMP
                                        //        [6]  VRM
                                        //        [5:4]CT_PAI
                                        //        [3:2]CT_PRI
                                        //        [1:0]CT_PRIL
  //RF_WT08(0x95, 0x12);                //95h     [7]  TXCAL_EN
                                        //        [6]  TXCAL_DONE
                                        //        [5]  DETGSW
                                        //        [4:3]CT_PARD
                                        //        [2:0]DLY_PAD
    RF_WT08(0x96, 0x10);                //CA1
  //RF_WT08(0x96, 0x90);//calibrate     //CA1
                                        //        [7]  RTXAC_EN: RX/TX auto calibration enables. 0: disable, 1: auto calibration before each TX/RX packet
                                        //        [6:5]-
                                        //        [4:2]DLY_VCOCAL: Delay time for VCO calibration module, 000:1us, 001:2us, 010:3us, 011:4us, 100:5us, 101:6us, 110:7us, 111:20us
                                        //        [1]  CALBUF_EN: 
                                        //        [0]  VCOCAL_EN: VCO calibration module enable
    RF_WT08(0x97, 0x0A);                //CA2
    RF_WT08(0x98, 0x12);                //CA3  new[7]  FDCOC_EN
                                        //        [6:5]-
                                        //        [4:2]DLY_DCOCTB: DCOC turbo mode timing (switch RC) 000:0us, 001:2us, 010:4us, 011:5us, 100:6us, 101:7us, 110:8us, 111:10us
                                        //        [1]  DCOCTB_EN: DCOC turbo mode trigger as PWR_SOFT=1
                                        //        [0]  DCOC_ENB: RX DCOC (server loop) disable
    RF_WT08(0x99, 0x57);                //LD1

//  RF_WT08(0x9A, 0x17); //1.1          //LD2     [7:6]CT_DIGLDO  00:1.1V, 01:1.2V, 10:1.3V, 11:1.4V
//  RF_WT08(0x9A, 0x57); //1.2          //LD2     [7:6]CT_DIGLDO  00:1.1V, 01:1.2V, 10:1.3V, 11:1.4V
    RF_WT08(0x9A, 0x97); //1.3          //LD2     [7:6]CT_DIGLDO  00:1.1V, 01:1.2V, 10:1.3V, 11:1.4V
//  RF_WT08(0x9A, 0xD7); //1.4          //LD2     [7:6]CT_DIGLDO  00:1.1V, 01:1.2V, 10:1.3V, 11:1.4V

//  RF_WT08(0x9B, 0x03); // 00          //LD3     [7:6]IB_BG=00
    RF_WT08(0x9B, 0x43); // 01          //LD3     [7:6]IB_BG=01
//  RF_WT08(0x9B, 0x83); // 10          //LD3     [7:6]IB_BG=10
//  RF_WT08(0x9B, 0xC3); // 11          //LD3     [7:6]IB_BG=11

    RF_WT08(0x9C, 0x13);                //XO1
                                        //        [7]  XO_IL
                                        //        [6:5]-
                                        //        [4:0]XO_TRIM
    RF_WT08(0x9D, 0x00);                //XO2
                                        //        [7]  ADCLK_DIV2   0h: ADC input CLK = Fxtal
                                        //                          1h: ADC input CLK = Fxtal/2. Only use in Fxtal=32MHz and ADCLK=16MHz mode
                                        //        [6:5]-
                                        //        [4:3]             [4]XO_SEL  [3]XODIV2
                                        //                XO=32MHz        0       0        XCLK=32MHz
                                        //                XO=32MHz        0       1        XCLK=16MHz
                                        //                XO=16MHz        1       0        XCLK=16MHz
                                        //        [2:0]-
//bank 3
    RF_WT08(0xA0, 0x00);                //IO1     [7:4]GIO2S[3:0]=0h:-      [3:0]GIO1S[3:0]=0h:-
    RF_WT08(0xA1, 0x00);                //IO2     [7:4]GIO4S[3:0]=0h:-      [3:0]GIO3S[3:0]=0h:-
  //RF_WT08(0xA2, 0x7F);                //IO3    0[7]  SDO_TEN
                                        //       1[6]  SPIPU
                                        //        [5:0]GIOPU
//  RF_WT08(0xA3, 0x__);                //TEST1
//  RF_WT08(0xA4, 0x__);                //TEST2
//  RF_WT08(0xA5, 0x__);                //TEST3

  //RF_WT08(0xA6, 0x00     );           //TEST4   RTX_SEL = 0 => GIOTM_O = {1'b0 (out), ADC_DATA [9:0] (out), ADC_32M (out), ADC_16M (out)
  //RF_WT08(0xA6, 0x80|0x00);           //TEST4   RTX_SEL = 0 => GIOTM_O = {1'b0 (out), ADC_DATA [9:0] (out), ADC_32M (out), ADC_16M (out)
                                        //        RTX_SEL = 1 => GIOTM_O = {8'hFF (out), EMTXD_EN (in), EMTXD (in), TBCLK (out), TBCLKX32 (out), XCLK (out)
  //RF_WT08(0xA6, 0x80|0x02);//deadlock //TEST4   GIOTM_O = {TBCLK(out), TXD (out), BDATA_OUT (in), RBCLK (in), SAMPLE_EN (in), IRQ (out)
  //RF_WT08(0xA6, 0x80|0x05);           //TEST4   GIOTM_O = {
  //RF_WT08(0xA6, 0x80|0x06);//2022oct20//TEST4   GIOTM_O = {IRQ_RXDR_PRE(out), RF_CONTI(out), BLE_MODE(out), RX_CTE_EN(out), AESCCM_EN(out), CUR_LENGTH[7:0] (out)}
  //RF_WT08(0xA6, 0x80|0x07);           //TEST4   GIOTM_O = {7'd0 (out), RSTATE [3:0] (out), BDATA_OUT (out), RBCLK (out)
  //RF_WT08(0xA6, 0x80|0x08);           //TEST4   GIOTM_O = {IRQL, 2'b0, RN_STATE[3:0], RSTATE[3:0], BDATA_OUT_DM, RBIT_CLK_DM}
  //RF_WT08(0xA6, 0x80|0x09);           //TEST4   GIOTM_O = {RXP_RLEN, RXP_RS1, PUL_S0_END1, PUL_S0_END2, PUL_S0_END3, RXN_STATE[3:0], RXP_STATE[3:0]}
  RF_WT08(0xA6, 0x80|0x0B);           //TEST4   GIOTM_O = {4'd0 (out), CLK_RX_BIN(out), RXD_DM(out), PDM_OUT(out), ID_MATCH_OK(out), RBCLK(out), BDATA_OUT(out), CGATE_EN(out), DEMOD_EN(out), SAMPLE_EN(out)
  //RF_WT08(0xA6, 0x80|0x0C);           //TEST4   GIOTM_O = {
  //RF_WT08(0xA6, 0x80|0x15);           //TEST4   GIOTM_O = {1'b0 (out), IRQL (out), CCM_MAC_OK (out), AESCCM_TM_EN (out), CCM_CMP (out), CCM_EN (out), AES_CS[2:0] (out), CCM_CS[2:0] (out), XCLK (out)}
  //RF_WT08(0xA6, 0x80|0x1B);           //TEST4   GIOTM_O = {HADDR[3:0] (out), HSEL (out), RF_CTE_DACK3 (out), RF_CTE_NDREQ3 (out), RF_PLD_DACK2,...
  //RF_WT08(0xA6, 0x80|0x1C);           //TEST4   GIOTM_O = {HWDATA[7:0] (out), HREADY (out), HTRANS[1] (out), HWRITE (out), HSEL (out), HCLK (out)
  //RF_WT08(0xA6, 0x80|0x1D);           //TEST4   GIOTM_O = {HRDATA[7:0] (out), HREADY (out), HTRANS[1] (out), HWRITE (out), HSEL (out), HCLK (out)
  //RF_WT08(0xA6, 0x80|0x1F);           //TEST4   GIOTM_O = {5'd0 (out),OMST[2:0] (out), VCBOMST(out), RXOMST(out),TXOMST(out),XCLK(out),RSTB(out)
                                        //TEST4   [7]  GIOTM_EN=1  GIO test mode enable
                                        //        [6]  CALTME      ROSCi Selection
                                        //        [5]  -
                                        //        [4:0]GIOTMS
//  RF_WT08(0xA7, 0x__);                //TEST5
  //RF_WT08(0xAB, 0x07);                //MOD1    [7:0]DTR[7:0]    DTR[8:0]=0x001, Data Rate = Fxtal /16/(DTR+1) = 32000000/(16*(7+1)) = 250K
  //RF_WT08(0xAC, 0x02);                //MOD2    [7:5]-
                                        //        [4]  DITHER_EN
                                        //        [3:2]DITHER
                                        //        [1]  DTR_EN      DTR[8:0] Only used in Engineer Mode DTR_EN=1     D_RATE [1:0] setting must correct too
                                        //        [0]  DTR[8]
    RF_WT08(0xE0, 0x08);
  //RF_WT08(0xE0, 0x18);
                                        //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
    RF_WT08(0xE1, 0x07);//write 1 clear //E1h     [7:3]-
                                        //        [2]  IRQ_ANCHOR  Anchor point IRQ flag, write 1 to clear
                                        //        [1]  IRQ_B_SET   BLE setup    IRQ flag, write 1 to clear
                                        //        [0]  IRQ_WK_UP   Wake UP      IRQ flag, write 1 to clear
}

void init_2fpga_directed_test_mode(uint8_t tx_rx_select, uint8_t channel,uint8_t phy,uint8_t modulation_index,uint8_t expected_cte_length,uint8_t expected_cte_type)
{
    strobe_software_reset();            //1Fh     [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
    delay_zzzz();
//delay_unit625us(2);
    strobe_pdb_0_deepsleep();                   //PDB [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
    strobe_pdb_1_lightsleep_wait_xclk_ready();  //PDB [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
    
  //RF_WT08(0x17, 0x07);                //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
//strobe
  //  init_syncID();
  

                RF_WT08(0x00, 0x29);    //P0B0
                RF_WT08(0x01, 0x41);    //P0B1
                RF_WT08(0x02, 0x76);    //P0B2
                RF_WT08(0x03, 0x71);    //P0B3
                RF_WT08(0x04, 0x71);    //P0B4
    
    RF_WT08(0x0A, 0x19);                //TIMEOUT[7:0]  unit 25us
  //RF_WT08(0x0B, 0x__);                //TIMEOUT[15:8] unit 25us
  //RF_WT08(0x0C, 0x__);                //BC_CNT[7:0]
  //RF_WT08(0x0D, 0x__);                //BC_CNT[15:8]
  if( _HOST_TESTMODE_ == 1 ) // 0x00:master, 0x01:slave
  {
    RF_WT08(0x0E, master_DeviceAddr[0]);//WL1     White list ADDRESS[7:0]   master_DeviceAddr[0]
    RF_WT08(0x0F, master_DeviceAddr[1]);//WL2     White list ADDRESS[15:8]  master_DeviceAddr[1]
    RF_WT08(0x10, master_DeviceAddr[2]);//WL3     White list ADDRESS[23:16] master_DeviceAddr[2]
    RF_WT08(0x11, master_DeviceAddr[3]);//WL4     White list ADDRESS[31:24] master_DeviceAddr[3]
    RF_WT08(0x12, master_DeviceAddr[4]);//WL5     White list ADDRESS[39:32] master_DeviceAddr[4]
    RF_WT08(0x13, master_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] master_DeviceAddr[5]
//  RF_WT08(0x13, 0xC0|master_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] master_DeviceAddr[5]
  }
  else
  {
    RF_WT08(0x0E, slave_DeviceAddr[0]); //WL1     White list ADDRESS[7:0]   slave_DeviceAddr[0]
    RF_WT08(0x0F, slave_DeviceAddr[1]); //WL2     White list ADDRESS[15:8]  slave_DeviceAddr[1]
    RF_WT08(0x10, slave_DeviceAddr[2]); //WL3     White list ADDRESS[23:16] slave_DeviceAddr[2]
    RF_WT08(0x11, slave_DeviceAddr[3]); //WL4     White list ADDRESS[31:24] slave_DeviceAddr[3]
    RF_WT08(0x12, slave_DeviceAddr[4]); //WL5     White list ADDRESS[39:32] slave_DeviceAddr[4]
    RF_WT08(0x13, slave_DeviceAddr[5]); //WL6     White list ADDRESS[47:40] slave_DeviceAddr[5]
//  RF_WT08(0x13, 0xC0|slave_DeviceAddr[5]);//WL6     White list ADDRESS[47:40] slave_DeviceAddr[5]
  }
  
  //RF_WT08(0x14, 0x__);                //HB1 H_S0
  //RF_WT08(0x15, 0x__);                //HB2 H_LENGTH
  
  //RF_WT08(0x16, 0x00);                //HB3 H_S1  unused at ble
    RF_WT08(0x17, 0x07);                //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
  if(expected_cte_type != 0xFF)
  {
    RF_WT08(0x18, 0x88);   	    //HB4     [7:4]L_S0     bits of S0
				    //        [3:0]L_S1     bits of S1
  }	
  else
  {	
    RF_WT08(0x18, 0x80);              //HB4     [7:4]L_S0     bits of S0
                                    //        [3:0]L_S1     bits of S1
  }
    RF_WT08(0x19, 0x08);                //HB5     [3:0]L_LENGTH bits of LENGTH
                                        //                                           L_S0  L_LENGTH  L_S1
                                        //                                              0      6       3   ==> 24L01 (ENAA=1)
                                        //                                              0      0       0   ==> 24L01 (ENAA=0)
                                        //                                              8      8       0   ==> BLE
    RF_WT08(0x1A, 0x00);   //White list //FILTEREN[7]  -
                                        //        [6]  Enable Filtering on type_0010 ADV_NONCONN_IND
                                        //        [5]  Enable Filtering on type_0101 CONNECT_REQ
                                        //        [4]  Enable Filtering on type_0000 ADV_IND
                                        //        [3]  Enable Filtering on type_0001 ADV_DIRECT_IND
                                        //        [2]  Enable Filtering on type_0110 ADV_SCAN_IND
                                        //        [1]  Enable Filtering on type_0100 SCAN_RSP
                                        //        [0]  Enable Filtering on type_0011 SCAN_REQ
    RF_WT08(0x1B, 0x60|0x02);           //1Bh     [6:4]PIPEAP_SEL: pipe selection PRX write Ack Payload  0h:pipe0, 1h:pipe1, 2h:pipe2, 3h:pipe3 4h:pipe4, 5h:pipe5, 6h:idle
                                        //        [1:0]TXFIFO_SEL: 0h:txfifo_ack, 1h:txfifo_no_ack, 2h:idle
  ////////////////
  //RF_WT08(0x1C, 0x01);                //PDB     [7:1]-, [0]PDB_EN/RFPDB, 0:deep sleep mode, 1:light sleep mode
  //done above strobe_pdb_1_lightsleep_wait_xclk_ready()
  ////////////////
    RF_WT08(0x1D, channel);           //RF_CH   [7]-,   [6:0]RF_CH
    RF_WT08(0x1E, 0x00);                //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
//common bank
	//RF_WT08(0x30,0x00);
	RF_WT08(0x30, 0x40 | 0x00);
   // RF_WT08(0x30, CFG1_AGC_EN | 0x00);  //CFG1
                                        //      [1:0]BANK[1:0] control register Bank select, 00:bank0, 01 : bank1, 10 : bank2, 11 : bank3
                                        //      [3:2]-
                                        //      [4]DIR_EN   direct mode enable
                                        //      [5]PN9_EN   Enable TX output PN9 sequence
                                        //      [6]AGC_EN=0
                                        //      [7]-
  //RF_WT08(0x31, 0x90);                //RC1
                                        //      [7]  PWRON
                                        //      [6]  FSYCK_RDY  FSYCK ready flag
                                        //      [5]  XCLK_RDY   XCLK  ready flag
                                        //      [4]  XCLK_EN    Enable XCLK path to baseband functional block
                                        //      [3:2]FSYCK_DIV  FSYCK dividend from XCLK. 0h¡G1/1, 1h¡G1/2, 2h¡G1/4, 3h¡G1/8
                                        //      [1]  FSYCK_EN   FSYCK clock enable
                                        //      [0]-
    RF_WT08(0x32, CAR_EN_IRQ1|0x00);    //RC2
  //RF_WT08(0x32, CAR_EN_IRQ1|0x30);    //RC2
                                //only BLE Mode [7]  CAR_EN  Clear after Read enable       (note: CAR_EN used only for BLE Mode)
                                //only BLE Mode [6]  BCC_OW  0:reading BC_CNT get prev written BC_CNT, 1:reading BC_CNT get RF real value BC_CNT
                                        //      [5]  PSAVE1  XCLK clock gating enable for power saving mode 1
                                        //      [4]  PSAVE   XCLK clock gating enable for power saving mode 0
                                        //    RO[3:1](RO)RX_P_NO
                                        //    RO[0]  (RO)TX_FULL
    RF_WT08(0x33, 0x00);                //MASK  [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO,  [0]-
                                        //      [7]  MASK_CRCF
                                        //      [6]  MASK_RX
                                        //      [5]  MASK_TX
                                        //      [4]  MASK_MAX_RT
                                        //      [3]  MASK_ADDRESS
                                        //      [2]  MASK_BCMATCH
                                        //      [1]  MASK_RX_TO
                                        //      [0]  -
    clear_34h_irq1();                   //IRQ1  [7]CRCF, [6]RX_DR, [5]TX_DS, [4]MAX_RT, [3]ADDRESS, [2]BCMATCH, [1]RX_TO, [0]-
                                        //      [7]  CRCF
                                        //      [6]  RX_DR
                                        //      [5]  TX_DS
                                        //      [4]  MAX_RT
                                        //      [3]  ADDRESS
                                        //      [2]  BCMATCH
                                        //      [1]  RX_TO
                                        //      [0]-
////RF_WT08(0x35, 0x__);(Read Only)     //fifoSTATUS  (RO read only)
                                        //        [7:6]-
                                        //        [5]  TX_FULL
                                        //        [4]  TX_EMPTY
                                        //        [3:2]-
                                        //        [1]  RX_FULL
                                        //        [0]  RX_EMPTY
////RF_WT08(0x36, 0x__);(Read Only)     //RXDLEN  [7:6]-,  [5:0](RO)RXDLEN
    RF_WT08(0x37, 0x80);                //RT1 (04_SETUP_RETR)
                                        //        [7:4]ARD: auto retransmit delay
                                        //        [3:0]ARC: auto retransmit maximum times allowed
////RF_WT08(0x38, 0x__);(Read Only)     //RT2 (08_OBSERVE_TX)(Read-Only)
                                        //        [7:4](RO)CNT_PLOS
                                        //        [3:0](RO)CNT_ARC
                                        
//bank 0
  //RF_WT08(0x40, 0x__);                //OM
    RF_WT08(0x41, 0x2A);
    //RF_WT08(0x41, 0x0A);                //41h
                                        //        [7]  STEP_EN    Phase step enable. 0:disable, 1:enable
                                        //        [6]  AMBLE2     Preamble length selection   0:Preamble one byte,  1:Preamble two bytes
                                        //       1[5]  SKIP_PRE   Skip preamble detect in BLE mode
                                        //        [4]  MDCG_EN    Match detect clock-gating enable for DCLK2. 0h¡GDisable, 1h¡GEnable
                                        //        [3:0]RSSI_CTHD  RSSI threshold for carrier detection in -dBm   => (RSSI_CTHD * 2 + 1) + 74 = RSSI threshold for carrier detection
  if(phy == 0x01)
  {  	
    RF_WT08(0x42, PKT1_CRCLEN|0x20|0x01); //DM1 001:1Mbps,
  }
  else if(phy == 0x02)
  {  	
    RF_WT08(0x42, PKT1_CRCLEN|0x20|0x02); //DM1 010:2Mbps,
  }  
  else if(phy == 0x03)
  {
    RF_WT08(0x42, PKT1_CRCLEN|0x20|0x04); //DM1 100:125Kbps(LE code)
  }
   else if(phy == 0x04)
  {
    RF_WT08(0x42, PKT1_CRCLEN|0x20|0x05); //DM1 100:125Kbps(LE code)
  }
  else
  {
    RF_WT08(0x42, PKT1_CRCLEN|0x20|0x01); //DM1
  }
				        //42h
                                        //        [7:6]-
                                        //        [5:4]AW   11:5 bytes, 10:4 bytes, 01:3 bytes, 00:-
                                        //             Note¡Gsyncword length is force to 4 bytes In BLE mode
                                        //        [3]  -
                                        //        [2:0]D_RATE 0h:250Kbps, 1h:1Mbps, 2h:2Mbps, 4h:TX LE coded S=8, RX is auto switch by CI
                                        //                                                    5h:TX LE coded S=2, RX is auto switch by CI
                                        //                    others:illegal
                                        
      
    RF_WT08(0x43, 0x20);                //43h     [7:0]FD_HOLD
    RF_WT08(0x44, 0x80);                //44h     [7:0]PHASE_DSTEP
    RF_WT08(0x45, PKT1_CRC_ADD_EN|0x00);                //45h (unused in BLE mode)
  //RF_WT08(0x45, P5ACTIVE | P4ACTIVE | P3ACTIVE | P2ACTIVE | P1ACTIVE | P0ACTIVE); // (unused in BLE mode)
                                        //        [7]  CRC_ADD_EN     0:sync not include, 1:sync include
                                        //        [6]  CRC_TEST       Enable TX test format, CRC is 0x555555 for LE RF-PHY conformance testing using Direct Test Mode
                                        //        [5]  P5ACTIVE (unused in BLE mode)
                                        //        [4]  P4ACTIVE (unused in BLE mode)
                                        //        [3]  P3ACTIVE (unused in BLE mode)
                                        //        [2]  P2ACTIVE (unused in BLE mode)
                                        //        [1]  P1ACTIVE (unused in BLE mode)
                                        //        [0]  P0ACTIVE (unused in BLE mode)
  //RF_WT08(0x46, 0x__);(Read Only)     //STA1
                                        //      RO[7]  CD_FLAG Carrier Detection flag
                                        //        [6:3]-
                                        //      RO[2:0]OMST
  //RF_WT08(0x47, 0x__);(Read Only)     //RSSI2 RO[7:0]RSSI_NEGDB (RO read only)
  //RF_WT08(0x48, 0x__);(Read Only)     //RSSI3 RO[7:0]RSSI_ID_OK (RO read only)
   if(tx_rx_select ==0x00)	//DTM RX MODE
   {   
    RF_WT08(0x49, 0x00|0x03);
   }
   else if (tx_rx_select == 0x01) //DTM TX MODE
   {
    RF_WT08(0x49, 0x40|0x03);   
   }	
  //  RF_WT08(0x49, PKT1_CRC_ADD_EN | PKT1_CRCLEN);
                                        //PKT1    [7]  CRC_ADD_EN     0:sync not include, 1:sync include
                                        //        [6]  CRC_TEST       Enable TX test format, CRC is 0x555555 for LE RF-PHY conformance testing using Direct Test Mode
                                        //        [5:2]-
                                        //        [1:0]CRC_LENGTH   00:disabled, 01:CRC8, 10:CRC16, 11:CRC24
    RF_WT08(0x49, 0x00|0x53);		// whitening off
                                        //PKT2    [7]  WHT_EN, [6:0]WHTSD whitening seed
    RF_WT08(0x4A, (0<<3)|0x03);
                                        //PKT3    [7]  WHT_PCF_EN=0
                                        //        [6:3]DLY_TIFS  add delay in us, Rx to Tx , switch time is DLY_TXS + DLY_TIFS
                                        //        [2:0]DLY_CONTI RF continue mode delay time  TX: 0:8uS, 1:16uS, 2:32uS, 3:64uS, 4:96uS, 5:128uS, 6:192uS, 7:256uS
                                        //                                                    RX: 0:1uS, 1: 4uS, 2: 8uS, 3:12uS, 4:16uS, 5: 24uS, 6: 40uS, 7:128uS
    RF_WT08(0x4B, 0x05);                //PKT5    [7]  -
                                        //        [6:4]DLY_RXS             0:4us, 1: 8us, 2:12us, 3:16us, 4:20us, 5:32us, 6: 64us, 7:100us
                                        //        [3]  - 
                                        //        [2:0]DLY_TXS  nRF24 mode 0:10us,1:20us, 2:30us, 3:40us, 4:50us, 5:60us, 6: 70us, 7: 80us
                                        //                        BLE mode 0:40us,1:50us, 2:60us, 3:70us, 4:80us, 5:90us, 6:100us, 7:110us

    RF_WT08(0x4C, 0x5B);                //CRC1        CRC24  x24 + x10 + x9 + x6 + x4 + x3 + x + 1
    RF_WT08(0x4D, 0x06);                //CRC2
    RF_WT08(0x4E, 0x00);                //CRC3

    RF_WT08(0x4F, 0x55);                //CRC4 CRC_INI[ 7: 0] CRC initial value 
    RF_WT08(0x50, 0x55);                //CRC5 CRC_INI[15: 8] CRC initial value 
    RF_WT08(0x51, 0x55);                //CRC6 CRC_INI[23:16] CRC initial value 
    RF_WT08(0x52, 0x80|0x31);
  //RF_WT08(0x52, 0x__);                //RXPL    [7]  RXPL_EN    RX Payload Length Limit Enable
                                        //        [6:0]MAX_RXPL   MAX RX Payload Length Limit Setting
                                        //        if(RXPL_EN==1 && received_rxdlen > MAX_RXPL)
                                        //        then   HW treat received rxdlen as MAX_RXPL
                                        //                  finish receive payload MAX_RXPL bytes
                                        //                  result IRQ[7]CRCF

  if(expected_cte_type == 0x00)	//AOA
  {	
    RF_WT08(0x53, 0x00); //CTET  CTE Type  	
    //RF_WT08(0x53, expected_cte_type); //CTET  CTE Type
    RF_WT08(0x54, expected_cte_length); //CTEL: CTE Length  
    RF_WT08(0x5C, (RF_RD08(0x5C)&0x7F)|0x80); //5C [7]RX_ONE_LENGTH : 1 AoD        
  }
  else if(expected_cte_type == 0x01)	//AOD 1us slots	
  {
    RF_WT08(0x39, 0x51);
   // RF_WT08(0x39, 0xB6); //SHIFT_C1/C2 [3:0]¡GThey are parameters of phase lock loop	
    RF_WT08(0x46, 0x01); //CTE SLOT_DURATION 1us	
    RF_WT08(0x53, 0x00); //CTET  CTE Type
    RF_WT08(0x54, expected_cte_length); //CTEL: CTE Length     
    RF_WT08(0x5C, (RF_RD08(0x5C)&0x7F)|0x80); //5C [7]RX_ONE_LENGTH : 1 AoD  
    if(phy == 0x01)
    {
    	  RF_WT08(0x78, 0x50); //DELAY_RXCTE [7:0]¡GRX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x79, 0x80); //DELAY_RXCTE [7]       DELAY_TXCTE [6:0]¡GTX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x7A, 0x69); //DELAY_DEMODCTE [7:0]¡GDEMOD CTE initial delay time. Time unit = 1 XCLK clock cycle. 
    }
    else if(phy == 0x02)
    {
    	  RF_WT08(0x78, 0xA8); //DELAY_RXCTE [7:0]¡GRX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x79, 0x00); //DELAY_RXCTE [7]       DELAY_TXCTE [6:0]¡GTX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x7A, 0x34); //DELAY_DEMODCTE [7:0]¡GDEMOD CTE initial delay time. Time unit = 1 XCLK clock cycle.     	
    } 
  }
  else if(expected_cte_type == 0x02)	//AOD 2us slots
  {	
    RF_WT08(0x39, 0x51);
    //RF_WT08(0x39, 0xB6); //SHIFT_C1/C2 [3:0]¡GThey are parameters of phase lock loop		
    RF_WT08(0x46, 0x02); //CTE SLOT_DURATION 1us  	
    RF_WT08(0x53, 0x00); //CTET  CTE Type
    RF_WT08(0x54, expected_cte_length); //CTEL: CTE Length     
    RF_WT08(0x5C, (RF_RD08(0x5C)&0x7F)|0x80); //5C [7]RX_ONE_LENGTH : 1 AoD      
    if(phy == 0x01)
    {
    	  RF_WT08(0x78, 0x50); //DELAY_RXCTE [7:0]¡GRX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x79, 0x80); //DELAY_RXCTE [7]       DELAY_TXCTE [6:0]¡GTX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x7A, 0x69); //DELAY_DEMODCTE [7:0]¡GDEMOD CTE initial delay time. Time unit = 1 XCLK clock cycle. 
    }
    else if(phy == 0x02)
    {
    	  RF_WT08(0x78, 0xA8); //DELAY_RXCTE [7:0]¡GRX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x79, 0x00); //DELAY_RXCTE [7]       DELAY_TXCTE [6:0]¡GTX CTE initial delay time. Time unit = 1 XCLK clock cycle.
	  RF_WT08(0x7A, 0x34); //DELAY_DEMODCTE [7:0]¡GDEMOD CTE initial delay time. Time unit = 1 XCLK clock cycle.     	
    }	
  }
  else
  {
    RF_WT08(0x53, 0xFF); //CTET  CTE Type
    RF_WT08(0x54, 0x00); //CTEL: CTE Length
  }

//bank 1
   if( phy == 0x01) // 1 Mbps
   {
     RF_WT08(0x60, 0x2C); //AGC1    [1:0]AGC_CMP_THD    AGC comparison number threshold, 0:continuos AGC until SYNCWORD is detected, 1~3:comparison number thrshold
                                        //        [3:2]ZONE_OFFSET
                                        //        [6:4]OFFSET_SEL
                                        //        [7]  MPT_0DB_EN     Force max point = 0dB enable
   }
   else if(phy == 0x02)
   {
     RF_WT08(0x60, 0x2C); //AGC1    [1:0]AGC_CMP_THD    AGC comparison number threshold, 0:continuos AGC until SYNCWORD is detected, 1~3:comparison number thrshold
                                        //        [3:2]ZONE_OFFSET
                                        //        [6:4]OFFSET_SEL
                                        //        [7]  MPT_0DB_EN     Force max point = 0dB enable
   }
   else
   {
     RF_WT08(0x60, 0x2C);
   }

   			               
      RF_WT08(0x61, 0x79);                //AGC2
                                        //        [2:0]IFDET_OK_THD   IF detection OK threshold
                                        //        [7:3]OFFSET         RSSI Calculation Offset Setting
  //RF_WT08(0x62, 0x__);                //AGC3(RO read only)
                                        //        [2:0]AGC_ST         AGC state machine's state, 0x7:AGC_complete
                                        //        [3]-
                                        //        [7:4]GAIN_SEL       gain curve select 
   if( phy == 0x01) // 1 Mbps
   {
        RF_WT08(0x63, 0x14);             //AGC4 //        [7:0]GAIN_STB[7:0]
   }
   else if(phy == 0x02)
   {
	RF_WT08(0x63, 0x14);             //AGC4 //        [7:0]GAIN_STB[7:0]
   }
   else
   {
        RF_WT08(0x63, 0x14);             //AGC4 //        [7:0]GAIN_STB[7:0]
   }

    RF_WT08(0x64, 0x03);                //FCF1
                                        //        [7:4]MOD_INDEX2[3:0]
                                        //        [3:0]MOD_INDEX1[3:0]

    RF_WT08(0x65, 0xCC);                //FCF2
                                        //        [7:0]FSCALE[7:0]     todo

    RF_WT08(0x66, 0x1C);//IF 600K 32M xtal //FCF3        RX's Fscale(change IF) = IF * 2^20 / (2 * Fxtal) = (600000 * 2^20)/(2*32000000) =  9830.4    9830=0x2666   1CCC
                                        //        [7]-
                                        //        [6:0]FSCALE[14:8]    todo
    RF_WT08(0x67, 0x00);                //FCF4
                                        //        [7]  RXFD_EN        RX Frequency Deviation enable
                                        //        [6]  FMOD_TS
                                        //        [5:0]-
    RF_WT08(0x69, 0x00);		//CPA_DIG [2:0]¡GPA Matching Control.	DBFS_OFFSET[2:0]¡GDBFS fine tune in test mode

    RF_WT08(0xB0, 0x04);  
  	
   if( phy == 0x01) // 1 Mbps
   {
      RF_WT08(0xB1, 0x02);  		//AGC_BYPASS[1:0]	AGC_FLT_SEL	ABORT_TIME[4:0]
   }
   else if(phy == 0x02)
   {
   	RF_WT08(0xB1, 0x02);  //111
   }   
   else
   {
   	RF_WT08(0xB1, 0x02);  		//AGC_BYPASS[1:0]	AGC_FLT_SEL	ABORT_TIME[4:0]
   }                        
      RF_WT08(0x6A, 0x35);                //DA1     [7]TPCALF    [6]TPCAC_EN     [5:4]TPCAL_TC  [3:1]DLY_TPCAL   [0]TPCAL_EN

  if( phy == 0x01) // 1 Mbps
  {
    RF_WT08(0x71, 0x08);                //DA8     [3]  TPCDNK_BP=1 if need ch hopping
                                        //        [2]  DAC_I_OW=0
  //RF_WT08(0x72, 0x00);                //DA9     [7:0]DAC_I[7:0]
  //RF_WT08(0x73, 0x02);                //DA10    [1:0]DAC_I[9:8]    [7:2]-
  }
  else if(phy == 0x02) // 2 Mbps
  {
    RF_WT08(0x71, 0x08);                //DA8     [3]  TPCDNK_BP=1 if need ch hopping
                                        //        [2]  DAC_I_OW=0
  //RF_WT08(0x72, 0x00);                //DA9     [7:0]DAC_I[7:0]
  //RF_WT08(0x73, 0x02);                //DA10    [1:0]DAC_I[9:8]    [7:2]-
  }

  if( phy == 0x01) // 1 Mbps
  {
    RF_WT08(0x3D, 0x57);                //DA20
    RF_WT08(0x3E, 0x1D);                //DA21    [7]-  [6]DAC_R_CHCEN:DAC read channel enable    [5:0]DAC_R_CHCS: DAC read channel selection
  }
  else if(phy == 0x02) // 2 Mbps
  {
    RF_WT08(0x3D, 0x57);                //DA20
    RF_WT08(0x3E, 0x1D);                //DA21    [7]-  [6]DAC_R_CHCEN:DAC read channel enable    [5:0]DAC_R_CHCS: DAC read channel selection
  }

//bank 2
    RF_WT08(0x80, 0x2C);                //RXG
                                        //        [1:0]GAIN_LNA[1:0] 00:ultralow gain, 01:low gain, 10:middle gain, 11:high gain
                                        //        [2]  GAIN_MX       0:low gain, 1:high gain
                                        //        [4:3]GAIN_BPF[1:0] 00:12dB, 01:18dB, 10:24dB, 11:24dB
                                        //        [7:5]GAIN_PGA[2:0] 000:0dB, 001:6dB, 010:12dB, 011:18dB, 100:24dB, 101:30dB, 110/111:36dB
  //RF_WT08(0x81, 0x__);                //SX1     [7]  -
                                        //        [6:0]DN[ 6: 0]
  //RF_WT08(0x82, 0x__);                //SX2     [7:0]DK[ 7: 0]
  //RF_WT08(0x83, 0x__);                //SX3     [7:0]DK[15: 8]
    RF_WT08(0x84, 0x00);                //SX4     [3:0]DK[19:16]
                                        //        [4]  -
                                        //        [5]  TX_MORE_EN
                                        //        [6]  -
                                        //        [7]  DKN_TEST         0:DKN value from RF_CH,  1:DKN value from register table
    RF_WT08(0x85, 0xC1);                //CP1
    RF_WT08(0x86, 0xFF);                //CP2     [3:0]CX_M
                                        //        [7:4]IB_CP_M

    RF_WT08(0x87, 0x01);//BLE           //CP3     [0]  ALPF_EN
                                        //        [2:1]PZ
                                        //        [4:3]P3RD
                                        //        [7:5]DLY_SYN[2:0] 000:16us, 001:20us, 010:24us, 011:28us, 100:32us, 101:36us, 110:40us, 111:100us
    RF_WT08(0x88, 0x03);                //OD1     [5]  RESET_DSM: Reset DSM modulator

    RF_WT08(0x89, 0x01);//debug         //OD2     [7:6]DLY_SXPD   00:55us, 01:70us, 10:90us, 11:127us

  //RF_WT08(0x92, 0xAE);                //92h     [0]  PAD_EN     TX PA driver enable
                                        //        [3:1]DRVL       Setting for TX low gain control stage, the setting of DRVG [3:0] should be 4'b0000
                                        //        [7:4]DRVG       Setting for TX gain control stage, the setting of DRVL [2:0] should be 3'b111
  //RF_WT08(0x93, 0x90);                //93h     [7:6]PAG        Gain setting for PA
                                        //        [5]  TXC_OW     Manual setting TX preamp and driver stage of PA
                                        //        [4:3]TXPRPG     Gain setting for driver stage of PA
                                        //        [2:0]TXPG       Gain setting for TX preamp stage
  //RF_WT08(0x94, 0x16);                //94h     [7]  FRAMP
                                        //        [6]  VRM
                                        //        [5:4]CT_PAI
                                        //        [3:2]CT_PRI
                                        //        [1:0]CT_PRIL
  //RF_WT08(0x95, 0x12);                //95h     [7]  TXCAL_EN
                                        //        [6]  TXCAL_DONE
                                        //        [5]  DETGSW
                                        //        [4:3]CT_PARD
                                        //        [2:0]DLY_PAD
    RF_WT08(0x96, 0x10);                //CA1
  //RF_WT08(0x96, 0x90);//calibrate     //CA1
                                        //        [7]  RTXAC_EN: RX/TX auto calibration enables. 0: disable, 1: auto calibration before each TX/RX packet
                                        //        [6:5]-
                                        //        [4:2]DLY_VCOCAL: Delay time for VCO calibration module, 000:1us, 001:2us, 010:3us, 011:4us, 100:5us, 101:6us, 110:7us, 111:20us
                                        //        [1]  CALBUF_EN: 
                                        //        [0]  VCOCAL_EN: VCO calibration module enable
    RF_WT08(0x97, 0x0A);                //CA2
    RF_WT08(0x98, 0x12);                //CA3  new[7]  FDCOC_EN
                                        //        [6:5]-
                                        //        [4:2]DLY_DCOCTB: DCOC turbo mode timing (switch RC) 000:0us, 001:2us, 010:4us, 011:5us, 100:6us, 101:7us, 110:8us, 111:10us
                                        //        [1]  DCOCTB_EN: DCOC turbo mode trigger as PWR_SOFT=1
                                        //        [0]  DCOC_ENB: RX DCOC (server loop) disable
    RF_WT08(0x99, 0x57);                //LD1


    RF_WT08(0x9A, 0x97); //1.3          //LD2     [7:6]CT_DIGLDO  00:1.1V, 01:1.2V, 10:1.3V, 11:1.4V



    RF_WT08(0x9B, 0x43); // 01          //LD3     [7:6]IB_BG=01


    RF_WT08(0x9C, 0x13);                //XO1
                                        //        [7]  XO_IL
                                        //        [6:5]-
                                        //        [4:0]XO_TRIM
    RF_WT08(0x9D, 0x00);                //XO2
                                        //        [7]  ADCLK_DIV2   0h: ADC input CLK = Fxtal
                                        //                          1h: ADC input CLK = Fxtal/2. Only use in Fxtal=32MHz and ADCLK=16MHz mode
                                        //        [6:5]-
                                        //        [4:3]             [4]XO_SEL  [3]XODIV2
                                        //                XO=32MHz        0       0        XCLK=32MHz
                                        //                XO=32MHz        0       1        XCLK=16MHz
                                        //                XO=16MHz        1       0        XCLK=16MHz
                                        //        [2:0]-
//bank 3
    RF_WT08(0xA0, 0x00);                //IO1     [7:4]GIO2S[3:0]=0h:-      [3:0]GIO1S[3:0]=0h:-
    RF_WT08(0xA1, 0x00);                //IO2     [7:4]GIO4S[3:0]=0h:-      [3:0]GIO3S[3:0]=0h:-
  //RF_WT08(0xA2, 0x7F);                //IO3    0[7]  SDO_TEN
                                        //       1[6]  SPIPU
                                        //        [5:0]GIOPU
//  RF_WT08(0xA3, 0x__);                //TEST1
//  RF_WT08(0xA4, 0x__);                //TEST2
//  RF_WT08(0xA5, 0x__);                //TEST3

  //RF_WT08(0xA6, 0x00     );           //TEST4   RTX_SEL = 0 => GIOTM_O = {1'b0 (out), ADC_DATA [9:0] (out), ADC_32M (out), ADC_16M (out)
  //RF_WT08(0xA6, 0x80|0x00);           //TEST4   RTX_SEL = 0 => GIOTM_O = {1'b0 (out), ADC_DATA [9:0] (out), ADC_32M (out), ADC_16M (out)
                                        //        RTX_SEL = 1 => GIOTM_O = {8'hFF (out), EMTXD_EN (in), EMTXD (in), TBCLK (out), TBCLKX32 (out), XCLK (out)
  //RF_WT08(0xA6, 0x80|0x02);//deadlock //TEST4   GIOTM_O = {TBCLK(out), TXD (out), BDATA_OUT (in), RBCLK (in), SAMPLE_EN (in), IRQ (out)

  RF_WT08(0xA6, 0x80|0x05);           //TEST4   GIOTM_O = {
  //RF_WT08(0xA6, 0x80|0x06);//2022oct20//TEST4   GIOTM_O = {IRQ_RXDR_PRE(out), RF_CONTI(out), BLE_MODE(out), RX_CTE_EN(out), AESCCM_EN(out), CUR_LENGTH[7:0] (out)}
  //RF_WT08(0xA6, 0x80|0x07);           //TEST4   GIOTM_O = {7'd0 (out), RSTATE [3:0] (out), BDATA_OUT (out), RBCLK (out)
  //RF_WT08(0xA6, 0x80|0x08);           //TEST4   GIOTM_O = {IRQL, 2'b0, RN_STATE[3:0], RSTATE[3:0], BDATA_OUT_DM, RBIT_CLK_DM}
  //RF_WT08(0xA6, 0x80|0x09);           //TEST4   GIOTM_O = {RXP_RLEN, RXP_RS1, PUL_S0_END1, PUL_S0_END2, PUL_S0_END3, RXN_STATE[3:0], RXP_STATE[3:0]}
//   RF_WT08(0xA6, 0x80|0x0B);           //TEST4   GIOTM_O = {4'd0 (out), CLK_RX_BIN(out), RXD_DM(out), PDM_OUT(out), ID_MATCH_OK(out), RBCLK(out), BDATA_OUT(out), CGATE_EN(out), DEMOD_EN(out), SAMPLE_EN(out)
   //RF_WT08(0xA6, 0x80|0x10); 
  //RF_WT08(0xA6, 0x80|0x0C);           //TEST4   GIOTM_O = {
  //RF_WT08(0xA6, 0x80|0x15);           //TEST4   GIOTM_O = {1'b0 (out), IRQL (out), CCM_MAC_OK (out), AESCCM_TM_EN (out), CCM_CMP (out), CCM_EN (out), AES_CS[2:0] (out), CCM_CS[2:0] (out), XCLK (out)}
  //RF_WT08(0xA6, 0x80|0x1B);           //TEST4   GIOTM_O = {HADDR[3:0] (out), HSEL (out), RF_CTE_DACK3 (out), RF_CTE_NDREQ3 (out), RF_PLD_DACK2,...
  //RF_WT08(0xA6, 0x80|0x1C);           //TEST4   GIOTM_O = {HWDATA[7:0] (out), HREADY (out), HTRANS[1] (out), HWRITE (out), HSEL (out), HCLK (out)
  //RF_WT08(0xA6, 0x80|0x1D);           //TEST4   GIOTM_O = {HRDATA[7:0] (out), HREADY (out), HTRANS[1] (out), HWRITE (out), HSEL (out), HCLK (out)
  //RF_WT08(0xA6, 0x80|0x1F);           //TEST4   GIOTM_O = {5'd0 (out),OMST[2:0] (out), VCBOMST(out), RXOMST(out),TXOMST(out),XCLK(out),RSTB(out)
                                        //TEST4   [7]  GIOTM_EN=1  GIO test mode enable
                                        //        [6]  CALTME      ROSCi Selection
                                        //        [5]  -
                                        //        [4:0]GIOTMS
//  RF_WT08(0xA7, 0x__);                //TEST5
  //RF_WT08(0xAB, 0x07);                //MOD1    [7:0]DTR[7:0]    DTR[8:0]=0x001, Data Rate = Fxtal /16/(DTR+1) = 32000000/(16*(7+1)) = 250K
  //RF_WT08(0xAC, 0x02);                //MOD2    [7:5]-
                                        //        [4]  DITHER_EN
                                        //        [3:2]DITHER
                                        //        [1]  DTR_EN      DTR[8:0] Only used in Engineer Mode DTR_EN=1     D_RATE [1:0] setting must correct too
                                        //        [0]  DTR[8]
    RF_WT08(0xE0, 0x08);
  //RF_WT08(0xE0, 0x18);
                                        //E0h     [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
                                        //        [6:5]-
                                        //        [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
                                        //        [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
                                        //        [2]  EN_ANCHOR   Anchor Point IRQ enable
                                        //        [1]  EN_B_SET    BLE Setup    IRQ enable
                                        //        [0]  EN_WK_UP    Wake UP      IRQ enable
    RF_WT08(0xE1, 0x07);//write 1 clear //E1h     [7:3]-
                                        //        [2]  IRQ_ANCHOR  Anchor point IRQ flag, write 1 to clear
                                        //        [1]  IRQ_B_SET   BLE setup    IRQ flag, write 1 to clear
                                        //        [0]  IRQ_WK_UP   Wake UP      IRQ flag, write 1 to clear
}

/**
 *  @ingroup BC3601_Host_FSM1_private_by_Holtek_label
 */


////////////////////////////////////////

void debug_foo_000s(void)
{
    //
        //nrf51822\nRF51_SDK_10.0.0\examples\ble_peripheral\ble_app_uart - (Advertiser)
        uint8_t      adv_data_nRF51_SDK_examples_ble_peripheral_ble_app_uart[0x10] = {
            0x02, 0x01, 0x05,
            //   Flags
            0x0C, 0x09, 0x53, 0x65, 0x61, 0x6e, 0x33, 0x46, 0x30, 0x55, 0x41, 0x52, 0x54
            //   DevName   S     e     a     n     3     F     0     U     A     R     T      Sean3F0UART
        };
        hcicmd_2008_LE_set_adv_data_TypeDef cmd2008 =
        {
            .adv_data_length = 0x10,
        //  .adv_data[]
        };
            ht_memory_copy( cmd2008.adv_data+0, adv_data_nRF51_SDK_examples_ble_peripheral_ble_app_uart+0, cmd2008.adv_data_length );
        hcmd_2008_le_set_adv_data((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd2008));
    //
        //nrf51822\nRF51_SDK_10.0.0\examples\ble_peripheral\ble_app_uart - (Advertiser)
        uint8_t scanresp_data_nRF51_SDK_examples_ble_peripheral_ble_app_uart[0x12] = {
            0x11, 0x07, 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E 
        };
        hcicmd_2009_LE_set_scanresp_data_TypeDef cmd2009 =
        {
            .scanresp_data_length = 0x12,
        //  .scanresp_data[]
        };
            ht_memory_copy( cmd2009.scanresp_data+0, scanresp_data_nRF51_SDK_examples_ble_peripheral_ble_app_uart+0, cmd2009.scanresp_data_length );
        hcmd_2009_le_set_scanresp_data((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd2009));
    //
}

void debug_foo_001(LEACL_TypeDef *pacl)
{
    pacl->flag1.field.Tx_aesccm_enabled = 0;//debug
    pacl->flag1.field.Rx_aesccm_enabled = 0;//debug

    pacl->TxPacketCounter.field.packetCounter = 0;
    pacl->RxPacketCounter.field.packetCounter = 0;

  #if   _CODE_VOL6_PARTC_1_2_ == 1              //1. START_ENC_RSP1 (packet 0, M --> S)
    pacl->TxPacketCounter.field.packetCounter = 0;
    pacl->RxPacketCounter.field.packetCounter = 0;
  #elif _CODE_VOL6_PARTC_1_2_ == 2              //2. START_ENC_RSP2 (packet 0, S --> M)
    pacl->TxPacketCounter.field.packetCounter = 0;
    pacl->RxPacketCounter.field.packetCounter = 0;
  #elif _CODE_VOL6_PARTC_1_2_ == 3              //3. Data packet1 (packet 1, M --> S)
    pacl->TxPacketCounter.field.packetCounter = 1;
    pacl->RxPacketCounter.field.packetCounter = 1;
  #endif
}

void debug_foo_002(LEACL_TypeDef *pacl)
{
//  uint8_t iii;
  //
        return;//debug
    #if AESCCM_LL_SIGNALING == 1
    if (pacl->flag1.field.Tx_aesccm_enabled == 0 ||
        pacl->flag1.field.Rx_aesccm_enabled == 0 ||
        LLC_INITIATE_PROCEDURE_is_active(pacl) ||
        LLC_RESPOND_PROCEDURE_is_active(pacl) )
    {
        return;
    }
    #endif
  //
  /*
    for (iii=0; iii<1; iii++)
    {
      //if( 0 == RINGBUF_isFull (pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ ) )
        if(      RINGBUF_isEmpty(pacl->lldataTxQ) )
        {
            uint16_t *pU16;
          //if ( rand() & 0x01 )
            if (1)
            {
            receive_02_hciAclDataPacket_fromhost(LEACL_conn_handle(pacl),
                                                 *(pU16 = (uint16_t *)(vol6_partc_1_2_3_plaindata+0)) + 4,//0x001B, length
                                                                       vol6_partc_1_2_3_plaindata+0
                                                );
                #if 1
                {
                      vol6_partc_1_2_3_plaindata[0] ++;         //L2CAP length
                  if( vol6_partc_1_2_3_plaindata[0] > (0x17) )  //L2CAP length
                //if( vol6_partc_1_2_3_plaindata[0] > (0xF7) )  //L2CAP length
                      vol6_partc_1_2_3_plaindata[0] = 0;
                    //                          [1] same 0      //L2CAP length
                    //                          [2] same 0x03   //L2CAP channel ID
                      vol6_partc_1_2_3_plaindata[3] ++;         //L2CAP channel ID
                  uint16_t i;
                  for( i=4; i<256; i++)
                      vol6_partc_1_2_3_plaindata[i] ++;         //L2CAP data
                }
                #endif
            }
            else
            {
            receive_02_hciAclDataPacket_fromhost(LEACL_conn_handle(pacl),
                                                 *(pU16 = (uint16_t *)(vol6_partc_1_2_4_plaindata+0)) + 4,//0x001B, length
                                                                       vol6_partc_1_2_4_plaindata+0
                                                );
                #if 1
                {
                      vol6_partc_1_2_4_plaindata[0] ++;         //L2CAP length
                  if( vol6_partc_1_2_4_plaindata[0] > (0x17) )  //L2CAP length
                //if( vol6_partc_1_2_4_plaindata[0] > (0xF7) )  //L2CAP length
                      vol6_partc_1_2_4_plaindata[0] = 0;
                    //                          [1] same 0      //L2CAP length
                    //                          [2] same 0x04   //L2CAP channel ID
                      vol6_partc_1_2_4_plaindata[3] ++;         //L2CAP channel ID
                  uint16_t i;
                  for( i=4; i<256; i++)
                      vol6_partc_1_2_4_plaindata[i] ++;         //L2CAP data
                }
                #endif
            }
        }
    }
  */
  /*
    for (iii=0; iii<1; iii++)
    {
      //if( 0 == RINGBUF_isFull (pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ ) )
        if(      RINGBUF_isEmpty(pacl->llcTxQ) )
        {
          //TxLLcPDU_06_start_enc_rsp(pacl);    //1. START_ENC_RSP1 (packet 0, M --> S)
                                                //2. START_ENC_RSP2 (packet 0, S --> M)
          //TxLLcPDU_08_feature_req(pacl);
          //TxLLcPDU_0c_version_ind(pacl);
            TxLLcPDU_12_ping_req(pacl);
        }
    }
  */
}

void debug_foo_003m(LEACL_TypeDef *pacl)
{
  //#if AESCCM_LL_SIGNALING == 1
        hcicmd_2019_LE_enable_encryption_TypeDef cmd =
        {
          //.long_term_key[0]
        };
            ht_write_hword( (uint8_t *)&(cmd.conn_handle), LEACL_conn_handle(pacl));
            ht_memory_copy( cmd.long_term_key+0, vol6_partc_1_LTK_lso_to_mso+0, 16);
        hcmd_2019_le_enable_encryption((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd));
      //TxLLcPDU_08_feature_req(pacl); //debug debug_send_m2s_1()
  //#endif
}

////////////////////////////////////////////////////
#define ST_0                                    0x00
#define ST_IDLE                                 0x01
#define ST_Sean_IDLE				0xFF

#define ST_TRT_SETTING                          0x40      // TRT:Advertiser TRT event
#define ST_TRT_SET_T1                           0x41      //
#define ST_TRT_W4_CLOSE_EVENT                   0x42      //

#define ST_RTR_SETTING                          0x50      // RTR:Scanner RTR event
#define ST_RTR_SET_R1                           0x51      //
#define ST_RTR_W4_CLOSE_EVENT                   0x52      //

#define ST_SLA_SETUP_TXWINDOW_SET_RX            0x60      // SLA:Slave Connection Event
#define ST_SLA_ANCHOR_SET_RX                    0x61      //
#define ST_SLA_W4_CLOSE_EVENT                   0x62      //

#define ST_MAS_SETUP_TXWINDOW_SET_TX            0x70      // MAS:Master Connection Event
#define ST_MAS_ANCHOR_SET_TX                    0x71      //
#define ST_MAS_W4_CLOSE_EVENT                   0x72      //

#define ST_DTM_TX_SETTING                       0x80
#define ST_DTM_TX_SET_T_IND                     0x81
#define ST_DTM_TX_STROBE			0x82
#define ST_DTM_TX_W4_CLOSE_EVENT		0x83

#define ST_DTM_RX_SETTING                       0x90
#define ST_DTM_RX_STROBE			0x91
#define ST_DTM_RX_W4_CLOSE_EVENT		0x92
#define ST_DTM_RX_CONTINUED			0x93


#define ST_ext_adv_TX_INIT			     0xA0
#define ST_ext_adv_TX_SETTING_ADV_EXT_IND	     0xA1
#define ST_ext_adv_TX_ADV_EXT_IND_W4_CLOSE_EVENT     0xA2
#define ST_ext_adv_TX_SETTING_AUX_ADV_IND            0xA3
#define ST_ext_adv_TX_AUX_ADV_IND_W4_CLOSE_EVENT     0xA4
#define ST_ext_adv_TX_SETTING_AUX_SYNC_IND           0xA5
#define ST_ext_adv_TX_AUX_SYNC_IND_W4_CLOSE_EVENT    0xA6
void debug_hcfsm_state_ST_0(void)
{
    hcfsm.state = ST_0;
}
void debug_hcfsm_state_ST_Sean_IDLE(void)
{
    RF_WT08(0x1F, 0x10);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]RFRST, [3:2]-, [1]RXM_START, [0]TXM_START
                                    // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically	
    hcfsm.state = ST_Sean_IDLE;
    
}
void bc5602b_hcfsm(LEACL_TypeDef *pacl)
{
    bool reloop;
    do //reloop
    {
        reloop = 0;
    switch(hcfsm.state)
    {
    case ST_Sean_IDLE:
    
    	pacl = (LEACL_TypeDef *)dllistPeek(&leACLlist);
    	if(pacl != NULL)
    	{
    		  // htqueueReset((htQueue_TypeDef *)(&leACLlist));
    		   dllistDeleteNode( (dllist_TypeDef *)(&leACLlist), (dllist_node_TypeDef *)pacl );
    	}
	hcfsm.state = ST_0;
    	break;	    	
    case ST_0:
            if (LEACL_is_role_master(pacl)) {
uart_puts("5602BLE  PRX Master  CH="); uart_putu8(RFCHANNEL); uart_putchar_n('\n');
                hcfsm.state = ST_RTR_SETTING;
            }
            else
            if (LEACL_is_role_slave(pacl)) {
uart_puts("5602BLE  PTX Slave  CH="); uart_putu8(RFCHANNEL); uart_putchar_n('\n');
                TRT_delay_offset = tmr625usGetCurrentTicks();
                hcfsm.state = ST_TRT_SETTING;
            }
            else
            if (LEACL_is_role_dtm_tx(pacl)){
            	hcfsm.state = ST_DTM_TX_SETTING;
            }
            else
            if (LEACL_is_role_dtm_rx(pacl)){
            	hcfsm.state = ST_DTM_RX_SETTING;
            }
            if (LEACL_is_role_ext_adv_tx(pacl)){         	
            	 if(pacl->ext_adv_enable == 0x01)
            	 {
           	 hcfsm.state = ST_ext_adv_TX_INIT;
           	 }
            }
        break;       
    case ST_TRT_W4_CLOSE_EVENT:
            if( irq1isr_TRT_advertising_event_is_closed() )
            {
//pacl->flag1.field.roger_connectind = 0; //debug__  repeat test send connect_ind
                if( pacl->flag1.field.roger_connectind == 1) {
                    send_04_hciEvent_3e_01_LE_connection_complete(pacl);
                    hcfsm.state = ST_SLA_SETUP_TXWINDOW_SET_RX; reloop = 1;
                }
                else {
                    TRT_delay_offset = tmr625usGetCurrentTicks();
                    hcfsm.state = ST_TRT_SETTING;
                }
            }
        break;
    case ST_RTR_W4_CLOSE_EVENT:
            if( irq1isr_RTR_scanning_event_is_closed() )
            {
//pacl->flag1.field.sent_connectind = 0;//debug________ repeat test send connect_ind
                if( pacl->flag1.field.sent_connectind == 1) //debug
                {
                    send_04_hciEvent_3e_01_LE_connection_complete(pacl);//tmp debug
                  //send_04_hciEvent_57_authenticated_payload_timeout_expired(LEACL_conn_handle(pacl));//tmp debug
                  #if 0
                  {
                    //master initiate
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_VERSION_EXCHANGE_PROCEDURE,   //send LL_0C_VERSION_IND, wait LL_0C_VERSION_IND
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_FEATURE_EXCHANGE_PROCEDURE,   //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
                    /*
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_DATA_LENGTH_UPDATE_PROCEDURE, //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
                    */
                  }
                  #endif
//debug_foo_003m(pacl);
                    hcfsm.state = ST_MAS_SETUP_TXWINDOW_SET_TX; reloop = 1;
                }
                else
                {
                    hcfsm.state = ST_RTR_SETTING;
                }
            }
        break;
    case ST_MAS_W4_CLOSE_EVENT:
            if( irq1isr_master_connection_event_is_closed() )
            {
                if ( (LLC_INITIATE_PROCEDURE_is_active(pacl) && LLC_INITIATE_PROCEDURE_is_expired(pacl, 32000)) || //  20 sec / 625us = 32000
                     (LLC_RESPOND_PROCEDURE_is_active (pacl) && LLC_RESPOND_PROCEDURE_is_expired (pacl, 32000)) )  //  20 sec / 625us = 32000
                {
                    //LL/CON/MAS/BV-19-C [Conn Control Timeout]             todo optional send_04_hciEvent_3e_04_LE_read_remote_feature_complete() Reason: 0x22
                    //LL/CON/MAS/BI-04-C [Master Conn Control Timer]        todo optional send_04_hciEvent_0c_read_remote_version_info_complete()  Reason: 0x22
                    //LL/CON/SLA/BI-05-C [Slave Conn Control Timer]         todo optional send_04_hciEvent_0c_read_remote_version_info_complete()  Reason: 0x22
                    //LL/CON/MAS/BI-05-C [Initiate Conn Param Req - Timeout]todo optional send_04_hciEvent_3e_03_LE_conn_update_complete()         Reason: 0x22
                    //LL/CON/SLA/BI-07-C [Initiate Conn Param Req - Timeout]todo optional send_04_hciEvent_3e_03_LE_conn_update_complete()         Reason: 0x22
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x22 ); //ERR_22_LL_RESPONSE_TIMEOUT
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.rxLLTerminateInd_txAck == 0x02 ) //0x02:finish send ACK for rx LL_02_TERMINATE_IND
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x13 ); //ERR_13_OTHER_END_TERMINATED_CONN_USER_END
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x02 )  //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x16 ); //ERR_16_CONN_TERMINATED_BY_LOCAL_HOST
                        hcfsm.state = ST_IDLE;
                    }
                }
    //
                else
                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x01 &&  //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
                    pacl->debug_consecutive_times_rxto >= 5 ) //debug tmp
                    //todo: The initial value for timer T_Terminate shall be set to the value of the connSupervisionTimeout
                {
                    //LL/CON/MAS/BI-02-C [Master T_Terminate Timer]  Reason: 0x16 or 0x22
                    //LL/CON/SLA/BI-02-C [Slave T_Terminate Timer]   Reason: 0x16 or 0x22
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x22 ); //ERR_22_LL_RESPONSE_TIMEOUT
                        hcfsm.state = ST_IDLE;
                    }
                }
    //
    //
                else
                if( pacl->debug_consecutive_times_rxto >= 20 ) //debug tmp
              //if( pacl->debug_consecutive_times_rxto >=  4 ) //debug tmp
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x08 ); //ERR_08_CONN_TIMEOUT
                        hcfsm.state = ST_IDLE;

                    }
                }
    //
                else {
                    hcfsm.state = ST_MAS_ANCHOR_SET_TX; reloop = 1;
                }
                /////////////////////////////////////////////////////////////////debug
                if( pacl->flag1.field.conn_setup_success_syncword_match == 1 ) //debug
                {
                    static volatile uint8_t done=0; //debug
                    if (done == 0) //debug
                    {
        	            done = 1;
    	                uint8_t i;
    	              //for (i=0; i<1; i++) {
    	                for (i=0; i<TOTAL_NUM_ELEMENTS_llcTxQ; i++) {
                          /* debug
                            llcmsgQ_push( LLCMSG_INITIATE_LLC_FEATURE_EXCHANGE_PROCEDURE,   //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
                                         LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                                         0,//length
                                         (uint8_t *)0
                                       );
                          */
    	                }
    	                for (i=0; i<TOTAL_NUM_ELEMENTS_lldataTxQ; i++) {
                      //    debug_send_m2s_1(pacl); //debug
    	                }
                          /* debug
                            llcmsgQ_push( LLCMSG_INITIATE_LLC_CHANNEL_MAP_UPDATE_PROCEDURE,   //send LL_01_CHANNEL_MAP_IND
                                         LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                                         0,//length
                                         (uint8_t *)0
                                       );
                          */
                    }
                }
            }
        break;
    case ST_SLA_W4_CLOSE_EVENT:
            if( irq1isr_slave_connection_event_is_closed() )
            {
                if ( (LLC_INITIATE_PROCEDURE_is_active(pacl) && LLC_INITIATE_PROCEDURE_is_expired(pacl, 32000)) || //  20 sec / 625us = 32000
                     (LLC_RESPOND_PROCEDURE_is_active (pacl) && LLC_RESPOND_PROCEDURE_is_expired (pacl, 32000)) )  //  20 sec / 625us = 32000
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x22 ); //ERR_22_LL_RESPONSE_TIMEOUT
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.rxLLTerminateInd_txAck == 0x02 ) //0x02:finish send ACK for rx LL_02_TERMINATE_IND
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x13 ); //ERR_13_OTHER_END_TERMINATED_CONN_USER_END
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x02 )  //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
                {
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x16 ); //ERR_16_CONN_TERMINATED_BY_LOCAL_HOST
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.txLLTerminateInd_w4ack == 0x01 &&  //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
                    pacl->debug_consecutive_times_rxto >= 5 ) //debug tmp
                    //todo: The initial value for timer T_Terminate shall be set to the value of the connSupervisionTimeout
                {
                    //LL/CON/MAS/BI-02-C [Master T_Terminate Timer]  Reason: 0x16 or 0x22
                    //LL/CON/SLA/BI-02-C [Slave T_Terminate Timer]   Reason: 0x16 or 0x22
                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x22 ); //ERR_22_LL_RESPONSE_TIMEOUT
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->debug_consecutive_times_rxto >= 20 ) //debug tmp
              //if( pacl->debug_consecutive_times_rxto >=  4 ) //debug tmp
                {

                    if( hcmsgQ_push( HCMSG_DELETE_LEACL_FREE_MEMORY, 
                                     LEACL_conn_handle(pacl), 
                                     0,//length
                                     (uint8_t *)0
                                   )
                      ) {
                        send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x08 ); //ERR_08_CONN_TIMEOUT
                        hcfsm.state = ST_IDLE;
                    }
                }
                else
                if( pacl->flag1.field.conn_setup_success_syncword_match == 0 ) {
                    hcfsm.state = ST_SLA_SETUP_TXWINDOW_SET_RX; reloop = 1;
                }
                else {
                    hcfsm.state = ST_SLA_ANCHOR_SET_RX; reloop = 1;
                }
            }
        break;
    ////////////////////
    case ST_TRT_SETTING:
            if ( tmr625usIsExpired(TRT_delay_offset, 70) )
            {
init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00();//debug
                hcfsm.state = ST_TRT_SET_T1;
            }
        break;
    case ST_TRT_SET_T1:
        {
////set_PRX_mode_prmrx_1(); //[5]PRM_RX unused in BLE mode
////set_PTX_mode_prmrx_0(); //[5]PRM_RX unused in BLE mode
            if (1)
            {
              //
                change_channel_index++; if(change_channel_index < 37) change_channel_index=37;
                                        if(change_channel_index > 39) change_channel_index=37;
//change_channel_index=39;//debug
                    init_0rf_2in1_5602B_set_rfch(RfCenterFreq[change_channel_index]);
                    RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[change_channel_index]);
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
            BLETIMER_TypeDef adv_anchor;
                adv_anchor.cntLoIn625us.reg = RF_RD32(0xF0)   //F0h~F3h: NUM_CNT_LO
                                              + 7;
                adv_anchor.phaseIn1us.reg = 0;//0x0200;
                irq1isr_open_TRT_advertising_event(pacl,
                                                   adv_anchor,
                                                   txAdvHeaderS0.field.type
                                                  );
                hcfsm.state = ST_TRT_W4_CLOSE_EVENT;
        }
        break;

    case ST_SLA_SETUP_TXWINDOW_SET_RX:
        {
            if (1)
            {
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
            }
                pacl->TxPacketCounter.field.directionBit  = 0; //0:Slave to Master, 1:Master to Slave
                pacl->RxPacketCounter.field.directionBit  = 1; //0:Slave to Master, 1:Master to Slave
//
//debug_foo_001(pacl);
//
                uint32_t rxto_25us;
                uint32_t winWideningInUs;
                BLETIMER_TypeDef sla_setup_anchor; 
                // Slave set rx connPacket timeout
                    rxto_25us = ppm_x_sinceLastAnchorIn1250us_to_slaveRXTO25us( cem_ppm + ces_ppm, 
                                                                                (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                                              )
                                + ( ((uint32_t)pacl->currParam_winSize) * 50 ) //TIMEOUT[15:0] unit 25us, but winSize unit 1.25 ms        1250/25=50
                                + (92);          // 92*25us = 2300 us > 2270 us         (Startup 150 + connPacket 2120) = (150 + (1+4+2+255+3)*8)
                if (rxto_25us > 0xFFFF) {
                    rxto_25us = 0xFFFF; //todo: TIMEOUT[15:0] unit 25us, but pNewUpdate->winSize unit 1.25 ms
                }
                RF_WT16(0x0A, rxto_25us);           //0x0A TIMEOUT[7:0]  unit 25us   slave setup CE anchor
                                                    //0x0B TIMEOUT[15:8] unit 25us
                  //
                    sla_setup_anchor.cntLoIn625us.reg = pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo
                                            #if   D_RATE == 0 //250 kbps
                                                        + 2 // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #elif D_RATE == 1 //1 Mbps
                                                        + 1 // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #elif D_RATE == 2 //2 Mbps
                                                        + 1 // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #endif
                                                        + 2 // transmitWindowDelay 1.25ms
                                                        + 2 * ((uint32_t)(pacl->currParam_winOffset))  // winOffset unit 1.25ms
                                                        + 2 * ((uint32_t)(pacl->currParam.interval ) * pacl->iSlave_ce1stRx_lost_rxto_consecutive_times);
                  //
                    winWideningInUs = ppm_x_sinceLastAnchorIn1250us_to_winWideningInUs( cem_ppm + ces_ppm,
                                                                                        (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                                                      );
                    uint32_t winWideningDiv625;
                    uint16_t winWideningMod625;
                    uint16_t earlierInUs;       //slave rx IRQ ANCHOR must be earlier than master tx IRQ ANCHOR
                             winWideningDiv625 = winWideningInUs / 625;
                             winWideningMod625 = winWideningInUs % 625;
                             earlierInUs       = winWideningMod625
                                                 + (625-312)  // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                                 + 100;  //extra   <<<<<<<<<<<<<<<<<<<<<<<<
                    sla_setup_anchor.cntLoIn625us.reg -= winWideningDiv625 ;
                    sla_setup_anchor.cntLoIn625us.reg -=(earlierInUs / 625);
                                                         earlierInUs %= 625;
                    if (                                  pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase > earlierInUs ) {
                        sla_setup_anchor.phaseIn1us.reg = pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase - earlierInUs ;
                    }
                    else {
                        sla_setup_anchor.cntLoIn625us.reg --;
                        sla_setup_anchor.phaseIn1us.reg = ((610-1) + pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.field.phase) - earlierInUs ;
                        //                                  610 = 30.517578125 * 20   in case 32768 LOCLK
                    }
                irq1isr_open_slave_connection_event(pacl,
                                                    sla_setup_anchor
                                                   );
  #if 0 //debug print to comport
  {
    /*
    uint32_t diff;
             diff = sla_setup_anchor.cntLoIn625us.reg - pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.reg;
                           uart_putu8(diff);
    uart_puts(",");        uart_putu32(pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.reg);
    uart_puts(",");        uart_putu16(pacl->ceAnchor_connectInd_get_F4h_F8h.phaseIn1us.reg);
    uart_puts(",  anch="); uart_putu32(sla_setup_anchor.cntLoIn625us.reg);
    uart_puts(",");        uart_putu16(sla_setup_anchor.phaseIn1us.reg);
  //uart_puts(",");        uart_putu8(RF_RD08(0xE0)); //MASK_ANCHOR, EN_TEST_LO. EN_B_T. EN_ANCHOR. EN_B_SET, EN_WK_UP
  //uart_puts(",");        uart_putu32(RF_RD32(0xF4));//F4h ~ F7h: CNT_LO_ADDR
    uart_putchar_n('\n');
    */
    uint8_t octet[0x22];
    ht_memory_set ( octet+0 ,  0x00, 0x22 );
    ht_memory_copy( octet+0 ,  (uint8_t *)&rxConnIndPayload, 0x22);
    send_04_hciEvent_3e_ff_LE_kidd_debug(0x22, octet+0);  //0x22:length
    //
  }
  #endif
                hcfsm.state = ST_SLA_W4_CLOSE_EVENT;
        }
        break;
    case ST_SLA_ANCHOR_SET_RX:
        {
            if (1)
            {
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
            }
                uint32_t rxto_25us;
                uint32_t winWideningInUs;
                BLETIMER_TypeDef sla_anchor;
                // Slave set rx connPacket timeout
                if ( ( pacl->pNewUpdate != 0 ) &&
                     ((pacl->pNewUpdate->iSlave_updateState == 0x00 && pacl->pNewUpdate->instant == pacl->connEventCount) || //first CE
                      (pacl->pNewUpdate->iSlave_updateState == 0x01) //not yet rx matched address
                     )
                   )
                {
                    rxto_25us = ppm_x_sinceLastAnchorIn1250us_to_slaveRXTO25us( cem_ppm + ces_ppm, 
                                                                                  (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                                                  ////instant
                                                                                + (uint32_t)(pacl->pNewUpdate->winOffset)
                                                                                + (uint32_t)(pacl->pNewUpdate->interval ) * pacl->pNewUpdate->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger
                                                                              )
                                + (92);  // 92*25us = 2300 us > 2270 us         (Startup 150 + connPacket 2120) = (150 + (1+4+2+255+3)*8)
                }
                else
                {
                    rxto_25us = ppm_x_sinceLastAnchorIn1250us_to_slaveRXTO25us( cem_ppm + ces_ppm, 
                                                                                (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                                              )
                                + (92);  // 92*25us = 2300 us > 2270 us         (Startup 150 + connPacket 2120) = (150 + (1+4+2+255+3)*8)
                }
rxto_25us += (3*(625/25));//debug bletimer
                if (rxto_25us > 0xFFFF) {
                    rxto_25us = 0xFFFF; //todo: TIMEOUT[15:0] unit 25us, but pNewUpdate->winSize unit 1.25 ms
                }
                RF_WT16(0x0A, rxto_25us);           //0x0A TIMEOUT[7:0]  unit 25us   slave CE anchor
                                                    //0x0B TIMEOUT[15:8] unit 25us
//
//debug_foo_002(pacl);
//
                if ( ( pacl->pNewUpdate != 0 ) &&
                     ((pacl->pNewUpdate->iSlave_updateState == 0x00 && pacl->pNewUpdate->instant == pacl->connEventCount) || //first CE
                      (pacl->pNewUpdate->iSlave_updateState == 0x01) //not yet rx matched address
                     )
                   )
                {
                  //
                    sla_anchor.cntLoIn625us.reg = pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.field.cntlo
                                                  + 2 * ((uint32_t)(pacl->currParam.interval   ) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1))
                                                  + 2 * ((uint32_t)(pacl->pNewUpdate->winOffset))  // winOffset unit 1.25ms
                                                  + 2 * ((uint32_t)(pacl->pNewUpdate->interval ) * (pacl->pNewUpdate->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger) );
                  //
                    winWideningInUs = ppm_x_sinceLastAnchorIn1250us_to_winWideningInUs( cem_ppm + ces_ppm,
                                                            (uint32_t)(pacl->currParam.interval   ) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                          + (uint32_t)(pacl->pNewUpdate->interval ) * (pacl->pNewUpdate->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger+1)
                                                                                      );
                    pacl->pNewUpdate->iSlave_updateState = 0x01; //not yet rx matched address
                }
                else
                {
                  //
                    sla_anchor.cntLoIn625us.reg = pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.field.cntlo
                                                  + 2 * (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1);
                  //
                    winWideningInUs = ppm_x_sinceLastAnchorIn1250us_to_winWideningInUs( cem_ppm + ces_ppm,
                                                            (uint32_t)(pacl->currParam.interval) * (pacl->iSlave_ce1stRx_lost_rxto_consecutive_times+1)
                                                                                      );
                    ///////////////////////////
                    if ((pacl->pNewUpdate != 0) &&
                        (pacl->pNewUpdate->iSlave_updateState == 0x02) ) //already ever rx matched address
                    {
                        //Conn Update procedure is complete when the instant has passed, and the new CE parameters have been applied
                        if (LLC_RESPOND_PROCEDURE_is_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND)) {
                            LLC_RESPOND_PROCEDURE_end_complete(pacl); //complete RespondProcedure, clear state upon the Connection Update instant has passed
                        }
                                    #if    __MALLOC_METHOD__  == 1
                                         free( (void *)pacl->pNewUpdate ); //slave free pNewUpdate
                                    #elif  __MALLOC_METHOD__  == 2
                                    vPortFree( (void *)pacl->pNewUpdate ); //slave free pNewUpdate
                                    #endif
                                             pacl->pNewUpdate = 0;         //slave free pNewUpdate
                        send_04_hciEvent_3e_03_LE_conn_update_complete(pacl, 0x00); //ERR_00_SUCCESS
                    }
                }
                    uint32_t winWideningDiv625;
                    uint16_t winWideningMod625;
                    uint16_t earlierInUs;       //slave rx IRQ ANCHOR must be earlier than master tx IRQ ANCHOR
                             winWideningDiv625 = winWideningInUs / 625;
                             winWideningMod625 = winWideningInUs % 625;
                             earlierInUs       = winWideningMod625
                                            #if   D_RATE == 0 //250 kbps
                                                 +(8<<2)  //preamble  8 bits
                                                 +(32<<2) //address   4 bytes = 32 bits
                                                 + 100;   //extra   <<<<<<<<<<<<<<<<<<<<<<<<
                                            #elif D_RATE == 1 //1 Mbps
                                                 + 8      //preamble  8 bits
                                                 + 32     //address   4 bytes = 32 bits
                                                 + 50;    //extra   <<<<<<<<<<<<<<<<<<<<<<<<
                                            #elif D_RATE == 2 //2 Mbps
                                                 +(8>>1)  //preamble  8 bits
                                                 +(32>>1) //address   4 bytes = 32 bits
                                                 + 50;    //extra   <<<<<<<<<<<<<<<<<<<<<<<<
                                            #endif
                    sla_anchor.cntLoIn625us.reg -= winWideningDiv625 ;
                    sla_anchor.cntLoIn625us.reg -=(earlierInUs / 625);
                                                   earlierInUs %= 625;
                    if (                            pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase > earlierInUs ) {
                        sla_anchor.phaseIn1us.reg = pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase - earlierInUs ;
                    }
                    else {
                        sla_anchor.cntLoIn625us.reg --;
                        sla_anchor.phaseIn1us.reg = ((610-1) + pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.field.phase) - earlierInUs ;
                        //                            610 = 30.517578125 * 20   in case 32768 LOCLK
                    }
                irq1isr_open_slave_connection_event(pacl,
                                                    sla_anchor
                                                   );
//sla_anchor.phaseIn1us.reg=1;//debug
//if (sla_anchor.phaseIn1us.reg == 1) sla_anchor.phaseIn1us.reg=2;//debug
  #if 0 //debug print to comport
  {
    uint32_t diff;
             diff = sla_anchor.cntLoIn625us.reg - pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.reg;
                           uart_putu8(diff);
    uart_puts(",");        uart_putu32(pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.reg);
    uart_puts(",");        uart_putu16(pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.reg);
    uart_puts(",  anch="); uart_putu32(sla_anchor.cntLoIn625us.reg);
    uart_puts(",");        uart_putu16(sla_anchor.phaseIn1us.reg);
  //uart_puts(",");        uart_putu8(RF_RD08(0xE0)); //MASK_ANCHOR, EN_TEST_LO. EN_B_T. EN_ANCHOR. EN_B_SET, EN_WK_UP
  //uart_puts(",");        uart_putu32(RF_RD32(0xF4));//F4h ~ F7h: CNT_LO_ADDR
    uart_putchar_n('\n');
  }
  #endif
                hcfsm.state = ST_SLA_W4_CLOSE_EVENT;
        }
        break;
    //////////////////////////////////////
    case ST_RTR_SETTING:
////set_PRX_mode_prmrx_1(); //[5]PRM_RX unused in BLE mode
////set_PTX_mode_prmrx_0(); //[5]PRM_RX unused in BLE mode
init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00();//debug
                hcfsm.state = ST_RTR_SET_R1;
        break;
    case ST_RTR_SET_R1:
        {
            if (1)
            {
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
            }
            BLETIMER_TypeDef scan_anchor;
                scan_anchor.cntLoIn625us.reg = RF_RD32(0xF0)   //F0h~F3h: NUM_CNT_LO
                                               + 5;
                scan_anchor.phaseIn1us.reg = 0x0200;
                irq1isr_open_RTR_scanning_event(pacl,
                                                scan_anchor
                                               );
                hcfsm.state = ST_RTR_W4_CLOSE_EVENT;
        }
        break;
    case ST_MAS_SETUP_TXWINDOW_SET_TX:
        {
            if (1)
            {
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
            }
                pacl->TxPacketCounter.field.directionBit  = 1; //0:Slave to Master, 1:Master to Slave
                pacl->RxPacketCounter.field.directionBit  = 0; //0:Slave to Master, 1:Master to Slave
//
//debug_foo_001(pacl);
//
                BLETIMER_CNT_LO_625US_TypeDef cemEchAnch_setup;
                BLETIMER_PHASE_1US_TypeDef    cemE2hFine_setup;
                cemEchAnch_setup.reg = pacl->ceAnchor_connectInd_get_F4h_F8h.cntLoIn625us.field.cntlo
                                            #if   D_RATE == 0 //250 kbps
                                       + 2  // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #elif D_RATE == 1 //1 Mbps
                                       + 1  // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #elif D_RATE == 2 //2 Mbps
                                       + 1  // CONNECT_IND 312us from sync_tail to crc_tail,  1+4+(2+34)+3=44 bytes:352us,  (2+34)+3=39 bytes:312us
                                            #endif
                                       + 2  // transmitWindowDelay 1.25ms
                                   //  + 2 *((uint32_t)(pacl->currParam_winSize  ) / 5)  // debug test: master anchor at middle of winSize
                                       + 2 * (uint32_t)(pacl->currParam_winOffset);      // winOffset unit 1.25ms
                cemEchAnch_setup.reg += 2;  //master assume E2h NUM_FINE=0, need Ech Anchor += 1 at least, here += 2
                cemE2hFine_setup.reg = 0;   //master assume E2h NUM_FINE=0
  #if 0 //debug print to comport
  {
    uart_puts("setup_anch="); uart_putu32(cemEchAnch_setup.reg);
    uart_puts(",");           uart_putu16(cemE2hFine_setup.reg);
    uart_putchar_n('\n');
  }
  #endif
                pacl->masAnchor.cntLoIn625us.reg = cemEchAnch_setup.reg;
                pacl->masAnchor.phaseIn1us.reg   = cemE2hFine_setup.reg;
                irq1isr_open_master_connection_event(pacl,
                                                     pacl->masAnchor
                                                    );
                hcfsm.state = ST_MAS_W4_CLOSE_EVENT;
        }
        break;
    case ST_MAS_ANCHOR_SET_TX:
        {
            if (1)
            {
                pacl->connEventCount ++; // 16-bit, shall be set to zero on the first connection event, shall wrap from 0xFFFF to 0x0000
                channel_selection_algorithm_1_leacl_currChannel(pacl);
                init_0rf_2in1_5602B_set_rfch( pacl->channel );
            }
//
//debug_foo_002(pacl);
//
                BLETIMER_CNT_LO_625US_TypeDef cemEchAnch;
                BLETIMER_PHASE_1US_TypeDef    cemE2hFine;
                if ((pacl->pNewUpdate != 0) &&                                     //master instant
                    (pacl->pNewUpdate->instant == pacl->connEventCount) )          //master instant
                {
                    cemEchAnch.reg = pacl->masAnchor.cntLoIn625us.field.cntlo
                                     + 2 * ( (uint32_t)(pacl->currParam.interval)                  // prev_anchor  + OLD interval = instant    interval unit 1.25ms
                                                      ////instant------------------------------------------------------------instant
                                                      ////+1 conn parameter Update dont have 1.25ms transmitWindowDelay
                                           + (uint32_t)(pacl->pNewUpdate->winOffset)
                                           + (uint32_t)(pacl->pNewUpdate->iMaster_deltaT_minus_winOffset)   // debug test: master anchor at middle of winSize
                                           );
                    //------------------------------
                            pacl->currParam.interval = pacl->pNewUpdate->interval; //master copy pNewUpdate->interval to currParam
                            pacl->currParam.latency  = pacl->pNewUpdate->latency ; //master copy pNewUpdate->latency  to currParam
                            pacl->currParam.timeout  = pacl->pNewUpdate->timeout ; //master copy pNewUpdate->timeout  to currParam
                             #if    __MALLOC_METHOD__  == 1
                                kidd_taskENTER_CRITICAL();
                                  free( (void *)pacl->pNewUpdate );                //master free pNewUpdate
                                kidd_taskEXIT_CRITICAL();
                             #elif  __MALLOC_METHOD__  == 2
                             vPortFree( (void *)pacl->pNewUpdate );                //master free pNewUpdate
                             #endif
                                                pacl->pNewUpdate = 0;              //master free pNewUpdate
                    //Conn Update procedure is complete when the instant has passed, and the new CE parameters have been applied
                    if (LLC_INITIATE_PROCEDURE_is_active(pacl) &&
                        LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND) )
                    {
                        send_04_hciEvent_3e_03_LE_conn_update_complete(pacl, 0x00); //ERR_00_SUCCESS
                        LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon the Connection Update instant has passed
                    }
                }
                else
                {
                    cemEchAnch.reg = pacl->masAnchor.cntLoIn625us.field.cntlo
                                     + 2 * (uint32_t)(pacl->currParam.interval); // interval unit 1.25ms
                }
                    cemE2hFine.reg = 0;
  #if 0 //debug print to comport
  {
  //uint32_t diff;
  //         diff = cemEchAnch.reg - pacl->masAnchor.cntLoIn625us.reg;
  //uart_puts(",");        uart_putu32(diff);
    uart_puts(",");        uart_putu32(pacl->ceAnchor_1stconnM2S_get_F4h_F8h.cntLoIn625us.reg);
    uart_puts(",");        uart_putu16(pacl->ceAnchor_1stconnM2S_get_F4h_F8h.phaseIn1us.reg);
    uart_puts(",  anch="); uart_putu32(cemEchAnch.reg);
    uart_puts(",");        uart_putu16(cemE2hFine.reg);
  //uart_puts(",");        uart_putu8(RF_RD08(0xE0)); //MASK_ANCHOR, EN_TEST_LO. EN_B_T. EN_ANCHOR. EN_B_SET, EN_WK_UP
    uart_putchar_n('\n');
  }
  #endif
                pacl->masAnchor.cntLoIn625us.reg = cemEchAnch.reg;
                pacl->masAnchor.phaseIn1us.reg   = cemE2hFine.reg;
                irq1isr_open_master_connection_event(pacl,
                                                     pacl->masAnchor
                                                    );
                hcfsm.state = ST_MAS_W4_CLOSE_EVENT;
        }
        break;
///////////////////////////////////////////////////////////////

    case ST_DTM_TX_SETTING:
    	        init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00();//debug                
                set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-                             
                RF_WT08(0x0A, 0xFF);                  //TIMEOUT[7:0]    							
		RF_WT08(0x0B, 0xFF);                  //TIMEOUT[15:8]  	        
                RF_WT08(0x33, 0x00);                //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-                           
                clear_34h_irq1();   
      
                hcfsm.state = ST_DTM_TX_SET_T_IND;
        break;        
    case ST_DTM_TX_SET_T_IND: 
               
                clear_34h_irq1();    
                pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1                	        		
		
		if(pacl->dtm_tx_aoa_mode == 0x01) //DTM tx AoA
		{
		  set_txHeader_txPayload_DTM_tx_aoa_packet(leconfig_txTest.packet_payload,  leconfig_txTest.test_data_length, (uint8_t *)&(leconfig_txTestData.advData), pacl->cte_type, pacl->cte_length);			
		}
		else
		{
		  set_txHeader_txPayload_DTM_tx_packet(leconfig_txTest.packet_payload,  leconfig_txTest.test_data_length, (uint8_t *)&(leconfig_txTestData.advData));                                                  
        	}
        	
                static BLETIMER_TypeDef dtm_tx_anchor;               
                dtm_tx_anchor.cntLoIn625us.field.cntlo = RF_RD32(0xF0)+1; //F0h~F3h: NUM_CNT_LO                                              
                dtm_tx_anchor.phaseIn1us.reg = 0;//0x0200;               

                hcfsm.state = ST_DTM_TX_STROBE;               
    	break;  
    case ST_DTM_TX_STROBE:
                                      
               	irq1isr_open_DTM_advertising_event(pacl,dtm_tx_anchor);
    		 
                hcfsm.state = ST_DTM_TX_W4_CLOSE_EVENT;
    	break;	
    case ST_DTM_TX_W4_CLOSE_EVENT:
       	   if( irq1isr_DTM_TX_event_is_closed() )
            {     
            	  pdma_ch2_bleTx_setting__aesccmEN0(CH2TXADDR_TRT_T1);//TRT event T1   
            	               
		  if(pacl->dtm_tx_aoa_mode == 0x01) //DTM tx AoA
		  {
		    set_txHeader_txPayload_DTM_tx_aoa_packet(leconfig_txTest.packet_payload,  leconfig_txTest.test_data_length, (uint8_t *)&(leconfig_txTestData.advData), pacl->cte_type, pacl->cte_length);			
		    dtm_tx_anchor.cntLoIn625us.reg=dtm_tx_anchor.cntLoIn625us.reg+2;   //F0h~F3h: NUM_CNT_LO     TX_AOA_INTERVAL = 625 *(2) us 
		  }
		  else
		  {
		    set_txHeader_txPayload_DTM_tx_packet(leconfig_txTest.packet_payload,  leconfig_txTest.test_data_length, (uint8_t *)&(leconfig_txTestData.advData));                                                  
        	    dtm_tx_anchor.cntLoIn625us.reg=dtm_tx_anchor.cntLoIn625us.reg+1;   //F0h~F3h: NUM_CNT_LO     maxPDUlength37byte_TX_INTERVAL = 625 *(1) us   
        	  }                 
                  
                                            
                  dtm_tx_anchor.phaseIn1us.reg = 0;//0x0200;
                  hcfsm.state = ST_DTM_TX_STROBE ;                  
            }
	break;	
    
        
///////////////////////////////////////////////////////////////
   case ST_DTM_RX_SETTING:
    {
    		//init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00();//debug
    		//set_disabledTxen0_disabledRxen1();  
		set_disabledTxen0_disabledRxen0();	
	        RF_WT08(0x33, 0x00); 		 //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-
                
                RF_WT08(0xE0, 0x00 | 0x00 | 0x00);                
                clear_34h_irq1(); 
                
		if(pacl->dtm_rx_aod_mode == 0x00)
		{ 
			pdma_ch2_bleRx_setting__aesccmEN0(CH2RXADDR_RTR_R1);//RTR event R1 
		}	
		else if(pacl->dtm_rx_aod_mode == 0x01)
		{
			pdma_ch2and3_bleRx_aod_setting__aesccmEN0(CH2RXADDR_RTR_R1,CH2RXADDR_RTR_R2_SCANRSP); 
		}                                	 
              		                                       	
                hcfsm.state = ST_DTM_RX_STROBE;
                         
          break; 
    }  
   case ST_DTM_RX_STROBE:   
    {
    		RF_WT08(0x0A, 0x19);                //TIMEOUT[7:0]  unit 25us
               // irq1isr_open_DTM_scanning_event(pacl, scan_anchor  );	                          			
                   irq1isr_open_DTM_scanning_event(pacl);                       	   
             	   strobe_RX();
                hcfsm.state = ST_DTM_RX_W4_CLOSE_EVENT;
         break; 
     }    
   case ST_DTM_RX_CONTINUED:   
    {
    	//while(!IS_BUFFER_EMPTY(UR0TxReadIndex, UR0TxWriteIndex)) {;}
    	while(!((RF_RD08(0x35)&0x80)==0x80)) {;}   //L2_IDLE_FF
		if(pacl->dtm_rx_aod_mode == 0x00)
		{ 
			pdma_ch2_bleRx_setting__aesccmEN0(CH2RXADDR_RTR_R1);//RTR event R1 
		}	
		else if(pacl->dtm_rx_aod_mode == 0x01)
		{
			pdma_ch2and3_bleRx_aod_setting__aesccmEN0(CH2RXADDR_RTR_R1,CH2RXADDR_RTR_R2_SCANRSP); 
		}
		
    		strobe_RX(); 
    		//debug_RFLA_pulse_to_trigger_LA();
               // irq1isr_open_DTM_scanning_event(pacl, scan_anchor  );	                          			
                irq1isr_open_DTM_scanning_event(pacl);                       	   
                hcfsm.state = ST_DTM_RX_W4_CLOSE_EVENT;
         break; 
     }       
   case ST_DTM_RX_W4_CLOSE_EVENT: 
     {
    	    if( irq1isr_DTM_RX_event_is_closed() )
            {             	                 
                  hcfsm.state = ST_DTM_RX_CONTINUED;               
            }
            if( irq1isr_DTM_RX_RXDR_CTE_event_is_closed() )
            {
                   if(pacl->dtm_rx_aod_mode == 0x01)
            	   {

            	     read_ctepld_and_formatting(pacl->cte_sample_count,iq_sample+0);  
            	     send_04_hciEvent_3e_15_LE_connectionless_iq_report(pacl->channelIndex,pacl->cte_type,pacl->cte_sample_count,iq_sample+0);          	         	
            	   }     
            	  //  RF_WT08(0x1F, 0x10);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]RFRST, [3:2]-, [1]RXM_START, [0]TXM_START        	             	                 
                  hcfsm.state = ST_DTM_RX_CONTINUED;               
            }

	break;   
     }
////////////////////////////////////////////////////////////////////////////////////     
   case ST_ext_adv_TX_INIT:
    
    		init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00();//debug
		set_disabledTxen0_disabledRxen0();	
	        RF_WT08(0x33, 0x00); 		 //MASK [7]MASK_CRCF, [6]MASK_RX, [5]MASK_TX, [4]MASK_MAX_RT, [3]MASK_ADDRESS, [2]MASK_BCMATCH, [1]MASK_RX_TO, [0]-             
                clear_34h_irq1(); 
               // write_00h_AccessAddress(P0_ACCESS_ADDR);
               // write_50h_CRCInit_555555();
                change_channel_index = 36;                          
                pacl->advEventCount = 0; 
                pacl->periodicEventCount = 0;
                pacl->ADInfo_DID=0;
                pacl->ExtAdvEventAnchor.cntLoIn625us.reg = RF_RD32(0xF0) ;
                pacl->currChM.chM[0]=      0xFF;        //ChM (5 octets) channel map
   	        pacl->currChM.chM[1]=      0xFF;        //ChM (5 octets) channel map
                pacl->currChM.chM[2]=      0xFF;        //ChM (5 octets) channel map
                pacl->currChM.chM[3]=      0xFF;        //ChM (5 octets) channel map
                pacl->currChM.chM[4]=      0x1F; 
                //pacl->PeriodicAdvEventAnchor.cntLoIn625us.reg =                                 	               		                                       	
                hcfsm.state = ST_ext_adv_TX_SETTING_ADV_EXT_IND;                        
          break; 
    
   case ST_ext_adv_TX_SETTING_ADV_EXT_IND:                 
                change_channel_index++; if(change_channel_index < 37) change_channel_index=37;
                                        if(change_channel_index > 39) change_channel_index=37;

                init_0rf_2in1_5602B_set_rfch(RfCenterFreq[change_channel_index]);
                RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[change_channel_index]);

                BLETIMER_TypeDef adv_anchor;
                if(change_channel_index == 37)
                {
                	
                	  pacl->channelIndex = SecondaryAdvChannelSelectionRandom(pacl->currChM.chM+0);
                	  //pacl->channelIndex = SecondaryAdvChannelSelectionRandom(0x1E00E00600);
                	  //uart_puts("channelIndex= ");uart_putu8(pacl->channelIndex);uart_puts(" ; ");uart_putchar_n('\n');
                	  //pacl->channelIndex = ChannelSelectionAlgorithm2(0x8E89BED6,pacl->advEventCount,0x1E00E00600);
                	//pacl->channelIndex = ChannelSelectionAlgorithm2(0x8E89BED6,pacl->advEventCount,0x1FFFFFFFFF);            		          		
                	 //pacl->ExtAdvEventAnchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg + leconfig_ExtAdvParam.primary_adv_interval_min;
                
                	 pacl->ExtAdvEventAnchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg        +0x000000A0;
			 pacl->AuxAdvEventAnchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg   	    +24;   
			 pacl->PeriodicAdvEventAnchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg   +36;                	 
                	 irq1isr_open_EXT_advertising_event(pacl,pacl->ExtAdvEventAnchor,PDUTYPE_ADV_EXT_IND);   
                	 pacl->advEventCount++;
             		 pacl->ADInfo_DID++;               	
               	}
               	else if(change_channel_index== 38)
               	{
               		adv_anchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg   //F0h~F3h: NUM_CNT_LO
                                              + 6;    
                	adv_anchor.phaseIn1us.reg = 0;//0x0200;    
              	        irq1isr_open_EXT_advertising_event(pacl,adv_anchor,PDUTYPE_ADV_EXT_IND);                 	           		
               	}
               	else if(change_channel_index== 39)
               	{
               		adv_anchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg   //F0h~F3h: NUM_CNT_LO
                                              + 12;    
                	adv_anchor.phaseIn1us.reg = 0;//0x0200;    
              	        irq1isr_open_EXT_advertising_event(pacl,adv_anchor,PDUTYPE_ADV_EXT_IND);     
                        //pacl->ExtAdvEventAnchor.cntLoIn625us.reg = pacl->ExtAdvEventAnchor.cntLoIn625us.reg+leconfig_ExtAdvParam.primary_adv_interval_min;                                          		
               	}
                hcfsm.state = ST_ext_adv_TX_ADV_EXT_IND_W4_CLOSE_EVENT;
        break;
     
   case ST_ext_adv_TX_ADV_EXT_IND_W4_CLOSE_EVENT: 
     
    	    if( irq1isr_EXT_TX_event_is_closed() )
            {      
            	           	         	      	         	               
                  if( change_channel_index== 39 )
                  {
                  	//hcfsm.state = ST_ext_adv_TX_SETTING_ADV_EXT_IND;   
                       hcfsm.state = ST_ext_adv_TX_SETTING_AUX_ADV_IND;               
            	  }
            	  else
            	  {
                    hcfsm.state = ST_ext_adv_TX_SETTING_ADV_EXT_IND;             	  	
            	  }	
            }
	break;   
   case ST_ext_adv_TX_SETTING_AUX_ADV_IND:     
                change_channel_index = pacl->channelIndex;
                init_0rf_2in1_5602B_set_rfch(RfCenterFreq[change_channel_index]);
                RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[change_channel_index]);		
                pacl->AA=                  calc_syncword_32(0x3488);  
    	        pacl->crcInit[0]=          0x14;                     //CRCInit  (3 octets) initialization value for the CRC calculation
                pacl->crcInit[1]=          0x15;                     //CRCInit  (3 octets) initialization value for the CRC calculation
                pacl->crcInit[2]=          0x16;			  //CRCInit  (3 octets) initialization value for the CRC calculation
                ChannelSelectionAlgorithm2(pacl);
                uart_puts("channelIndex0= ");uart_putu8(pacl->mappedChannelIndex[0]);uart_puts(" ; ");uart_putchar_n('\n');
                uart_puts("channelIndex1= ");uart_putu8(pacl->mappedChannelIndex[1]);uart_puts(" ; ");uart_putchar_n('\n');
                uart_puts("channelIndex2= ");uart_putu8(pacl->mappedChannelIndex[2]);uart_puts(" ; ");uart_putchar_n('\n');
                uart_puts("channelIndex3= ");uart_putu8(pacl->mappedChannelIndex[3]);uart_puts(" ; ");uart_putchar_n('\n');
                
                adv_anchor.cntLoIn625us.reg = pacl->AuxAdvEventAnchor.cntLoIn625us.reg;   //F0h~F3h: NUM_CNT_LO
                                                            	                                                     		          		
              	irq1isr_open_EXT_advertising_event(pacl,adv_anchor,PDUTYPE_AUX_ADV_IND);              	
                hcfsm.state = ST_ext_adv_TX_AUX_ADV_IND_W4_CLOSE_EVENT;
        break;                                 
   case ST_ext_adv_TX_AUX_ADV_IND_W4_CLOSE_EVENT:  
    	    if( irq1isr_EXT_TX_event_is_closed() )
            {                  	           	         	      	         	               
                    //hcfsm.state = ST_ext_adv_TX_SETTING_ADV_EXT_IND;         
                    hcfsm.state = ST_ext_adv_TX_SETTING_AUX_SYNC_IND;       	  		
            }          
   	break;
   case ST_ext_adv_TX_SETTING_AUX_SYNC_IND:     
                change_channel_index = pacl->mappedChannelIndex[0];
                init_0rf_2in1_5602B_set_rfch(RfCenterFreq[change_channel_index]);
                RF_WT08(0x49, PKT2_WHT_EN | WhiteningSeed[change_channel_index]);
                //write_00h_AccessAddress(pacl->AA);	
		//write_50h_CRCInit((uint8_t*)pacl->crcInit+0);	                 
                adv_anchor.cntLoIn625us.reg = pacl->PeriodicAdvEventAnchor.cntLoIn625us.reg;   //F0h~F3h: NUM_CNT_LO
                                                            	                                                     		          		
              	irq1isr_open_EXT_advertising_event(pacl,adv_anchor,PDUTYPE_AUX_SYNC_IND);              	
                hcfsm.state = ST_ext_adv_TX_AUX_SYNC_IND_W4_CLOSE_EVENT;
        break;
   case ST_ext_adv_TX_AUX_SYNC_IND_W4_CLOSE_EVENT:  
    	    if( irq1isr_EXT_TX_event_is_closed() )
            {                  	           	         	      	         	               
                    hcfsm.state = ST_ext_adv_TX_SETTING_ADV_EXT_IND;             	  		
            }          
   	break;             	
    } //end switch
    } while (reloop == 1);
}


////////////////////////////////////////
/**
 *  @ingroup BC3601_Host_FSM1_public_by_Holtek_label
 */
/*
void test_calc_deltaT(void)
{
    uint32_t deltaT;
    uint16_t instant;
    uint16_t ReferenceConnEventCount;
    uint16_t offset0;
    uint16_t connIntervalOLD;
    uint16_t connIntervalNEW;
    int32_t  zzz;

                    instant = 10;
    ReferenceConnEventCount = 12;
                    offset0 = 0x0003;
            connIntervalOLD = 50;
            connIntervalNEW = 52;
    zzz =     (   connIntervalNEW
                - ( instant                           //  instant
                    - ReferenceConnEventCount         //- ReferenceConnEventCount
                  ) * connIntervalOLD                 //x connIntervalOLD            unit 1.25 ms
                    % connIntervalNEW 
                + offset0 
              ) % connIntervalNEW ;
    uart_putu32(zzz);
};
*/

void bc5602B_initial(void)
{	
	int i;
	uint16_t x;
        init_1ad_2in1_5602G2_SAR_ADC_10bits();
	#if RF_SEL == 0
        init_0rf_5602B();
        #elif RF_SEL ==1        
        init_0rf_5602G2TC8_v8(0x01);
        #endif
        SPI_CS(0);//0:RF
        init_2fpga();
	#if HAHA ==0
	 uart_putchar(0xCC);
	#endif
        delay_unit625us(500);

        hcAclDataBuffer_toair_init();
        hcAclDataBuffer_tohost_init();
        llcBuffer_tx2air_init();
        llcBuffer_rx_init();
        initial_advRxQ();
        initial_hc();
        initial_leconfig();
        
      //test_htqueue_api();
      //test_dllist_api();
      //test_calc_deltaT();

    if( _HOST_TESTMODE_ == 1 ) // 0x00:master, 0x01:slave
    {
        ht_memory_copy(leconfig_bdaddr.le_public_AdvA+0, slave_DeviceAddr+0, 6);
              //debug_foo_000s(); //debug
        
     	 debug_foo_000s(); //debug
        /*
        hcicmd_200a_LE_set_adv_enable_TypeDef cmd =
        {
            .adv_enable = 0x01
        };
        hcmd_200a_le_set_adv_enable((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd));
        */
    }
    else
    {
        ht_memory_copy(leconfig_bdaddr.le_public_AdvA+0, master_DeviceAddr+0, 6);
        /*
        hcicmd_200d_LE_create_connection_TypeDef cmd =
        {
            .peer_address[0]   = slave_DeviceAddr[0],
            .peer_address[1]   = slave_DeviceAddr[1],
            .peer_address[2]   = slave_DeviceAddr[2],
            .peer_address[3]   = slave_DeviceAddr[3],
            .peer_address[4]   = slave_DeviceAddr[4],
            .peer_address[5]   = slave_DeviceAddr[5],
        };
        hcmd_200d_le_create_connection((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd));
        */
    }    
    
    #if 0
    {
       hcicmd_2036_LE_set_ext_adv_parameters_TypeDef cmd36 =
       {
          .adv_handle = 0x01,
          .adv_event_properties= 0x40,	                //bit[0] Connectable advertising
							//bit[1] Scannable advertising
							//bit[2] Directed advertising
							//bit[3] High Duty Cycle Directed Connectable advertising ( 3.75 ms Advertising Interval)
							//bit[4] Use legacy advertising PDUs
							//bit[5] Omit advertiser's address from all PDUs ("anonymous advertising")
							//bit[6] Include TxPower in the extended header of at least one advertising PDU 
         .primary_adv_interval_min[0]= 0x40,
         .primary_adv_channel_map = 0x07,	//bit[0] Channel 37 shall be used ;bit[1] Channel 38 shall be used; bit[2] Channel 39 shall be used    
  	 .own_address_type = 0x00,
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
   	 .adv_filter_policy = 0x00,
                                                        //0x00: Process scan and connection requests from all devices (i.e., the White List is not in use) (default)
                                                        //0x01: Process connection requests from all devices and only scan requests from devices that are in the White List
                                                        //0x02: Process scan requests from all devices and only connection requests from devices that are in the White List
                                                        //0x03: Process scan and connection requests only from devices in the White List
         .adv_tx_power = 0x05,		//Range: -127 to +20 Units: dBm
         .primary_adv_phy =0x01,                         //0x01 Primary advertisement PHY is LE 1M 
    							//0x03 Primary advertisement PHY is LE Coded
         .secondary_adv_max_skip =0x00,          	//0x00 AUX_ADV_IND shall be sent prior to the next advertising event
    							//0x01 to 0xFF Maximum advertising events the Controller can skip before sending the AUX_ADV_IND packets on the secondary advertising physical channel 							
         .secondary_adv_max_phy = 0x01,         	//0x01 Secondary advertisement PHY is LE 1M
    							//0x02 Secondary advertisement PHY is LE 2M
	 .adv_sid= 0x01,               			//0x00 to 0x0F Value of the Advertising SID subfield in the ADI field of the PDU
	 .scan_req_notification_enable= 0x00	//0x00 Scan request notifications disabled ; 0x01 enable		    	    		
       };
       
       hcicmd_203E_LE_set_periodic_adv_parameters_TypeDef cmd3e=
       {
  	  .adv_handle = 0x01,
          .periodic_adv_interval_min = 0x00A0,       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)	
          .periodic_adv_interval_max = 0x00B0,       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)
    	  .periodic_adv_properties= 0x0080	//bit[6] Include TxPower in the advertising PDU    	
       }; 
       hcicmd_2051_LE_set_connectionless_cte_transmit_parameters_TypeDef cmd51 =
       {
          .adv_handle = 0x01,  // 
          .cte_length = 0x14,    // 0x02 to 0x14 Constant Tone Extension length in 8 £gs units
          .cte_type=0x00,      // 0x00:AoA , 0x01:AoD 1us , 0x02:AoD 2us
          .cte_count=0x02,	//The number of Constant Tone Extensions to transmit in each periodic advertising interval Range: 0x01 to 0x10
          .length_of_switching_pattern_length = 0x00,
          .antenna_ids=0x00       	  
       };
       hcicmd_2052_LE_set_connectionless_cte_transmit_enable_TypeDef cmd52=
       {
       	  .adv_handle=0x01,   // 
          .cte_enable=0x01   // 
       };
       hcicmd_2040_LE_set_periodic_adv_enable_TypeDef cmd40 =
       {
       	  .enable = 0x01,    
          .adv_handle= 0x01   
       };
       hcicmd_2039_LE_set_ext_adv_enable_TypeDef cmd39=
       {

          .enable = 0x01,   
	  .number_of_sets = 0x00, 
          .adv_handle = 0x01,    
          .duration = 0x00,
          .max_extended_adv_events = 0x00       
       };
              
       //hcmd_2036_le_set_extended_advertising_parameters((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd36));
      // hcmd_203e_le_set_periodic_advertising_parameters((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd3e));
      // hcmd_2051_le_set_connectionless_cte_transmit_parameters((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd51));
      // hcmd_2052_le_set_connectionless_cte_transmit_enable((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd52));
      // hcmd_2040_le_set_periodic_advertising_enable((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd40));
      // hcmd_2039_le_set_extended_advertising_enable((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd39));
           LEACL_TypeDef *pacl;
           pacl = leacl_alloc();
           pacl->role = 0x05;
           pacl->aclConnHandle = 0x01;
           pacl->ext_adv_enable = 0x01;
           debug_hcfsm_state_ST_0(); 
    }
    #endif 
    
        hcfsm.state = ST_0;
        
    ////test
    #if 0
    {	
	unsigned char a = 0xFF;
	signed char b ;
	long int c;
	b = (signed char)a;			
	if(b == -1){uart_puts("yes");}
	uart_putu8(b);
	b = -1;
	uart_putu8(b);
	b = -2;
	uart_putu8(b);		
    	c = b*b;
    	uart_putu16(c);
    	
    	x=0;
    	
    	for(i=0;i<256;i+=4)
    	{	
    		debug_ctepld[i] = x<<6;
    		debug_ctepld[i+1] = (x>>2)&0x3F;
    		debug_ctepld[i+2] = x<<6;
    		debug_ctepld[i+3] = (x>>2)|0xC0;
    		x++; 
    	}
    	/*
    	for(i=256;i<512;i+=4)
    	{
    		debug_ctepld[i] = x<6;
    		debug_ctepld[i+1] = (x>2)&0x3F;
    		debug_ctepld[i+2] = x<6;
    		debug_ctepld[i+3] = x>2|0xE0;
    		x++; 
    	}*/
       format_with_saturation(255,debug_ctepld+0);
       /*
        for(i=0;i<1024;i+=4)
    	{	
    		uart_putu8(debug_ctepld[i]);uart_puts(" ");
    		uart_putu8(debug_ctepld[i+1]);uart_puts(" ");
    		uart_puts(",");
    		uart_putu8(debug_ctepld[i+2]);uart_puts(" ");
    		uart_putu8(debug_ctepld[i+3]);uart_puts(" ");
    		uart_puts(",");
    	}*/
       for(i=0;i<256;i++)       {uart_putu8(debug_ctepld[i]); uart_puts(" ");}
       
    }
    #endif
}

void bc5602B_process(void)
{
    LEACL_TypeDef *pacl;
            switch(schedluezzzz) 
            {
            case 0:
                    schedluezzzz ++;
                        pacl = (LEACL_TypeDef *)dllistPeek(&leACLlist);
                        while (pacl != 0)
                        {
                            bc5602b_hcfsm(pacl);
                            pacl = (LEACL_TypeDef *)(pacl->node.next) ;
                        }
                    break;
            case 1:
                    schedluezzzz ++;
                    process_hc(); //uart rx
                    break;
            case 2:
                    schedluezzzz ++;
                        pacl = (LEACL_TypeDef *)dllistPeek(&leACLlist);
                        while (pacl != 0)
                        {
                            process_llcRxQ(pacl);
                            process_lldataRxQ(pacl);
                            if( pacl->HcAclDataBUFF2air_finished_count[0] != pacl->HcAclDataBUFF2air_finished_count[1] ) {
                                if( send_04_hciEvent_13_number_of_completed_packets(LEACL_conn_handle(pacl), 0x0001)) { //0x0001: num_completed_packets
                                    pacl->HcAclDataBUFF2air_finished_count[1] ++;
                                }
                            }
                            pacl = (LEACL_TypeDef *)(pacl->node.next) ;
                        }
                    break;
            case 3:
                    schedluezzzz ++;
                    process_hcmsgQ();
                    break;
            case 4:
                    schedluezzzz ++;
                    process_advRxQ();
                    break;
            default:
                    schedluezzzz = 0;
                    break;
           }//switch
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
void init_0rf_5602B_rewrite_RXG_to_cover_fpgaWriteRXG0x00(void)
{
    SPI_CS(0);//0:RF
//bank 2
    SPI_BYTE(0x22);
//  SPI_WT(0x20, 0x08);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x21, 0x6E);             //RXG   PGA=011(18dB),BPF=01(18dB),Mixer=1(high) LNA=10(middle)
  //SPI_WT(0x21, 0x8F);             //RXG   PGA=011(18dB),BPF=01(24dB),Mixer=1(high) LNA=11(High)
  //SPI_WT(0x21, 0x34); 
#if AGC_RXG_SEL == 77 
  SPI_WT(0x21, 0x77);             //RXG   PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)  97  17,    77  4F  4E  4D
#elif AGC_RXG_SEL == 57  
   SPI_WT(0x21, 0x57);
#endif   
   // SPI_WT(0x21, 0x36);//36 35 34 for IF 500K    //RXG: PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)  97  17,    77  4F  4E  4D
  //SPI_WT(0x21, 0x4f);             //RXG  PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)
	SPI_BYTE(0x20);
}
void init_0rf_5602B(void)
{
    uint16_t i;
    SPI_CS(0);//0:RF
    
    SPI_BYTE(0x0A);//strobe_0A_deepsleep();
    SPI_BYTE(0x0C);//strobe_0C_lightsleep();
        for(i=0;i<1000;i++) {;}    //delay 57us
    SPI_BYTE(0x08);//strobe_08_software_reset();
    SPI_WT(0x00, 0x10);             //CFG1    Direct mode
                                    //        [1:0]BANK[1:0] control register Bank select, 00:bank0, 01 : bank1, 10 : bank2, 11 : bank3
                                    //        [3:2]-
                                    //        [4]  DIR_EN direct mode enable
                                    //        [5]  PN9_EN
                                    //        [6]  AGC_EN
                                    //        [7]  -
  //SPI_WT(0x07, RFCHANNEL);        //RFCH            2402M:02h,  2426M:1Ah,  2441M:29h,  2480M:50h
    init_0rf_2in1_5602B_set_rfch( RFCHANNEL );
//bank 2
    SPI_BYTE(0x22);
    SPI_WT(0x2D, 0x18);             //VC3   CT_VDMID=3 for calibration initial
    SPI_WT(0x3D, 0x11);             //XO1     [5:0]XO_TRIM=0x__ accroding to board
  //SPI_WT(0x3D, 0x0D);//sean       //XO1     [5:0]XO_TRIM
  #if   _FRONTEND_RF_XCLK_MHZ_ == 16
    SPI_WT(0x3E, 0x08);             //XO2
  #elif _FRONTEND_RF_XCLK_MHZ_ == 32
    SPI_WT(0x3E, 0x00);             //XO2
  #endif
                                    //        [7]  ADCLK_DIV2   0h: ADC input CLK = Fxtal
                                    //                          1h: ADC input CLK = Fxtal/2. Only use in Fxtal=32MHz and ADCLK=16MHz mode
                                    //        [6:5]-
                                    //        [4:3]             [4]XO_SEL  [3]XODIV2
                                    //                XO=32MHz        0       0        XCLK=32MHz
                                    //                XO=32MHz        0       1        XCLK=16MHz
                                    //                XO=16MHz        1       0        XCLK=16MHz
                                    //        [2:0]-
//bank 1
    SPI_BYTE(0x21);
    //For 5602B_TC1(Delta-Sigma)
  //SPI_WT(0x28, 0x07);             //DLY1: CLA_DIG[2:0], CMA_DIG[2:0]
  //SPI_WT(0x29, 0x00);             //DLY2: LB_Offset_SEL[2:0], CPA_DIG[2:0]
    
    //For 5602B_TC2(ASAR)
    SPI_WT(0x28, 0x00);             //DLY1: CLA_DIG[2:0], CMA_DIG[2:0]
    SPI_WT(0x29, 0x50);             //DLY2: I_PGA_DIG[2:0], CPA_DIG[2:0]
    
//bank 2
    SPI_BYTE(0x22);
  //SPI_WT(0x37, 0x91);             //VCO Calibration enable
  //SPI_WT(0x38, 0x01);             //RC Time Calibration enable
//bank 2
    SPI_BYTE(0x22);
    SPI_WT(0x20, 0x08);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x21, 0x6E);             //RXG   PGA=011(18dB),BPF=01(18dB),Mixer=1(high) LNA=10(middle)
  //SPI_WT(0x21, 0x8F);             //RXG   PGA=011(18dB),BPF=01(24dB),Mixer=1(high) LNA=11(High)
//  SPI_WT(0x21, 0x77);             //RXG   PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)  97  17,    77  4F  4E  4D
    SPI_WT(0x21, 0x36);//36 35 34 for IF 500K    //RXG: PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)  97  17,    77  4F  4E  4D
  //SPI_WT(0x21, 0x4f);             //RXG  PGA=100(24dB),BPF=10(24dB),Mixer=1(high) LNA=11(High)
    SPI_WT(0x26, 0x01);             //CP1  CP_EN=1
    SPI_WT(0x27, 0x66);             //CP2
    SPI_WT(0x28, 0x22);             //CP3  P3RD[1:0]=0,PZ[1:0]=1,ALPF_EN=1   #2A     22
  //SPI_WT(0x29, 0x83);             //OD1  RT_ENB=0, [4]TR_MODE=0:RX, D2 Enable=1,OD_EN=1
    SPI_WT(0x29, 0x93);             //OD1  RT_ENB=1, [4]TR_MODE=1:TX, D2 Enable=1,OD_EN=1
    SPI_WT(0x2A, 0x7D);             //OD2     [5]ADCLK_SELM=1(ADC sampling rate 32MHz) (only for TC1, TC2 no use)
    SPI_WT(0x2B, 0x13);             //VC1  IB_VCOBUF=0, VCO EN=1,AMP_CLT=0    ???
  //SPI_WT(0x2C, 0x0D);             //VC2  Force VCO_DFC[3:0]= 4'b0110(line6)
    SPI_WT(0x2D, 0x18);//sean       //VC3
    SPI_WT(0x2E, 0xEF);             //RX1  MIX_EN=1, LNA_EN=1, IB_LNA[2:0]=3'b111     ?????   EF
  //SPI_WT(0x2E, 0x73);             //RX1  MIX_EN=1, LNA_EN=1, IB_LNA[2:0]=3'b100
    SPI_WT(0x2F, 0x5B);             //RX2  AMCTRL=1   LNAFC[1:0]=11     5602B_TC1&TC2: 5B     ##############
  //SPI_WT(0x2F, 0x58);             //RX2  AMCTRL=1 5602T3 5C
    SPI_WT(0x30, 0x09);             //RX3  IF_EN=1, CF=500kHZ , IB_IF<2:0>=3'b101     0B   09
    SPI_WT(0x31, 0x01);             //RX4     [0]ADC_EN=1  For SAR
  //SPI_WT(0x31, 0x07);             //RX4     [0]ADC_EN=0  For DS
    SPI_WT(0x32, 0x08);             //RX5  RCB[2:0]=3'b100
  //SPI_WT(0x33, 0x54);             //TX1     [0]  PAD_EN: TX PA driver enable
  //SPI_WT(0x33, 0x42);//debug      //TX1     [0]  PAD_EN: TX PA driver enable
  //SPI_WT(0x33, 0x24);//debug      //TX1     [0]  PAD_EN: TX PA driver enable
    SPI_WT(0x33, 0x25);//sean       //TX1    1[0]  PAD_EN: TX PA driver enable
  //SPI_WT(0x33, 0x36);//debug      //TX1     [0]  PAD_EN: TX PA driver enable
                                    //        [3:1]RAMP_DN   000:x  001:30u  010:40u  011:50u  100:65u  101:80u  110:100u  111:120u
                                    //        [6:4]RAMP_UP   000:x  001:25u  010:35u  011:45u  100:60u  101:75u  110: 95u  111:115u
                                    //        [7]-
    SPI_WT(0x34, 0xC6);// 7 dbm     //TX2
  //SPI_WT(0x34, 0xAF);// sean      //TX2
                                    //        [7:5]CT_VDD_PAD
                                    //        [4:3]CT_PRE_PAD
                                    //        [2:0]CT_DRV_PAD
    SPI_WT(0x35, 0x0F);// 7 dbm     //TX3     
  //SPI_WT(0x35, 0x21);// 0 dbm     //TX3
                                    //        [7]-
                                    //        [6:4]IBP_PAD[2:0]
                                    //        [3:0]IBN_PAD[3:0]
 // SPI_WT(0x36, 0x02);             //TX4     [7:4]-
 // SPI_WT(0x36, 0x03);//011:40us   //TX4     [7:4]-
 // SPI_WT(0x36, 0x04);//100:55us   //TX4     [7:4]-
    SPI_WT(0x36, 0x0A);//sean       //TX4     [7:4]-
                                    //        [3]  PAD_OW
                                    //        [2:0]DLY_PAD, PAD delay. 000:6us  001:20us  010:30us  011:40us  100:55us  101:75us  110:100us  111:128us
    SPI_WT(0x39, 0x12);             //CA3: DCOCTB_EN[1]=1,DCOC_ENB[0]=1        /////// 12
  //SPI_WT(0x37, 0x93);             //CA1
  //SPI_WT(0x39, 0x13);             // 
    SPI_WT(0x3A, 0x57);             //LD1: SXLDO_EN[7:6]=01(1.5V),VCOLDO_EN[5:4]=01(1.5V),XOLDO[3:2]=01(1.5V)
    SPI_WT(0x3B, 0x57);             //LD2: DIG_LDO[7:6]=01(1.2V), IFLDO_EN[5:4]=01(1.5V),ADCLDO_EN[3:2]=01(1.5V)      57
    SPI_WT(0x3C, 0x43);             //LD3: IB_BG[1:0]=2 LNALDO_EN[2:1]=01(1.5V) with Reg34=6F----------------------------------------------
    SPI_WT(0x3F, 0x01);             //RTM1
    //spi_rd64 0xEC 0X1;    // Read DFC line
//################################## IF = 1 MHz
  //SPI_WT(0x38, 0x22);             //RC calibration manual setting   16 22
    SPI_WT(0x38, 0x20);             //RC calibration auto
//bank 1
    SPI_BYTE(0x21);
    SPI_WT(0x25, 0x00);             //FCF2  FSCALE[7:0]
  #if   D_RATE == 0 // 250 kbps
    SPI_WT(0x26, 0x20);             //FCF3  FSCALE[15:8]  DR 250K(20h) 1M(40h) 2M(80h) 0x2000= 8192
  #elif D_RATE == 1 // 1 Mbps
    SPI_WT(0x26, 0x40);             //FCF3  FSCALE[15:8]  DR 250K(20h) 1M(40h) 2M(80h) 0x4000=16384 Fscale = (2^18 * Fd / Fxtal)-1 = (262144*100000/32000000)= 819.2  ???
  #elif D_RATE == 2 // 2 Mbps
    SPI_WT(0x26, 0x80);             //FCF3  FSCALE[15:8]  DR 250K(20h) 1M(40h) 2M(80h) 0x8000=32768
  #else
    ...
  #endif
  //SPI_WT(0x27, 0x80);             //FCF4  For DS          RXFD_EN    
    SPI_WT(0x27, 0x80);             //FCF4  For ASAR     RXFD_EN,       80
//bank 0
    SPI_BYTE(0x20);
    SPI_WT(0x21, 0x3A);             //DM0/CFO1  ACFO DISABLE   
                                    //DM0/CFO1[7]  ACFO_EN    Auto CFO Calculations enable
                                    //        [6]  AMBLE2     0:preamble 1 byte, 1:preamble 2 bytes
                                    //        [5]  SKIP_PRE   Skip preamble detect in BLE mode
                                    //        [4]  -
                                    //        [3:0]RSSI_CTHD  RSSI threshold for carrier detection
    SPI_WT(0x22, AW | D_RATE);      //DM1  32:2M   31:1M   30:250K
                                    //DM1     [7:6]-
                                    //        [5:4]AW   11:5 bytes, 10:4 bytes, 01:3 bytes, 00:-
                                    //        [3]  -
                                    //        [2:0]D_RATE 0h:250Kbps, 1h:1Mbps, 2h:2Mbps
    SPI_WT(0x23, 0x08);             //DM2     [7:0]FD_HOLD
  #if   D_RATE == 0 // 250 kbps
    SPI_WT(0x24, 0x40);             //DM3     [7:0]PHASE_DSTEP       DR 250K(40)    1M(80)    2M(80)     default(80)
  #elif D_RATE == 1 // 1 Mbps
    SPI_WT(0x24, 0x80);             //DM3     [7:0]PHASE_DSTEP       DR 250K(40)    1M(80)    2M(80)     default(80)
  #elif D_RATE == 2 // 2 Mbps
    SPI_WT(0x24, 0x80);             //DM3     [7:0]PHASE_DSTEP       DR 250K(40)    1M(80)    2M(80)     default(80)
  #else
    ...
  #endif
//bank 3
    SPI_BYTE(0x23);
    SPI_WT(0x20, 0x40);             //IO1     [5:3]GIO2S[2:0]=0h:-     , [2:0]GIO1S[2:0]=1h:MISO   , [7:6]PADDS PAD drive strength:0:0.5mA,1:1mA,2:5mA,3:10mA
  //SPI_WT(0x20, 0x80);             //IO1     [5:3]GIO2S[2:0]=0h:-     , [2:0]GIO1S[2:0]=1h:MISO   , [7:6]PADDS PAD drive strength:0:0.5mA,1:1mA,2:5mA,3:10mA

    SPI_WT(0x26, 0x80);             //TEST4
                                    //        [3:0]GIOTMS[3:0]=0h
                                    //        [5:4]GIOTMR[1:0]
                                    //        [6]-
                                    //        [7]GIOTME:GIO test mode functions enable
    SPI_WT(0x2D, 0x00);             //SAD1    [1:0]CPF_M IF filter mode  00(250K&500K),  01(1M),  10&11(2M)
  #if   D_RATE == 0 // 250 kbps
    SPI_WT(0x2D, 0x00);             //SAD1    [1:0]CPF_M IF filter mode  00(250K&500K),  01(1M),  10&11(2M)
  #elif D_RATE == 1 // 1 Mbps
    SPI_WT(0x2D, 0x01);             //SAD1    [1:0]CPF_M IF filter mode  00(250K&500K),  01(1M),  10&11(2M)
  #elif D_RATE == 2 // 2 Mbps
    SPI_WT(0x2D, 0x02);             //SAD1    [1:0]CPF_M IF filter mode  00(250K&500K),  01(1M),  10&11(2M)
  #else
    ...
  #endif

    SPI_WT(0x07, RFCHANNEL);        //RFCH            2402M:02h,  2426M:1Ah,  2441M:29h,  2480M:50h
//bank 2
    SPI_BYTE(0x22);
  //SPI_WT(0x37, 0x11);             //CA1      [7]RTXAC_EN, [4:2]DLY_VCOCAL, [1]CALBUF_EN,   [0]VCOCAL_EN=1 to cali DFC
    SPI_WT(0x20, 0x08);             //OM  
  delay_zzzz();
//bank 1
    SPI_BYTE(0x21);
  #if   D_RATE == 0 // 250 kbps
    SPI_WT(0x31, 0x0C);//DAC_I_OW=1 //DA8   [7]-, [6:4]DAC_R_OS, [3]TPCDNK_BP=1 if need ch hopping, [2]DAC_I_OW, [1]-, [0]DAC_EN
    SPI_WT(0x32, 0x00);             //DA9   [7:0]DAC_I[7:0]
    SPI_WT(0x33, 0x02);             //DA10  [1:0]DAC_I[9:8]    [7:2]-
    //
    SPI_WT(0x2A, 0x30);//yongzong   //DA1   [7]TPCALF(read only), [6]TPCAC_EN,  [5:4]TPCAL_TC, [3:1]DLY_TPCAL,  [0]TPCAL_EN=0 disable Two Point calibration
    SPI_WT(0x30, 0x00);//yongzong   //TXFD  [7]DACFD_EN, [6]MMDFD_EN, [5]-, [4]DLY_BPS_EN=1=by pass delay,  [3]DAC_DLY_EN(SEL),  [2:0]DAC_DLY,  18(default)=No DAC delay,  08=MMD delay max.
  #else
    SPI_WT(0x31, 0x08);//DAC_I_OW=0 //DA8   [7]-, [6:4]DAC_R_OS, [3]TPCDNK_BP=1 if need ch hopping, [2]DAC_I_OW, [1]-, [0]DAC_EN
    //
  //SPI_WT(0x3E, 0x1D);             //DA21, [6]DAC_R_CHCEN,  [5:0]DAC_R_CHCS
    SPI_WT(0x3E, 0x5D);//DGL        //DA21, [6]DAC_R_CHCEN,  [5:0]DAC_R_CHCS
  //SPI_WT(0x3D, 0x57);             //DA20, TPC_DONE, TPCAL4M, TPCAL_MDLY<1:0>, TPCAL_TCM<1:0>, CTMS<1:0>;
    SPI_WT(0x3D, 0x5F);//DGL        //DA20, TPC_DONE, TPCAL4M, TPCAL_MDLY<1:0>, TPCAL_TCM<1:0>, CTMS<1:0>;
    SPI_WT(0x2A, 0x35);             //DA1   [7]TPCALF(read only), [6]TPCAC_EN,  [5:4]TPCAL_TC, [3:1]DLY_TPCAL,  [0]TPCAL_EN
  delay_zzzz();
    SPI_WT(0x30, 0x08);//yongzong   //TXFD  [7]DACFD_EN, [6]MMDFD_EN, [5]-, [4]DLY_BPS_EN=1=by pass delay,  [3]DAC_DLY_EN(SEL),  [2:0]DAC_DLY,  18(default)=No DAC delay,  08=MMD delay max.
  //SPI_WT(0x30, 0x0F);//sean       //TXFD  [7]DACFD_EN, [6]MMDFD_EN, [5]-, [4]DLY_BPS_EN=1=by pass delay,  [3]DAC_DLY_EN(SEL),  [2:0]DAC_DLY,  18(default)=No DAC delay,  08=MMD delay max.
  #endif

 //######################################  AGC
  //SPI_WT(0x02, 0x01);             //RC2     ADCLK_SEL[0]=0:select 16M ; ADCLK_SEL[0]=1:select 32M
//bank 2
    SPI_BYTE(0x22);
  //SPI_WT(0x20, 0x01);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x20, 0x05);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x20, 0x03);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x20, 0x07);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x20, 0x08);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
  //SPI_WT(0x20, 0x0a);             //OM      [7]PWR_SOFT, ACAL_EN,RTX_EN,RTX_SEL,SX_EN
}


void init_0rf_5602G2TC8_v8(uint8_t phy)
{

    uint16_t i;
    SPI_CS(0);//0:RF
    
    SPI_BYTE(0x0A);//strobe_0A_deepsleep();
    SPI_BYTE(0x0C);//strobe_0C_lightsleep();
        for(i=0;i<1000;i++) {;}    //delay 57us
    SPI_BYTE(0x08);//strobe_08_software_reset();
    SPI_WT(0x00, 0x10);             //CFG1    Direct mode
                                    //        [1:0]BANK[1:0] control register Bank select, 00:bank0, 01 : bank1, 10 : bank2, 11 : bank3
                                    //        [3:2]-
                                    //        [4]  DIR_EN direct mode enable
                                    //        [5]  PN9_EN
                                    //        [6]  AGC_EN
                                    //        [7]  -
  //SPI_WT(0x07, RFCHANNEL);        //RFCH            2402M:02h,  2426M:1Ah,  2441M:29h,  2480M:50h
    init_0rf_2in1_5602B_set_rfch( RFCHANNEL );

//bank 2
    SPI_BYTE(0x22);
    SPI_WT(0x2D, 0x18);              //VC3: CT_VDMID=3 for calibration initial
    SPI_WT(0x3D, 0x18);              //XO1: XSHIFT,  XO_IL,  XO_TRIM[4:0]   
	  SPI_WT(0x3E, 0x00);              //XO2: XODIV2=0, XO_SEL=32MHz

//common bank
	  SPI_WT(0x10, 0x53);              //RFCH: 
//uart_puts("_0rf RFCH ="); uart_putu8(spi_r(0x10));  uart_putchar_n('\n');
	//bank 3
    SPI_BYTE(0x23);
    SPI_WT(0x2D, 0x00);              //RFT1: CPA[2:0], CMA[2:0]
    SPI_WT(0x2E, 0x50);  		//RFT2: IB_PGA=101, CLA[2:0] 
   // SPI_WT(0x2E, 0x40);              //RFT2: IB_PGA=100, CLA[2:0] 
    SPI_WT(0x2C, 0x01);              //LD4: BG_FT[2:0]	

//bank 2
    SPI_BYTE(0x22);	
    SPI_WT(0x20, 0x08);              //Auto calibration(VCO calibration enable), RX Mode
		
        for(i=0;i<10000;i++) {;}    //delay 57us
//bank 2
    SPI_BYTE(0x22);		
 
if( phy == 0x01)
{	
   	SPI_BYTE(0x22);	
	SPI_WT(0x21, 0x57);              //RXG: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_BYTE(0x23);
	SPI_WT(0x20, 0x2E);              //RXG2: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x21, 0x2D);              //RXG3: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x22, 0x2C);	         //RXG4: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
		
}
else if (phy == 0x02)
{
	SPI_BYTE(0x22);	
	SPI_WT(0x21, 0x77);
	SPI_BYTE(0x23);
	SPI_WT(0x20, 0x36);              //RXG2: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x21, 0x35);              //RXG3: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x22, 0x34);              //RXG4: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
}
else
{
 	SPI_BYTE(0x22);		
	SPI_WT(0x21, 0x57);
	SPI_BYTE(0x23);
	SPI_WT(0x20, 0x2E);              //RXG2: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x21, 0x2D);              //RXG3: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
	SPI_WT(0x22, 0x2C);	         //RXG4: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
}
 	SPI_BYTE(0x22);	  	

		//SPI_WT(0x21, 0x34);              //RXG: PGA=011, BPF=10, Mixer=1(high), LNA=11(High)
    SPI_WT(0x26, 0x01);              //CP1: CP_EN=1 
    SPI_WT(0x27, 0x66);              //CP2: IB_CP[3:0], CX[3:0]
		SPI_WT(0x28, 0x21);              //CP3: DLY_SYN[2:0], P3RD[1:0]=0, PZ[1:0]=1, ALPF_EN=1      2021.11.03 modified
		SPI_WT(0x29, 0x83);              //OD1: RT_ENB=1, TR_MODE=RX, D2 Enable=1,OD_EN=1
		SPI_WT(0x2A, 0x7D);              //OD2: ADCLK_SELM=1  32MHz  ??
		SPI_WT(0x2B, 0x13);              //VC1: IB_VCOBUF=0, VCO EN=1, AMP_CTL=01   
		SPI_WT(0x2E, 0xEF);              //RX1: MIX_EN=1, LNA_EN=1, IB_LNA[2:0]=3'b011, IB_MIX[2:0]=3'b111   
		SPI_WT(0x2F, 0x56);              //RX2: AMCTRL=1 	CT_MIXVB[1:0]=01 , LNAFC[1:0]=11  modified at 2020.12.1
		
//////////modify to default IB_IF[2:0]=100## 2021.01.08
		SPI_WT(0x30, 0x0B);              //RX3: PGA_PM[1:0], IB_CPF=101, IF_EN=1     09
		//SPI_WT(0x30, 0x09);              //RX3: PGA_PM[1:0], IB_CPF=100, IF_EN=1     09
		SPI_WT(0x31, 0x21);              //RX4: ADC_EN=1,     BC[1:0]=00(8bit)             
		SPI_WT(0x32, 0x08);              //RX5: RCB[2:0]=3'b100   
		//SPI_WT(0x39, 0x90);              //CA3: DCOCTB_EN[1]=1, DCOC_ENB[0]=0:RX DCOC Enable	   
		SPI_WT(0x39, 0x84);
		
		SPI_WT(0x3A, 0x57);              //LD1: CT_SXLDO=01(1.5V), CT_VCOLDO_EN=01(1.5V), CT_XOLDO[3:2]=01(1.5V)
		SPI_WT(0x3B, 0x57);              //LD2: CT_DIGLDO=01(1.2V), IFLDO_EN[5:4]=01(1.5V), ADCLDO_EN[3:2]=01(1.5V)	  57  
		SPI_WT(0x3C, 0x43);              //LD3: IB_BG[1:0]=2 LNALDO_EN[2:1]=01(1.5V) 
		SPI_WT(0x3F, 0x01);              //RTM1  
////////////IF = 1 MHz		
		SPI_WT(0x38, 0x20);              //RC calibration manual setting  22 16
//bank 1
    SPI_BYTE(0x21);
		SPI_WT(0x25, 0x00);              //FCF2 , FSCALE[7:0]
	if(   phy == 0x02)
	{
		SPI_WT(0x26, 0x80);              //FCF3 , FSCALE[15:8]    DR 250K(20)    1M(40)     2M(80) ?IF Location           
	}
	else if (phy == 0x01)
	{
 		SPI_WT(0x26, 0x40);              //FCF3 , FSCALE[15:8]    DR 250K(20)    1M(40)     2M(80) ?IF Location           
	}
	else if( phy == 0x00)
	{
 		SPI_WT(0x26, 0x20);              //FCF3 , FSCALE[15:8]    DR 250K(20)    1M(40)     2M(80) ?IF Location           
	}

		SPI_WT(0x27, 0x80);              //FCF4  For ASAR     RXFD_EN,       80 		

//bank 0
     SPI_BYTE(0x20);
 	if(   phy == 0x02)
	{
		SPI_WT(0x3C, 0x02);              //CPF_M[1:0]=00(CF_500K&BW_350K),  01(CF_1M&BW_0.7M),  10&11(CF_2M&BW_1.4M)  ?IF Filter           
	}
	else if (phy == 0x01)
	{
 		SPI_WT(0x3C, 0x01);              //CPF_M[1:0]=00(CF_500K&BW_350K),  01(CF_1M&BW_0.7M),  10&11(CF_2M&BW_1.4M)  ?IF Filter           
	}
	else if( phy == 0x00)
	{
 		SPI_WT(0x3C, 0x00);              //CPF_M[1:0]=00(CF_500K&BW_350K),  01(CF_1M&BW_0.7M),  10&11(CF_2M&BW_1.4M)  ?IF Filter          
	}    	

//bank 3
    SPI_BYTE(0x23);	
     	if(   phy == 0x02)
	{
		SPI_WT(0x39, 0x80);              //DM3: PHASE_DSTEP    #For 16Ms/s ADC : 250Kbps(if500k=20, if1M=40)   1Mbps(40)   2Mbps(80)   ?Shift to DC Location           
	}
	else if (phy == 0x01)
	{
 		SPI_WT(0x39, 0x40);              //DM3: PHASE_DSTEP    #For 16Ms/s ADC : 250Kbps(if500k=20, if1M=40)   1Mbps(40)   2Mbps(80)   ?Shift to DC Location   
	}
	else if( phy == 0x00)
	{
 		SPI_WT(0x39, 0x20);              //DM3: PHASE_DSTEP    #For 16Ms/s ADC : 250Kbps(if500k=20, if1M=40)   1Mbps(40)   2Mbps(80)   ?Shift to DC Location
	}
		
	
//		SPI_WT(0x3A, 0x01);              //DM4: STEP_EN
///////////////AGC		
//bank 1
    SPI_BYTE(0x21);
		SPI_WT(0x20, 0x04);              //AGC1 , YAVF_FSEL,  OFFSET_SEL,  AGC_CMP_THD
		SPI_WT(0x21, 0x13);              //AGC2 ,  OFFSET[4:0],   IF_DETOK_THD[2:0] 	           
		SPI_WT(0x23, 0x20);              //AGC7 ,  GAIN_STB[7:0]    13, 20=5.5us	
//bank 0
     SPI_BYTE(0x20);
		SPI_WT(0x3D, 0x04);              //AGC5 , MPT_0dB_EN[3] , LB_OFFSET_SEL[2:0]
//bank 3
 //   SPI_BYTE(0x23);	           
	//	SPI_WT(0x3B, 0x8B);              //AGC5 , GAIN_MSWT, ABORT_TIME[4:0]	
///////////For ASAR ADC   //For G2_V6 Version		
//bank 1
    SPI_BYTE(0x21);
		SPI_WT(0x28, 0x42);              //SAD2 , AD_ENCL[6] , AD_CLOS[1:0]=10
		//SPI_WT(0x29, 0x05);              //SAD3 , AD_CR[2:0](R)=000 , AD_ROF[4](R)=0 , AD_CLOK[3](R)=0, AD_CRW[2:0]=100    modified 2022.4.14  
//common bank		
		SPI_WT(0x06, 0x80);              //IO1
		SPI_WT(0x00, 0x10);              //DIRECT MODE ENABLE[4]--> Enable direct mode
		SPI_WT(0x11, 0x51);              //DM1: AW[1:0] , D_RATE[1:0] , 02:2M   51:1M   A0:250K
//bank 0
//     SPI_BYTE(0x20);
//		SPI_WT(0x21, 0x00);              //CFO1 , AMBLE2
//		SPI_WT(0x20, 0x01);              //PWR_SOFT, BAND_SEL[1:0],XO_MODE,ACAL_EN,RTX_EN,RTX_SEL,SX_EN
//		SPI_WT(0x20, 0x05);              //PWR_SOFT, BAND_SEL[1:0],XO_MODE,ACAL_EN,RTX_EN,RTX_SEL,SX_EN
//bank 3
    SPI_BYTE(0x23);

		SPI_WT(0x3B, 0x00);              //ABORT	

		//SPI_WT(0x33, 0x80|0x08);              //TS MODE GIO2,GIO1	INPUT	
		
		SPI_WT(0x33, 0x96);              //TS MODE GIO2,GIO1	INPUT	
//bank 0
     SPI_BYTE(0x20);
		SPI_WT(0x21, 0x00);
		SPI_WT(0x20, 0x01);
		SPI_WT(0x20, 0x05);
		
//bank 2
    SPI_BYTE(0x22);//FPGA maybe send strobe to write OM, RXG
    SPI_BYTE(0x23);

}

void init_1ad_2in1_5602G2_SAR_ADC_10bits(void)
{
    uint16_t i;
    SPI_CS(1);//1:AD
    
  //SPI_BYTE(0x0A);//strobe_0A_deepsleep();
    SPI_BYTE(0x0C);//strobe_0C_lightsleep();
        for(i=0;i<1000;i++) {;}    //delay 57us
    SPI_BYTE(0x08);//strobe_08_software_reset();

    SPI_WT(0x06, 0x80);             // IO1      [7:6]PADDS PAD drive strength, 0:0.5mA, 1:1mA, 2:5mA, 3:10mA            adjust for fine-tune(2019.09.24)
                                    //          [5:3]GIO2S=0
                                    //          [2:0]GIO1S=0
//bank 2
    SPI_BYTE(0x22);
    SPI_WT(0x20, 0x80);//yongzong   //OM   [7]PWR_SOFT, [6:4]-, [3]ACAL_EN, [2]RTX_EN, [1]RTX_SEL, [0]SX_EN
    SPI_WT(0x3A, 0x57);             //LD1  SXLDO_EN =1(1.5V),VCOLDO_EN=1(1.5V)
  //SPI_WT(0x3B, 0x54);             //LD2  DIG_LDO=1.5V IFLDO_EN =1(1.5V),ADCLDO_EN=1(1.5V) adjust for fine-tune(2020.03.11)
    SPI_WT(0x3B, 0x5F);//yongzong   //LD2  for ADC TK4 testkey 
  //########################
    SPI_WT(0x3C, 0x43);             //LD3  LNALDO_EN[2:1]=01(1.5V) 
    SPI_WT(0x28, 0x13);             //CP3  P3RD[1:0]=2'b11,PZ[1:0]=1,ALPF_EN=1 
    SPI_WT(0x2A, 0x61);             //OD2   [5]ADCLK_SELM=1(ADC sampling rate 32MHz)
  //SPI_WT(0x2A, 0x41);             //OD2   [5]ADCLK_SELM=0(ADC sampling rate 16MHz)
  
  //SPI_WT(0x31, 0x00);             //RX4
    SPI_WT(0x31, 0x01);             //RX4   10bit ,  adjust for fine-tune(2020.03.11)
  //SPI_WT(0x31, 0x39);             //RX4    8bit ,  adjust for fine-tune(2020.03.11)
    SPI_WT(0x32, 0x08);             //RX5   ADC_EXTH[7]=1 extrernal 80M input
    
    SPI_WT(0x3D, 0x10);             //XO1    xotrim for CL=16pF for testkey TK3=07 , TK4=0F 
    SPI_WT(0x3E, 0x00);             //XO2
                                    //        [7]  ADCLK_DIV2   0h: ADC input CLK = Fxtal
                                    //                          1h: ADC input CLK = Fxtal/2. Only use in Fxtal=32MHz and ADCLK=16MHz mode
                                    //        [6:5]-
                                    //        [4:3]             [4]XO_SEL  [3]XODIV2
                                    //                XO=32MHz        0       0        XCLK=32MHz
                                    //                XO=32MHz        0       1        XCLK=16MHz
                                    //                XO=16MHz        1       0        XCLK=16MHz
                                    //        [2:0]-
    SPI_WT(0x3F, 0x00);             //RTM1   RFTM[3:0]=0 , ADC single end test mode
//bank 3
    SPI_BYTE(0x23);
    SPI_WT(0x33, 0x94);             //TEST4 [4:0]GIOTMS=14h, SAR ADC Test mode2    GIO[10:0] = {EOCL (out), SAR_OL [7:0] (out), SAR_NCL [1:0] (out)}
//bank 1
    SPI_BYTE(0x21);
  //###Auto Cal######
/*
    SPI_WT(0x28, 0x42);//yongzong   //SAD2  [6]AD_ENCL=1, [1:0]AD_CHG=10, Enable calibration
  //SPI_WT(0x28, 0x22);//old        //SAD2  [6]AD_ENCL=1, [1:0]AD_CHG=10, Enable calibration
  //###over write cal###
  //SPI_WT(0x28, 0x02);             //SAD2  [6]AD_ENCL=1, [1:0]AD_CHG=10, Enable calibration
  //SPI_WT(0x29, 0x04);             //SAD3  AD_CR[2:0]=000 , AD_ROF[4]=0 , AD_CLOK[3]=0, AD_CRW[2:0]=100  modified 2021.5.14
  //SPI_WT(0x29, 0x01);             //SAD3
*/
    SPI_WT(0x28, 0x00);//yongzong   //SAD2  Disable calibration
    SPI_WT(0x29, 0x04);//yongzong   //SAD3  
//
  //#############
//bank 2
    SPI_BYTE(0x22);
  //SPI_WT(0x31, 0x00);
  //#after 50
  //SPI_WT(0x31, 0x01);
    SPI_WT(0x00, 0x10);             //CFG1  [4]DIR_EN=1  DIRECT MODE ENABLE
                                    //        [1:0]BANK[1:0] control register Bank select, 00:bank0, 01 : bank1, 10 : bank2, 11 : bank3
                                    //        [3:2]-
                                    //        [4]  DIR_EN direct mode enable
                                    //        [5]  PN9_EN
                                    //        [6]  AGC_EN
                                    //        [7]  -
  //SPI_WT(0x20, 0x01);             //OM    [7]PWR_SOFT, [6:4]-, [3]ACAL_EN, [2]RTX_EN, [1]RTX_SEL, [0]SX_EN
  //SPI_WT(0x20, 0x05);             //OM    [7]PWR_SOFT, [6:4]-, [3]ACAL_EN, [2]RTX_EN, [1]RTX_SEL, [0]SX_EN
    SPI_WT(0x20, 0x85);             //OM    [7]PWR_SOFT, [6:4]-, [3]ACAL_EN, [2]RTX_EN, [1]RTX_SEL, [0]SX_EN
//bank 3
    SPI_BYTE(0x23); // bank 3 addr 20h not used, when FPGA send write OM 0x23 0x27 0x22, 0x21 0x25 0x22
        for(i=0;i<1000;i++) {;}    //delay 57us
//back to default 0:RF
    SPI_CS(0);//0:RF
}



