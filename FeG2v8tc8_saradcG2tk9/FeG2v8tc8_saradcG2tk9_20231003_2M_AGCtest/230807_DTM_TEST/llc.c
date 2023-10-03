/*********************************************************************************************************//**
 * @file    llc.c
 * @version $Rev:: 100          $
 * @date    $Date:: 2022-02-22 #$
 * @brief   
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
 * <h2><center>Copyright (C) 2022 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>                           //printf
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcmp, memcpy

#include "bc5602b_host.h"                    //#define  __MALLOC_METHOD__ ,      ht_read_byte
#include "pdma.h"                            //pdma_ram_bleRx_read_payload()
#include "usart.h"                           //uart_puts()
#include "hc.h"                              //send_04_hciEvent_3e_04_LE_read_remote_feature_complete()
#include "hcmsg.h"
#include "aes128.h"                          //aes128_encrypt___lsbyte_to_msbyte  #if AESCCM_LL_SIGNALING == 1
#include "leconfig.h"
#include "llc.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"

bool debug_send_m2s_1(LEACL_TypeDef *); //debug
static inline void debug_RFLA_pulse_to_trigger_LA(void)
{
    RF_WT08(0x100, 0x1);
}

/* Private function prototypes -----------------------------------------------------------------------------*/
bool RxLLcPDU_00_connection_update_ind(uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_01_channel_map_ind      (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_02_terminate_ind        (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_03_enc_req              (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_04_enc_rsp              (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_05_start_enc_req        (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_06_start_enc_rsp        (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_07_unknown_rsp          (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_08_feature_req          (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_09_feature_rsp          (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0a_pause_enc_req        (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0b_pause_enc_rsp        (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0c_version_ind          (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0d_reject_ind           (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0e_slave_feature_req    (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_0f_connection_param_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_10_connection_param_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_11_reject_ext_ind       (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_12_ping_req             (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_13_ping_rsp             (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_14_length_req           (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
bool RxLLcPDU_15_length_rsp           (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);
/*
const uint8_t LLCPDU_SIZEOF_PAYLOAD[0x1F] = {
    sizeof(llcpdu_00_connection_update_ind_TypeDef), //0x00 LL_00_CONNECTION_UPDATE_IND
    sizeof(llcpdu_01_channel_map_ind_TypeDef),       //0x01 LL_01_CHANNEL_MAP_IND
    sizeof(llcpdu_02_terminate_ind_TypeDef),         //0x02 LL_02_TERMINATE_IND
    sizeof(llcpdu_03_enc_req_TypeDef),               //0x03 LL_03_ENC_REQ
    sizeof(llcpdu_04_enc_rsp_TypeDef),               //0x04 LL_04_ENC_RSP
    sizeof(llcpdu_05_start_enc_req_TypeDef),         //0x05 LL_05_START_ENC_REQ
    sizeof(llcpdu_06_start_enc_rsp_TypeDef),         //0x06 LL_06_START_ENC_RSP
    sizeof(llcpdu_07_unknown_rsp_TypeDef),           //0x07 LL_07_UNKNOWN_RSP
    sizeof(llcpdu_08_feature_req_TypeDef),           //0x08 LL_08_FEATURE_REQ
    sizeof(llcpdu_09_feature_rsp_TypeDef),           //0x09 LL_09_FEATURE_RSP
    sizeof(llcpdu_0a_pause_enc_req_TypeDef),         //0x0A LL_0A_PAUSE_ENC_REQ
    sizeof(llcpdu_0b_pause_enc_rsp_TypeDef),         //0x0B LL_0B_PAUSE_ENC_RSP
    sizeof(llcpdu_0c_version_ind_TypeDef),           //0x0C LL_0C_VERSION_IND
    sizeof(llcpdu_0d_reject_ind_TypeDef),            //0x0D LL_0D_REJECT_IND
    sizeof(llcpdu_0e_slave_feature_req_TypeDef),     //0x0E LL_0E_SLAVE_FEATURE_REQ
    sizeof(llcpdu_0f_connection_param_req_TypeDef),  //0x0F LL_0F_CONNECTION_PARAM_REQ
    sizeof(llcpdu_10_connection_param_rsp_TypeDef),  //0x10 LL_10_CONNECTION_PARAM_RSP
    sizeof(llcpdu_11_reject_ext_ind_TypeDef),        //0x11 LL_11_REJECT_EXT_IND
    sizeof(llcpdu_12_ping_req_TypeDef),              //0x12 LL_12_PING_REQ
    sizeof(llcpdu_13_ping_rsp_TypeDef),              //0x13 LL_13_PING_RSP
    sizeof(llcpdu_14_length_req_TypeDef),            //0x14 LL_14_LENGTH_REQ
    sizeof(llcpdu_15_length_rsp_TypeDef),            //0x15 LL_15_LENGTH_RSP
    sizeof(llcpdu_16_phy_req_TypeDef),               //0x16 LL_16_PHY_REQ                 // spec 5.0
    sizeof(llcpdu_17_phy_rsp_TypeDef),               //0x17 LL_17_PHY_RSP                 // spec 5.0
    sizeof(llcpdu_18_phy_update_ind_TypeDef),        //0x18 LL_18_PHY_UPDATE_IND          // spec 5.0
    sizeof(llcpdu_19_min_used_channels_ind_TypeDef), //0x19 LL_19_MIN_USED_CHANNELS_IND   // spec 5.0
    sizeof(llcpdu_1a_cte_req_TypeDef),               //0x1A LL_1A_CTE_REQ                 // spec 5.1
    sizeof(llcpdu_1b_cte_rsp_TypeDef),               //0x1B LL_1B_CTE_RSP                 // spec 5.1
    sizeof(llcpdu_1c_periodic_sync_ind_TypeDef),     //0x1C LL_1C_PERIODIC_SYNC_IND       // spec 5.1
    sizeof(llcpdu_1d_clock_accuracy_req_TypeDef),    //0x1D LL_1D_CLOCK_ACCURACY_REQ      // spec 5.1
    sizeof(llcpdu_1e_clock_accuracy_rsp_TypeDef),    //0x1E LL_1E_CLOCK_ACCURACY_RSP      // spec 5.1
};
*/

/* Private variables ---------------------------------------------------------------------------------------*/


/* Private types -------------------------------------------------------------------------------------------*/
typedef bool (*rxllcpduHandle_func_pointer_TypeDef)(uint8_t, llcpdu_PAYLOAD_TypeDef *, LEACL_TypeDef *);


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
static inline uint16_t lesser_u16(uint16_t a, uint16_t b)
{
    return a > b ? b : a;
}

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
  llc_buffer_tx2air_TypeDef LLcBUFFtx[TOTAL_NUM_LE_LLC_PACKETS_TX];
  llc_buffer_rx_TypeDef     LLcBUFFrx[TOTAL_NUM_LE_LLC_PACKETS_RX];
static uint32_t LLcBUFFtx_RAM_START_ADDRESS[TOTAL_NUM_LE_LLC_PACKETS_TX] = {
    TX_CH0_SRCADDR_LLC,
   #if TOTAL_NUM_LE_LLC_PACKETS_TX >= 2
    TX_CH0_SRCADDR_LLC + 0x0040
   #endif
};

void llcBuffer_tx2air_init(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_TX; i++) {
        llcBuffer_tx2air_free( LLcBUFFtx + i );
    }
}
void llcBuffer_rx_init(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_RX; i++) {
        llcBuffer_rx_free( LLcBUFFrx + i );
    }
}

llc_buffer_tx2air_TypeDef * llcBuffer_tx2air_alloc(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_TX; i++) {
        if (LLcBUFFtx[i].tttstate == 0) { // 0x00:free, 0x01:booked
            LLcBUFFtx[i].ram_start_address = LLcBUFFtx_RAM_START_ADDRESS[i];
            LLcBUFFtx[i].tttstate = 1;    // 0x00:free, 0x01:booked
            return( LLcBUFFtx + i );
        }
    }
    //all used already
            return(0);//NULL:fail
}

uint32_t llcBuffer_tx2air_availableNum(void)
{
    uint32_t i;
    uint32_t availableNum;
             availableNum = 0;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_TX; i++) {
        if (LLcBUFFtx[i].tttstate == 0) { // 0x00:free, 0x01:booked
             availableNum ++;
        }
    }
        return(availableNum);
}

bool irq1isr_llcBuffer_rx_isAvailable(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_RX; i++) {
        if (LLcBUFFrx[i].rrrstate == 0) { // 0x00:free, 0x01:booked
            return(1);
        }
    }
    //all used already
            return(0);
}
llc_buffer_rx_TypeDef * irq1isr_llcBuffer_rx_alloc(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_LLC_PACKETS_RX; i++) {
        if (LLcBUFFrx[i].rrrstate == 0) { // 0x00:free, 0x01:booked
            LLcBUFFrx[i].rrrstate = 1;    // 0x00:free, 0x01:booked
            return( LLcBUFFrx + i );
        }
    }
    //all used already
            return(0);//NULL:fail
}


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
bool PoTxllcPDU_high(uint8_t len, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
        llc_buffer_tx2air_TypeDef *pBuff;
    
        pBuff = llcBuffer_tx2air_alloc();
    if (pBuff == 0)
    {
uart_puts("PoTxllcPDU fail____"); uart_putchar_n('\n');
        return(0);
    }
            pBuff->connection_handle = LEACL_conn_handle(pacl) ;
            pBuff->length            = len ;
        //////////////////
      //
        pdma_ram_bleTx_write_payload (pBuff->ram_start_address + 0x0010, // 0x0010: first 16 bytes for Session Key (SK)
                                      len,
                                      (uint8_t *) pPayload
                                     );
      //
      if (1) //also write, because use (cqBuff->llcpdu_payload.opcode == LL_05_START_ENC_REQ) #if AESCCM_LL_SIGNALING == 1
      {
        unsigned char *src;
        unsigned char *dst;
        src = (unsigned char *) pPayload;
        dst = (unsigned char *) &(pBuff->llcpdu_payload);
        while (len) {
            *dst = *src;
            dst ++; src ++;
            len --;
        }
      }
        //////////////////
        if (RINGBUF_isFull(pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high )) // full Q
        {
            // full Q
            llcBuffer_tx2air_free( pBuff );
            return(0);
        }
        else
        {
            //push this buffer to pacl->llcTxQ_high
            RINGBUF_push( pacl->llcTxQ_high, pBuff, TOTAL_NUM_ELEMENTS_llcTxQ_high );
            return(1);
        }
}

bool PoTxllcPDU(uint8_t len, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
        llc_buffer_tx2air_TypeDef *pBuff;
    
        pBuff = llcBuffer_tx2air_alloc();
    if (pBuff == 0)
    {
uart_puts("PoTxllcPDU fail____"); uart_putchar_n('\n');
        return(0);
    }
            pBuff->connection_handle = LEACL_conn_handle(pacl) ;
            pBuff->length            = len ;
        //////////////////
      //
        pdma_ram_bleTx_write_payload (pBuff->ram_start_address + 0x0010, // 0x0010: first 16 bytes for Session Key (SK)
                                      len,
                                      (uint8_t *) pPayload
                                     );
      //
      if (1) //also write, because use (cqBuff->llcpdu_payload.opcode == LL_05_START_ENC_REQ) #if AESCCM_LL_SIGNALING == 1
      {
        unsigned char *src;
        unsigned char *dst;
        src = (unsigned char *) pPayload;
        dst = (unsigned char *) &(pBuff->llcpdu_payload);
        while (len) {
            *dst = *src;
            dst ++; src ++;
            len --;
        }
      }
        //////////////////
        if (RINGBUF_isFull(pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ )) // full Q
        {
            // full Q
            llcBuffer_tx2air_free( pBuff );
            return(0);
        }
        else
        {
            //push this buffer to pacl->llcTxQ
            RINGBUF_push( pacl->llcTxQ, pBuff, TOTAL_NUM_ELEMENTS_llcTxQ );
            return(1);
        }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
llc_buffer_rx_TypeDef * irq1isr_read_rxfifo_to_llcRxQ(LEACL_TypeDef *pacl)
{
    llc_buffer_rx_TypeDef *pBuff;
    if (pacl->receivedHdr.field.receivedLength == 0) {
        return 0;
    }
        pBuff = irq1isr_llcBuffer_rx_alloc();
    if (pBuff == 0) {
//uart_puts("rxfifo_to_llcRxQ fail____"); uart_putchar_n('\n');
        return 0;
    }
    /////////
    {
            pBuff->received_AESCCM_MIC_F_C0h = RF_RD08(0xC0);//debug
if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
        pdma_ram_bleRx_read_payload(RX_CH1_DESADDR, pacl->receivedHdr.field.receivedLength, (unsigned char *)&(pBuff->llcpdu_payload) );
}
else {
        pdma_ram_bleRx_read_payload(RX_CH1_DESADDR, pacl->receivedHdr.field.receivedLength, (unsigned char *)&(pBuff->llcpdu_payload) );
}
            pBuff->length                    = pacl->receivedHdr.field.receivedLength ;
            pBuff->connection_handle         = LEACL_conn_handle(pacl) ;
    }
    /////////
    //push this buffer to pacl->llcRxQ
    if (RINGBUF_isFull(pacl->llcRxQ, TOTAL_NUM_ELEMENTS_llcRxQ )) // full Q
    {
        // full Q
        llcBuffer_rx_free( pBuff );
        return 0;
    }
    else
    {
        RINGBUF_push( pacl->llcRxQ, pBuff, TOTAL_NUM_ELEMENTS_llcRxQ );
        return pBuff;
    }
}
static inline rxllcpduHandle_func_pointer_TypeDef rxllcpduGetHandler(const uint8_t opcode)
{
    rxllcpduHandle_func_pointer_TypeDef pFun= (rxllcpduHandle_func_pointer_TypeDef)0;
    switch(opcode)
    {
    case 0x00: //LL_00_CONNECTION_UPDATE_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_00_connection_update_ind ;
        break;
    case 0x01: //LL_01_CHANNEL_MAP_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_01_channel_map_ind ;
        break;
    case 0x02: //LL_02_TERMINATE_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_02_terminate_ind ;
        break;
    case 0x03: //LL_03_ENC_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_03_enc_req ;
        break;
    case 0x04: //LL_04_ENC_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_04_enc_rsp ;
        break;
    case 0x05: //LL_05_START_ENC_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_05_start_enc_req ;
        break;
    case 0x06: //LL_06_START_ENC_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_06_start_enc_rsp ;
        break;
    case 0x07: //LL_07_UNKNOWN_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_07_unknown_rsp ;
        break;
    case 0x08: //LL_08_FEATURE_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_08_feature_req ;
        break;
    case 0x09: //LL_09_FEATURE_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_09_feature_rsp ;
        break;
    case 0x0A: //LL_0A_PAUSE_ENC_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0a_pause_enc_req ;
        break;
    case 0x0B: //LL_0B_PAUSE_ENC_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0b_pause_enc_rsp ;
        break;
    case 0x0C: //LL_0C_VERSION_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0c_version_ind ;
        break;
    case 0x0D: //LL_0D_REJECT_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0d_reject_ind ;
        break;
    case 0x0E: //LL_0E_SLAVE_FEATURE_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0e_slave_feature_req ;
        break;
    case 0x0F: //LL_0F_CONNECTION_PARAM_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_0f_connection_param_req ;
        break;
    case 0x10: //LL_10_CONNECTION_PARAM_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_10_connection_param_rsp ;
        break;
    case 0x11: //LL_11_REJECT_EXT_IND:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_11_reject_ext_ind ;
        break;
    case 0x12: //LL_12_PING_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_12_ping_req ;
        break;
    case 0x13: //LL_13_PING_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_13_ping_rsp ;
        break;
    case 0x14: //LL_14_LENGTH_REQ:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_14_length_req ;
        break;
    case 0x15: //LL_15_LENGTH_RSP:
        pFun = (rxllcpduHandle_func_pointer_TypeDef) RxLLcPDU_15_length_rsp ;
        break;
//  case LL_:
//      break;
        
    }
    return pFun ;
}
void process_llcRxQ(LEACL_TypeDef *pacl)
{
    bool done;
    rxllcpduHandle_func_pointer_TypeDef pFun;
    llc_buffer_rx_TypeDef *pBuff;
    if (RINGBUF_isEmpty( pacl->llcRxQ )) // empty q
    {
        return; //exit empty q
    }
        pBuff = (llc_buffer_rx_TypeDef *)( RINGBUF_peek( pacl->llcRxQ, TOTAL_NUM_ELEMENTS_llcRxQ) );
    /////////////////////////
        #if 1 //debug print to comport
        {
            uint8_t i;
            uart_putchar_n('\n');
          {/*
            uint8_t length;
            uart_puts("llcRxQ.len=");
            length = RINGBUF_length( pacl->llcRxQ );
            uart_putu8(length);
            uart_puts(",TxQ.len=");
            length = RINGBUF_length( pacl->llcTxQ );
            uart_putu8(length);
        */}
            uart_puts(",C0h=");
            uart_putu8(pBuff->received_AESCCM_MIC_F_C0h);
            
            uart_puts(",llcRxQ op=");
            uart_putu8(pBuff->llcpdu_payload.opcode);
            uart_puts(",");
          for(i=0; i<(pBuff->length-1); i++) {
            uart_putu8(pBuff->llcpdu_payload.ctrData[i]);
            uart_puts(" ");
          }
            uart_putchar_n('\n');
        }
        #endif
//send_04_hciEvent_3e_ff_LE_kidd_debug(pBuff->length,
//                                     &(pBuff->llcpdu_payload.opcode)
//                                    );
      /*////debug print to Terminal 
        {
            uint8_t i;
            printf("llcRxQ ");
            printf("%02X,",pBuff->llcpdu_payload.opcode);
          for (i=0; i<(pBuff->length-1); i++) {
            printf("%02X ",pBuff->llcpdu_payload.ctrData[i]);
          }
            printf("\n");
        }
      */////////////////////////
            pFun = rxllcpduGetHandler(pBuff->llcpdu_payload.opcode);
        if (pFun == 0)
        {
                //LL/PAC/SLA/BV-01-C [Unknown Packet from Master]
                //LL/PAC/MAS/BV-01-C [Unknown Packet from Slave]
                //If an LL Control PDU is received that is "not supported"            //not supported by the IUT
                //                                      or "reserved for future use", //not in the supported spec
                //  the LL shall respond with an LL_07_UNKNOWN_RSP PDU
                //If an LL Control PDU is received with "wrong length" 
                //                              or with "invalid CtrData" fields, 
                //  the LL may continue with the relevant LL procedure with an implementation-specific interpretation of the data 
                //   If it does not continue the procedure, 
                //      it shall respond with an LL_07_UNKNOWN_RSP PDU or, if the relevant procedure allows it, 
                //                            an LL_0D_REJECT_IND or 
                //                               LL_11_REJECT_EXT_IND PDU
uart_puts("llcRxQ invalid opcode"  ); uart_putchar_n('\n');
// printf("llcRxQ invalid opcode\n");
                done = TxLLcPDU_07_unknown_rsp(pacl, pBuff->llcpdu_payload.opcode); //unknownType
        }
        else
        {
          /*
            if (pBuff->length < LLCPDU_SIZEOF_PAYLOAD[ pBuff->llcpdu_payload.opcode ])
            {
                //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
                //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
                //step 2 If the valid length of CtrData for the current opcode is greater than 0, execute steps 5-9 for 
                //       CtrData length equal to 0 and for CtrData length equal to (the valid length - 1)
                //step 6 option (a) or (d)
uart_puts("llcRxQ invalid length"  ); uart_putchar_n('\n');
   printf("llcRxQ invalid length\n");
                done = TxLLcPDU_07_unknown_rsp(pacl, pBuff->llcpdu_payload.opcode); //unknownType
            }
            else
          */
            {
                done = pFun( pBuff->length,         //actually no need length
                                                    //      need check length for BQB test
                                                    //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
                                                    //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
                             &(pBuff->llcpdu_payload),
                             pacl
                           );
            }
        }
        /////////////////////////
        if (done) {
            llcBuffer_rx_free( pBuff );
            RINGBUF_incrementPopIndex( pacl->llcRxQ ); // ridx ++   pop Q
        }
}

bool RxLLcPDU_00_connection_update_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    LLCONNECTIONUPDATE_TypeDef *pUpdat;
    llcpdu_00_connection_update_ind_TypeDef *pInd00 = (llcpdu_00_connection_update_ind_TypeDef *)pPayload;
    if (LEACL_is_role_master(pacl)) {
            //LL/PAC/MAS/BV-01-C [Unknown Packet from Slave]
            //invalid for the current role (e.g., LL_00_CONNECTION_UPDATE_IND or LL_01_CHANNEL_MAP_IND sent to a Master IUT)
            //invalid for Master IUT to rx  LL_00_CONNECTION_UPDATE_IND
            done = TxLLcPDU_07_unknown_rsp(pacl, LL_00_CONNECTION_UPDATE_IND); //unknownType
            return (done);
    }
    else {
        if (LLC_INITIATE_PROCEDURE_is_involve_instant(pacl)) {
            //       Slave shall proceed to handle the remote Master-initiated procedure B and take no further action in the Slave-initiated procedure A
            //later, Slave would receive RxLLcPDU_11_reject_ext_ind()
        }
    }
    ////////
    if (pacl->pNewUpdate == 0)
        #if    __MALLOC_METHOD__  == 1
        pUpdat = (LLCONNECTIONUPDATE_TypeDef *)      malloc(sizeof(LLCONNECTIONUPDATE_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pUpdat = (LLCONNECTIONUPDATE_TypeDef *)pvPortMalloc(sizeof(LLCONNECTIONUPDATE_TypeDef));
        #else
        ...
        #endif
    else
        pUpdat = pacl->pNewUpdate ;
    if (pUpdat != 0)
    {
    //first set value
    //final assign the pointer pNewUpdate = pUpdat
        pUpdat->winSize   = pInd00->winSize;
        pUpdat->winOffset = pInd00->winOffset;
        pUpdat->interval  = pInd00->interval;
        pUpdat->latency   = pInd00->latency;
        pUpdat->timeout   = pInd00->timeout;
        pUpdat->instant   = pInd00->instant;
        //       Slave
        pUpdat->iSlave_updateState = 0x00;
        pUpdat->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger = 0;

    //final assign the pointer pNewUpdate = pUpdat
        pacl->pNewUpdate = pUpdat; //Rx LL_00_CONNECTION_UPDATE_IND
        
      //if (LEACL_is_role_slave(pacl)) //above checked
      //{
        if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0F_CONNECTION_PARAM_REQ)) { //Conn Params Request procedure
            //Slave Tx LL_0F_CONNECTION_PARAM_REQ     start InitiateProcedure
            //      Rx LL_00_CONNECTION_UPDATE_IND complete InitiateProcedure
            LLC_INITIATE_PROCEDURE_end_complete(pacl);
            //                                           then start RespondProcedure
            LLC_RESPOND_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT); //Conn Update procedure, Slave Rx LL_00_CONNECTION_UPDATE_IND
        }
        else {
            LLC_RESPOND_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT); //Conn Update procedure, Slave Rx LL_00_CONNECTION_UPDATE_IND
        }
      //}//end Slave
        done = 1 ;
    }
    else
    {
        done = 0 ;
    }
    return (done);
}

bool RxLLcPDU_01_channel_map_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    LLCHANNELMAP_TypeDef *pMap;
    llcpdu_01_channel_map_ind_TypeDef *pInd01 = (llcpdu_01_channel_map_ind_TypeDef *)pPayload;
    if (LEACL_is_role_master(pacl)) {
            //LL/PAC/MAS/BV-01-C [Unknown Packet from Slave]
            //invalid for the current role (e.g., LL_00_CONNECTION_UPDATE_IND or LL_01_CHANNEL_MAP_IND sent to a Master IUT)
            //invalid for Master IUT to rx  LL_01_CHANNEL_MAP_IND
            done = TxLLcPDU_07_unknown_rsp(pacl, LL_01_CHANNEL_MAP_IND); //unknownType
            return (done);
    }
    else {
            //LL/CON/SLA/BV-27-C [Initiating Conn Param Request - different procedure collision - channel map update]
        if (LLC_INITIATE_PROCEDURE_is_involve_instant(pacl)) {
            //       Slave shall proceed to handle the remote Master-initiated procedure B and take no further action in the Slave-initiated procedure A
            //later, Slave would receive RxLLcPDU_11_reject_ext_ind()
        }
    }
    ////////
    if (pacl->pNewChM == 0)
        #if    __MALLOC_METHOD__  == 1
        pMap = (LLCHANNELMAP_TypeDef *)      malloc(sizeof(LLCHANNELMAP_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pMap = (LLCHANNELMAP_TypeDef *)pvPortMalloc(sizeof(LLCHANNELMAP_TypeDef));
        #else
        ...
        #endif
    else
        pMap = pacl->pNewChM ;
    if (pMap != 0)
    {
        pMap->chM[0]  = pInd01->chM[0];
        pMap->chM[1]  = pInd01->chM[1];
        pMap->chM[2]  = pInd01->chM[2];
        pMap->chM[3]  = pInd01->chM[3];
        pMap->chM[4]  = pInd01->chM[4];
        pMap->instant = pInd01->instant;
        
        pacl->pNewChM = pMap;
        
        calc_usedChannel_byRemapIndex(pacl->pNewChM);
        
        LLC_RESPOND_PROCEDURE_start(pacl, LL_01_CHANNEL_MAP_IND | FLAG_INVOLVE_INSTANT); //Channel Map Update procedure, Slave Rx LL_01_CHANNEL_MAP_IND
        done = 1 ;
    }
    else
    {
        done = 0 ;
    }
    return (done);
}

bool RxLLcPDU_02_terminate_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
  //job moved to pacl->irq1_rxLLTerminateInd_txAck
  //uint8_t errorCode;
  //errorCode = ht_read_byte( pPayload->ctrData+0 ); //uint8_t errorCode; //ctrData[0]
  //        done = send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), errorCode ); //ERR_13_OTHER_END_TERMINATED_CONN_USER_END
            done = 1;
    return (done);
}

bool RxLLcPDU_03_enc_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_03_enc_req_TypeDef *pReq03 = (llcpdu_03_enc_req_TypeDef *)pPayload;
    if (LEACL_is_role_slave(pacl))
    {
        if (0 == LOCAL_FEATURE_is_supported_LE_encryption())
        {
            //If encryption is not supported by slave LL, slave LL shall send an LL_REJECT_IND or LL_REJECT_EXT_IND
            //with ErrorCode set to Unsupported Remote Feature / Unsupported LMP Feature (0x1A)
            //LL/SEC/MAS/BV-03-C [Master Receiving LL_REJECT_IND]      ERR_1A_UNSUPPORTED_REMOTE_FEATURE
            return TxLLcPDU_11RejectExtInd_or_0dRejectInd(pacl, LL_03_ENC_REQ, //rejectOpcode
                                                                0x1A           //errorCode   ERR_1A_UNSUPPORTED_REMOTE_FEATURE
                                                         );
        }
    }
            ht_memory_copy( pacl->encrypt.rand, pReq03->rand, 8 );
            ht_memory_copy( pacl->encrypt.ediv, pReq03->ediv, 2 );
            ht_memory_copy( pacl->encrypt.SKDm, pReq03->SKDm, 8 );
            ht_memory_copy( pacl->encrypt.IVm,  pReq03->IVm,  4 );

        if (llcBuffer_tx2air_availableNum() < 2) {
            return (0);
        }
          //done = 1;                                  //The Lower Tester//LL/SEC/MAS/BI-01-C [Master Encryption Setup: Missing Response] ERR_22_LL_RESPONSE_TIMEOUT
          //done = TxLLcPDU_0E_slave_feature_req(pacl);//The Lower Tester//LL/SEC/MAS/BV-13-C [Master Start Encryption: Overlapping Procedure with LL_SLAVE_FEATURE_REQ]
            done = TxLLcPDU_04_enc_rsp(pacl);
        if (done) {
            //Slave Rx LL_03_ENC_REQ     start RespondProcedure
            //      Tx LL_04_ENC_RSP
            //      tx request event to host, wait reply
            LLC_RESPOND_PROCEDURE_start(pacl, LL_03_ENC_REQ | FLAG_INVOLVE_INSTANT); //Encryption Start procedure, Slave Rx LL_03_ENC_REQ
          //
          //done = TxLLcPDU_0c_version_ind(pacl);//The Lower Tester//LL/SEC/MAS/BV-14-C [Master Receiving unexpected Data Channel PDU during encryption start] 
            send_04_hciEvent_3e_05_LE_LTK_request(pacl);
          /*
            hcicmd_201a_LE_LTK_request_reply_TypeDef cmd =
            {
              //.long_term_key[]
            };
                ht_write_hword( (uint8_t *)&(cmd.conn_handle), LEACL_conn_handle(pacl));
                ht_memory_copy( cmd.long_term_key+0, vol6_partc_1_LTK_lso_to_mso+0, 16);
            hcmd_201a_le_LTK_request_reply((HCI_01_COMMAND_PACKET_TypeDef *)(&cmd));
          */
        }
    return (done);
}

bool RxLLcPDU_04_enc_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_04_enc_rsp_TypeDef *pRsp04 = (llcpdu_04_enc_rsp_TypeDef *)pPayload;

            ht_memory_copy(pacl->encrypt.SKDs+0, pRsp04->SKDs+0, 8 );
            ht_memory_copy(pacl->encrypt.IVs+0,  pRsp04->IVs+0,  4 );
        if( LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x03) || //LL_03_ENC_REQ
            LLC_INITIATE_PROCEDURE_is_llcOP(pacl, 0x0A) )  //LL_0A_PAUSE_ENC_REQ
        {
            LLC_INITIATE_PROCEDURE_set_peerRespondAtLeastOnePdu(pacl);
        }
            calc_pacl_encrypt_SK(pacl);
            done = 1;
    return (done);
}

bool RxLLcPDU_05_start_enc_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
  //done = 1;// The Lower Tester  //LL/SEC/SLA/BI-01-C [Slave Encryption Setup: Missing Response]      ERR_22_LL_RESPONSE_TIMEOUT
    done = TxLLcPDU_06_start_enc_rsp(pacl);
    return (done);
}

bool RxLLcPDU_06_start_enc_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    if (LEACL_is_role_slave(pacl)) {
          //done = 1;// The Lower Tester  //LL/SEC/MAS/BI-04-C [Master Encryption Setup: Missing Acknowledgement]      ERR_22_LL_RESPONSE_TIMEOUT
            done = TxLLcPDU_06_start_enc_rsp(pacl);
        if (done) {
            //Slave  Rx LL_03_ENC_REQ               start RespondProcedure
            //       Tx LL_04_ENC_RSP
            //       tx request event to host, wait reply
            //       Tx LL_05_START_ENC_REQ
            //       Rx LL_06_START_ENC_RSP $$$     complete RespondProcedure
            //       Tx LL_06_START_ENC_RSP
            //Encryption start procedure is complete in Slave when Slave receives the LL_START_ENC_RSP from Master
            LLC_RESPOND_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_06_START_ENC_RSP
        }
    }
    else {
            done = 1;
            done = TxLLcPDU_12_ping_req(pacl);//debug
        if (done) {
            //Master Tx LL_03_ENC_REQ               start InitiateProcedure
            //       Rx LL_04_ENC_RSP
            //       Rx LL_05_START_ENC_REQ
            //       Tx LL_06_START_ENC_RSP
            //       Rx LL_06_START_ENC_RSP $$$     complete InitiateProcedure
            //Encryption start procedure is complete in Master when Master receives the LL_START_ENC_RSP from Slave
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_06_START_ENC_RSP
        }
    }
    //
    if (done) {
        if ( pacl->flag0.field.ever_pause_encryption == 0 ) {
            send_04_hciEvent_08_encryption_change(LEACL_conn_handle(pacl), 
                                                  0x00, //ERR_00_SUCCESS
                                                  0x01  //Encryption_Enabled, 0x01 Link Level Encryption is ON with AES-CCM for LE
                                                 );
        }
        else {
            send_04_hciEvent_30_encryption_key_refresh_complete(LEACL_conn_handle(pacl), 
                                                  0x00  //ERR_00_SUCCESS
                                                 );
        }
    }
    return (done);
}

bool RxLLcPDU_07_unknown_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    uint8_t unknownType;
    unknownType = ht_read_byte( pPayload->ctrData+0 ); //uint8_t unknownType; //ctrData[0]
    
    //LL/CON/SLA/BV-85-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
    LEACL_set_remote_said_unknownType(pacl, unknownType); //set corresponding bit @ remote_said_unknownType[6]
    
    switch (unknownType)
    {
    case 0x08://LL_08_FEATURE_REQ:
    case 0x0E://LL_0E_SLAVE_FEATURE_REQ:
            //LL/CON/SLA/BV-23-C [Initiate Feature Exchange - Master does not support]
        if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
            done = send_04_hciEvent_3e_04_LE_read_remote_feature_complete(pacl, 0x1A); //ERR_1A_UNSUPPORTED_REMOTE_FEATURE
        }
        else {
            done = 1;
        }
        if (done) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_07_UNKNOWN_RSP
        }
        break;
    case 0x0C://LL_0C_VERSION_IND:
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_07_UNKNOWN_RSP
            done = 1;
        break;
    case 0x0F://LL_0F_CONNECTION_PARAM_REQ:
        if (LEACL_is_role_master(pacl)) {
            //Master
            //LL/CON/MAS/BV-81-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
          //if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0F_CONNECTION_PARAM_REQ)) { //Conn Params Request procedure
                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Tx LL_0F_CONNECTION_PARAM_REQ     start InitiateProcedure
                    //       Rx LL_07_UNKNOWN_RSP
                    //       Tx LL_00_CONNECTION_UPDATE_IND          InitiateProcedure transition to Conn Update procedure
                    LLC_INITIATE_PROCEDURE_transition_to_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND);       //Conn Update procedure
                }
          //}
        }
        else {
            //Slave
            //LL/CON/SLA/BV-85-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
          //if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0F_CONNECTION_PARAM_REQ)) { //Conn Params Request procedure
                if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
                    done = send_04_hciEvent_3e_03_LE_conn_update_complete(pacl, 0x1A); //ERR_1A_UNSUPPORTED_REMOTE_FEATURE
                }
                else {
                    done = 1;
                }
                if (done) {
                    LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_07_UNKNOWN_RSP
                }
          //}
        }
        break;
    case 0x12://LL_12_PING_REQ:
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_07_UNKNOWN_RSP
            //LL/SEC/SLA/BV-10-C [Initiate LE Ping procedure when the other side does not support the procedure]
            //LL/SEC/MAS/BV-10-C [Initiate LE Ping procedure when the other side does not support the procedure]
            //The Lower Tester responds with LL_07_UNKNOWN_RSP
            done = 1;
        break;
    case 0x14://LL_14_LENGTH_REQ:
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_07_UNKNOWN_RSP
            done = 1;
        break;
    default:
            done = 1;
        break;
    }
    return (done);
}

bool RxLLcPDU_08_feature_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    //responding procedure
    bool done;
    if (rxdlen < sizeof(llcpdu_08_feature_req_TypeDef)) {
        //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
        //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
        //step 6 option (a) The Lower Tester receives an LL_07_UNKNOWN_RSP PDU from the IUT
        done = TxLLcPDU_07_unknown_rsp(pacl, LL_08_FEATURE_REQ); //unknownType
        return (done);
    }
  //else
  //if (rxdlen > sizeof(llcpdu_08_feature_req_TypeDef)) {
        //step 6 option (c) The Lower Tester receives a PDU from the IUT that would be a response to the opcode sent in step 5 with VALID parameters
        //continue, adopt this PDU
  //}
    //save
    if (pacl->pRemoteFeature == 0) {
        pacl->pRemoteFeature = (REMOTE_FEATURES_TypeDef *) malloc(sizeof(REMOTE_FEATURES_TypeDef));
    }
    if (pacl->pRemoteFeature) {
        ht_memory_copy((pacl->pRemoteFeature)->featureSet+0, pPayload->ctrData, 8);
    }
    //then respond
      //
        done = TxLLcPDU_09_feature_rsp(pacl);
      /*
        //LL/CON/MAS/BV-19-C [Conn Control Timeout]
        //The Lower Tester will ACK LL_FEATURE_REQ packet. However, it will not send LL_FEATURE_RSP
        done = 1;
      */
    return (done);
}

bool RxLLcPDU_09_feature_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    //save
    if (rxdlen >= sizeof(llcpdu_09_feature_rsp_TypeDef)) {
        if (pacl->pRemoteFeature == 0) {
            pacl->pRemoteFeature = (REMOTE_FEATURES_TypeDef *) malloc(sizeof(REMOTE_FEATURES_TypeDef));
        }
        if (pacl->pRemoteFeature) {
            ht_memory_copy((pacl->pRemoteFeature)->featureSet+0, pPayload->ctrData, 8);
        }
    }
//debug
//debug_send_m2s_1(pacl);        //debug
//TxLLcPDU_08_feature_req(pacl); //debug
    if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_08_FEATURE_REQ)) //Feature Exchange procedure
    {
    //llcInitiateProcedure is complete
        uint8_t reason;
            reason = 0x00; //ERR_00_SUCCESS
        if (rxdlen < sizeof(llcpdu_09_feature_rsp_TypeDef)) {
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (d) The Lower Tester does NOT receive an LL Control PDU from the IUT for 20 seconds (i.e., half the procedure response timeout)
            reason = 0x19; //ERR_19_UNKNOWN_LMP_PDU
        }
      //else
      //if (rxdlen > sizeof(llcpdu_09_feature_rsp_TypeDef)) {
            //step 6 option (d) The Lower Tester does NOT receive an LL Control PDU from the IUT for 20 seconds (i.e., half the procedure response timeout)
            //continue, adopt this PDU
      //}
        //
        if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
            done = send_04_hciEvent_3e_04_LE_read_remote_feature_complete(pacl, reason);
        }
        else {
            done = 1;
        }
        ////////
        if (done) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_09_FEATURE_RSP
        }
        return (done);
    }
    else
    {
        //ERROR: llcInitiateProcedure is not Feature Exchange procedure,
        //       but, roger LL_09_FEATURE_RSP
        if (rxdlen < sizeof(llcpdu_09_feature_rsp_TypeDef)) {
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (a) The Lower Tester receives an LL_07_UNKNOWN_RSP PDU from the IUT
            done = TxLLcPDU_07_unknown_rsp(pacl, LL_09_FEATURE_RSP); //unknownType
        }
        else {
            done = 1;
        }
        return (done);
    }
}

bool RxLLcPDU_0a_pause_enc_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
            //Slave  Rx LL_0A_PAUSE_ENC_REQ $$$     start RespondProcedure
            //       Tx LL_0B_PAUSE_ENC_RSP
            //       Rx LL_0B_PAUSE_ENC_RSP
            //       Rx LL_03_ENC_REQ
            //       Tx LL_04_ENC_RSP
            //       tx request event to host, wait reply
            //       Tx LL_05_START_ENC_REQ
            //       Rx LL_06_START_ENC_RSP         complete RespondProcedure
            //       Tx LL_06_START_ENC_RSP
            done = TxLLcPDU_0b_pause_enc_rsp(pacl);
        if (done) {
            LLC_RESPOND_PROCEDURE_start(pacl, LL_0A_PAUSE_ENC_REQ | FLAG_INVOLVE_INSTANT); //Encryption Pause procedure, Slave Rx LL_0A_PAUSE_ENC_REQ
        }
    return (done);
}

bool RxLLcPDU_0b_pause_enc_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    bool don2;
            pacl->flag0.field.ever_pause_encryption = 1;
    if (LEACL_is_role_slave(pacl)) {
            done = 1;
        if (done) {
            //Slave  Rx LL_0A_PAUSE_ENC_REQ         start RespondProcedure
            //       Tx LL_0B_PAUSE_ENC_RSP
            //       Rx LL_0B_PAUSE_ENC_RSP $$$
            //       Rx LL_03_ENC_REQ
            //       Tx LL_04_ENC_RSP
            //       tx request event to host, wait reply
            //       Tx LL_05_START_ENC_REQ
            //       Rx LL_06_START_ENC_RSP         complete RespondProcedure
            //       Tx LL_06_START_ENC_RSP
            LLC_RESPOND_PROCEDURE_transition_to_llcOP(pacl, LL_03_ENC_REQ);       //Encryption Start procedure
        }
    }
    else {
        if (llcBuffer_tx2air_availableNum() < 2) {
            return (0);
        }
            done = TxLLcPDU_0b_pause_enc_rsp(pacl);
        if (done) {
            //Master Tx LL_0A_PAUSE_ENC_REQ         start InitiateProcedure
            //       Rx LL_0B_PAUSE_ENC_RSP $$$
            //       Tx LL_0B_PAUSE_ENC_RSP
            //       Tx LL_03_ENC_REQ
            //       Rx LL_04_ENC_RSP
            //       Rx LL_05_START_ENC_REQ
            //       Tx LL_06_START_ENC_RSP
            //       Rx LL_06_START_ENC_RSP         complete InitiateProcedure
        //directly TxLLcPDU_03_enc_req(), instead of llcmsgQ_push(LLCMSG_INITIATE_LLC_START_ENCRYPTION_PROCEDURE,,) which use high_priority == 0
            don2 = TxLLcPDU_03_enc_req(pacl, 1);//1:high_priority
            if (don2) {
                LLC_INITIATE_PROCEDURE_transition_to_llcOP(pacl, LL_03_ENC_REQ);       //Encryption Start procedure
                LLC_INITIATE_PROCEDURE_set_peerRespondAtLeastOnePdu(pacl);
            }
            return (don2);
        }
    }
    return (done);
}

bool RxLLcPDU_0c_version_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0c_version_ind_TypeDef *pInd0c = (llcpdu_0c_version_ind_TypeDef *)pPayload;
    //save
    if (rxdlen >= sizeof(llcpdu_0c_version_ind_TypeDef)) {
        if (pacl->pRemoteVersInfo == 0) {
            pacl->pRemoteVersInfo = (REMOTE_VERSION_INFO_TypeDef *) malloc(sizeof(REMOTE_VERSION_INFO_TypeDef));
        }
        if (pacl->pRemoteVersInfo) {
            pacl->pRemoteVersInfo->versNr    = pInd0c->llVersNr;
            pacl->pRemoteVersInfo->compId    = pInd0c->compId;
            pacl->pRemoteVersInfo->subVersNr = pInd0c->llSubVersNr;
//          (pacl->pRemoteVersInfo)->versNr    = ht_read_byte (pPayload->ctrData+0);//ctrData[0]
//          (pacl->pRemoteVersInfo)->compId    = ht_read_hword(pPayload->ctrData+1);//ctrData[1~2]
//          (pacl->pRemoteVersInfo)->subVersNr = ht_read_hword(pPayload->ctrData+3);//ctrData[3~4]
        }
    }
    //action
    if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0C_VERSION_IND)) //Version Exchange procedure
    {
    //llcInitiateProcedure is complete
        uint8_t reason;
            reason = 0x00; //ERR_00_SUCCESS
        if (rxdlen < sizeof(llcpdu_0c_version_ind_TypeDef)) {
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (d) The Lower Tester does NOT receive an LL Control PDU from the IUT for 20 seconds (i.e., half the procedure response timeout)
            reason = 0x19; //ERR_19_UNKNOWN_LMP_PDU
        }
        else
        if (rxdlen > sizeof(llcpdu_0c_version_ind_TypeDef)) {
            //step 6 option (d) The Lower Tester does NOT receive an LL Control PDU from the IUT for 20 seconds (i.e., half the procedure response timeout)
            //continue, adopt this PDU
        }
        //
        if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
            done = send_04_hciEvent_0c_read_remote_version_info_complete(pacl, reason);
        }
        else {
            done = 1;
        }
        ////////
        if (done) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_0C_VERSION_IND
        }
        return (done);
    }
    else
    {
    //responding procedure
        if (rxdlen < sizeof(llcpdu_0c_version_ind_TypeDef)) {
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (a) The Lower Tester receives an LL_07_UNKNOWN_RSP PDU from the IUT
            done = TxLLcPDU_07_unknown_rsp(pacl, LL_0C_VERSION_IND); //unknownType
        }
        else
        if (rxdlen > sizeof(llcpdu_0c_version_ind_TypeDef)) {
            //step 6 option (c) The Lower Tester receives a PDU from the IUT that would be a response to the opcode sent in step 5 with VALID parameters
            //continue, adopt this PDU
            done = TxLLcPDU_0c_version_ind(pacl);
        }
        else
        if (   LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_03_ENC_REQ)
            && LLC_INITIATE_PROCEDURE_is_peerRespondAtLeastOnePdu(pacl)
           ) {
            //LL/SEC/MAS/BV-14-C [Master Receiving unexpected PDU during encryption start]
            done = send_04_hciEvent_05_disconnection_complete( LEACL_conn_handle(pacl), 0x3D ); //ERR_3D_CONN_TERMINATED_DUE_TO_MIC_FAILURE
        }
        else {
            done = TxLLcPDU_0c_version_ind(pacl);
        }
        return (done);
    }
}

bool RxLLcPDU_0d_reject_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    uint8_t errorCode;
    errorCode = ht_read_byte( pPayload->ctrData+0 ); //uint8_t errorCode   ; //ctrData[0]
    done = 1;
    if (LLC_INITIATE_PROCEDURE_is_active(pacl))
    {
    switch (pacl->llcInitiateProcedure.state.field.llcOP)
    {
    case 0x03://LL_03_ENC_REQ:              //Encryption Start procedure
        if (LEACL_is_role_master(pacl))
        {
          //if (errorCode == 0x1A)     //ERR_1A_UNSUPPORTED_REMOTE_FEATURE, remote does not support the feature associated with the issued command, LMP or LL Control PDU
          //if (errorCode == 0x06)     //ERR_06_PIN_OR_KEY_MISSING
            {
                //LL/SEC/MAS/BV-03-C [Master Receiving LL_REJECT_IND]      ERR_1A_UNSUPPORTED_REMOTE_FEATURE
                //LL/SEC/MAS/BV-11-C [Master Receiving LL_REJECT_EXT_IND]  ERR_06_PIN_OR_KEY_MISSING
                    done = send_04_hciEvent_08_encryption_change(LEACL_conn_handle(pacl), 
                                                                 errorCode,
                                                                 0x00  //Encryption_Enabled, 0x01 Link Level Encryption is ON with AES-CCM for LE
                                                                );
            }
                if (done) {
                    //Master Tx LL_03_ENC_REQ     start InitiateProcedure
                    //       Rx LL_0D_REJECT_IND or LL_11_REJECT_EXT_IND
                    //Encryption start procedure is complete in Master when Master receives the LL_REJECT_IND or LL_REJECT_EXT_IND from Slave
                    LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_0D_REJECT_IND
                }
        }
        break;
    case 0x0F://LL_0F_CONNECTION_PARAM_REQ: //Conn Params Request procedure
        if (LEACL_is_role_master(pacl)
            && (    (errorCode == 0x1A)     //ERR_1A_UNSUPPORTED_REMOTE_FEATURE, remote does not support the feature associated with the issued command, LMP or LL Control PDU
                                            //                                   master skip param asking and then directly send LL_00_CONNECTION_UPDATE_IND
                                            //LL/CON/MAS/BV-29-C [Initiating Conn Param Request - remote legacy host]
              // || (errorCode == 0x3B)     //ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                            //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#2 Master can then send LL_00_CONNECTION_UPDATE_IND
               )
           )
        {
                //LL/CON/MAS/BV-29-C [Initiating Conn Param Request - remote legacy host]
                //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#2 Master can then send LL_00_CONNECTION_UPDATE_IND

                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Tx LL_0F_CONNECTION_PARAM_REQ     start InitiateProcedure
                    //       Rx LL_0D_REJECT_IND
                    //       Tx LL_00_CONNECTION_UPDATE_IND          InitiateProcedure transition to Conn Update procedure
                    LLC_INITIATE_PROCEDURE_transition_to_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND);       //Conn Update procedure
                }
        }
        else
        {
                //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#1
                //LL/CON/SLA/BV-25-C [Initiating Conn Param Request - Reject]
                if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
                    done = send_04_hciEvent_3e_03_LE_conn_update_complete( pacl, errorCode );
                }
                else {
                    done = 1;
                }
                if (done) {
                    LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_11_REJECT_EXT_IND
                }
        }
        break;
    default:
            done = 1;
        break;
    }//end switch
    }//end LLC_INITIATE_PROCEDURE_is_active
    return (done);
}

bool RxLLcPDU_0e_slave_feature_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    //responding procedure
    bool done;
    if ((0 == LOCAL_FEATURE_is_supported_slave_initiated_features_exchange()) ||
        (rxdlen < sizeof(llcpdu_08_feature_req_TypeDef)) ||
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (a) The Lower Tester receives an LL_07_UNKNOWN_RSP PDU from the IUT
        LEACL_is_role_slave(pacl)
            //LL/PAC/SLA/BV-01-C [Unknown Packet from Master]
            //invalid for the current role (e.g., LL_0E_SLAVE_FEATURE_REQ sent to a Slave IUT)
            //invalid for Slave IUT to rx   LL_0E_SLAVE_FEATURE_REQ
       )
    {
        return TxLLcPDU_07_unknown_rsp(pacl, LL_0E_SLAVE_FEATURE_REQ); //unknownType
    }
  //else
  //if (rxdlen > sizeof(llcpdu_08_feature_req_TypeDef)) {
        //step 6 option (c) The Lower Tester receives a PDU from the IUT that would be a response to the opcode sent in step 5 with VALID parameters
        //continue, adopt this PDU
  //}
    ////////
    //save
    if (pacl->pRemoteFeature == 0) {
        pacl->pRemoteFeature = (REMOTE_FEATURES_TypeDef *) malloc(sizeof(REMOTE_FEATURES_TypeDef));
    }
    if (pacl->pRemoteFeature) {
        ht_memory_copy((pacl->pRemoteFeature)->featureSet+0, pPayload->ctrData, 8);
    }
    //then respond
        done = TxLLcPDU_09_feature_rsp(pacl);
    return (done);
}

bool RxLLcPDU_0f_connection_param_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0f_connection_param_req_TypeDef *pReq0f = (llcpdu_0f_connection_param_req_TypeDef *)pPayload;
    if (0 == LOCAL_FEATURE_is_supported_conn_params_request_procedure()) {
                return TxLLcPDU_07_unknown_rsp(pacl, LL_0F_CONNECTION_PARAM_REQ); //unknownType
    }
    if (0 == EVTMASK_is_allowed_LE_remote_conn_param_request()) {
                //LL/CON/MAS/BV-34-C [ Accepting Conn Param Request - event masked]
                //LL/CON/SLA/BV-33-C [ Accepting Conn Param Request - event masked]
                return TxLLcPDU_11RejectExtInd_or_0dRejectInd(pacl, LL_0F_CONNECTION_PARAM_REQ, //rejectOpcode
                                                                    0x1A                        //errorCode   ERR_1A_UNSUPPORTED_REMOTE_FEATURE
                                                             );
    }
    if (LLC_INITIATE_PROCEDURE_is_active(pacl) &&
        LLC_INITIATE_PROCEDURE_is_involve_instant(pacl) )
    {
        if (LEACL_is_role_master(pacl)) {
            //Master shall reject the PDU received from the remote Slave by issuing an LL_11_REJECT_EXT_IND or LL_0D_REJECT_IND PDU.
            //    It shall then proceed with Master-initiated procedure
            if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0F_CONNECTION_PARAM_REQ)) {
                //LL/CON/MAS/BV-26-C [Initiating Conn Param Request - same procedure collision]
                return TxLLcPDU_11RejectExtInd_or_0dRejectInd(pacl, LL_0F_CONNECTION_PARAM_REQ, //rejectOpcode
                                                                    0x23                        //errorCode   ERR_23_LL_PROCEDURE_COLLISION
                                                             );
            }
            else {
                //LL/CON/MAS/BV-27-C [Initiating Conn Param Request - different procedure collision - channel map update]
                return TxLLcPDU_11RejectExtInd_or_0dRejectInd(pacl, LL_0F_CONNECTION_PARAM_REQ, //rejectOpcode
                                                                    0x2A                        //errorCode   ERR_2A_DIFFERENT_TRANSACTION_COLLISION
                                                             );
            }
        }
        else {
            //       Slave shall proceed to handle the remote Master-initiated procedure B and take no further action in the Slave-initiated procedure A
            //later, Slave would receive RxLLcPDU_11_reject_ext_ind()
        }
    }
    ////////
    //save
    if (pacl->pTemplateConnParam == 0) {
        #if    __MALLOC_METHOD__  == 1
        pacl->pTemplateConnParam = (LLCONNECTIONPARAMS_TEMPLATE_TypeDef *)      malloc(sizeof(LLCONNECTIONPARAMS_TEMPLATE_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pacl->pTemplateConnParam = (LLCONNECTIONPARAMS_TEMPLATE_TypeDef *)pvPortMalloc(sizeof(LLCONNECTIONPARAMS_TEMPLATE_TypeDef));
        #else
        ...
        #endif
    }
    if (pacl->pTemplateConnParam != 0) {
        pacl->pTemplateConnParam->minInterval             = pReq0f->minInterval;
        pacl->pTemplateConnParam->maxInterval             = pReq0f->maxInterval;
        pacl->pTemplateConnParam->latency                 = pReq0f->latency;
        pacl->pTemplateConnParam->timeout                 = pReq0f->timeout;
        pacl->pTemplateConnParam->preferredPeriodicity    = pReq0f->preferredPeriodicity;
        pacl->pTemplateConnParam->referenceConnEventCount = pReq0f->referenceConnEventCount;
        pacl->pTemplateConnParam->offset0                 = pReq0f->offset0;
        pacl->pTemplateConnParam->offset1                 = pReq0f->offset1;
        pacl->pTemplateConnParam->offset2                 = pReq0f->offset2;
        pacl->pTemplateConnParam->offset3                 = pReq0f->offset3;
        pacl->pTemplateConnParam->offset4                 = pReq0f->offset4;
        pacl->pTemplateConnParam->offset5                 = pReq0f->offset5;
    }
    ////////
    if (LEACL_is_role_master(pacl))
    {
        //Master shall not send the LL_10_CONNECTION_PARAM_RSP
        if ( (pReq0f->minInterval <= pacl->currParam.interval &&
                                     pacl->currParam.interval <= pReq0f->maxInterval) && 
             (pReq0f->latency == pacl->currParam.latency) &&
             (pReq0f->timeout == pacl->currParam.timeout) &&
             (pReq0f->preferredPeriodicity == 0) && // A value of 0 means no preference
             (pReq0f->offset0 != 0xFFFF ||
              pReq0f->offset1 != 0xFFFF ||
              pReq0f->offset2 != 0xFFFF ||
              pReq0f->offset3 != 0xFFFF ||
              pReq0f->offset4 != 0xFFFF ||
              pReq0f->offset5 != 0xFFFF )
           ) {
                //If the received LL_0F_CONNECTION_PARAM_REQ PDU requests only a change in the anchor points of the LE connection, 
                //   then the LL shall not indicate this request to its Host
                //                                       send_04_hciEvent_3e_06_LE_remote_conn_param_request()
                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure, but no need LLC_RESPOND_PROCEDURE_start()
                    //       Tx LL_00_CONNECTION_UPDATE_IND    start InitiateProcedure
                    LLC_INITIATE_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT); //Conn Update procedure
                }
        }
        else
        if (1) {
                done = send_04_hciEvent_3e_06_LE_remote_conn_param_request(pacl);
            if (done) {
                //Master Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure
                //       tx request event to host, wait reply
                LLC_RESPOND_PROCEDURE_start(pacl, LL_0F_CONNECTION_PARAM_REQ | FLAG_INVOLVE_INSTANT); //Conn Params Request procedure, Master Rx LL_0F_CONNECTION_PARAM_REQ
            }
        }
        else {
            done = TxLLcPDU_11_reject_ext_ind(pacl, LL_0F_CONNECTION_PARAM_REQ,   //rejectOpcode
                                                    0x3B                          //errorCode   ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                             );
        }
    }
    else
    {
        //Slave shall NOT send LL_00_CONNECTION_UPDATE_IND
        if (1) {
          //
                done = send_04_hciEvent_3e_06_LE_remote_conn_param_request(pacl);
            if (done) {
                //Slave Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure
                //      tx request event to host, wait reply
                LLC_RESPOND_PROCEDURE_start(pacl, LL_0F_CONNECTION_PARAM_REQ | FLAG_INVOLVE_INSTANT); //Conn Params Request procedure, Slave Rx LL_0F_CONNECTION_PARAM_REQ
            }
          /*
            //if no need request to and reply by host
            //eg only move anchor point
                done = TxLLcPDU_10_connection_param_rsp(pacl);
            if (done) {
                //Slave Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure
                //      Tx LL_10_CONNECTION_PARAM_RSP  complete RespondProcedure, no need LLC_RESPOND_PROCEDURE_start() and _end_complete()
            }
          */
        }
        else {
            done = TxLLcPDU_11_reject_ext_ind(pacl, LL_0F_CONNECTION_PARAM_REQ,   //rejectOpcode
                                                    0x3B                          //errorCode   ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                             );
        }
    }
    return (done);
}

bool RxLLcPDU_10_connection_param_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_10_connection_param_rsp_TypeDef *pRsp10 = (llcpdu_10_connection_param_rsp_TypeDef *)pPayload;
    done = 1;
    if (LEACL_is_role_master(pacl))
    {
        //Master
        if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_0F_CONNECTION_PARAM_REQ)) { //Conn Params Request procedure
            if (pacl->pTemplateConnParam != 0) {
                pacl->pTemplateConnParam->minInterval = pRsp10->minInterval;
                pacl->pTemplateConnParam->maxInterval = pRsp10->maxInterval;
                pacl->pTemplateConnParam->latency     = pRsp10->latency;
                pacl->pTemplateConnParam->timeout     = pRsp10->timeout;
            }
              //pRsp10->preferredPeriodicity
              //pRsp10->referenceConnEventCount
              //pRsp10->offset0
              //pRsp10->offset1
              //pRsp10->offset2
              //pRsp10->offset3
              //pRsp10->offset4
              //pRsp10->offset5

                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Tx LL_0F_CONNECTION_PARAM_REQ     start InitiateProcedure
                    //       Rx LL_10_CONNECTION_PARAM_RSP
                    //       Tx LL_00_CONNECTION_UPDATE_IND          InitiateProcedure transition to Conn Update procedure
                    LLC_INITIATE_PROCEDURE_transition_to_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND);       //Conn Update procedure
                }
        }
    }
    else
    {
        //Slave
        //      invalid for Slave IUT to receive LL_10_CONNECTION_PARAM_RSP
            done = TxLLcPDU_07_unknown_rsp(pacl, LL_10_CONNECTION_PARAM_RSP); //unknownType
            return (done);
    }
    return (done);
}

bool RxLLcPDU_11_reject_ext_ind (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    uint8_t rejectOpcode;
    uint8_t errorCode;
    rejectOpcode = ht_read_byte( pPayload->ctrData+0 ); //uint8_t rejectOpcode; //ctrData[0]
    errorCode    = ht_read_byte( pPayload->ctrData+1 ); //uint8_t errorCode   ; //ctrData[1]
    done = 1;
    if (LLC_INITIATE_PROCEDURE_is_active(pacl))
    {
    switch (rejectOpcode)
    {
    case 0x03://LL_03_ENC_REQ:              //Encryption Start procedure
        if (LEACL_is_role_master(pacl))
        {
          //if (errorCode == 0x1A)     //ERR_1A_UNSUPPORTED_REMOTE_FEATURE, remote does not support the feature associated with the issued command, LMP or LL Control PDU
          //if (errorCode == 0x06)     //ERR_06_PIN_OR_KEY_MISSING
            {
                //LL/SEC/MAS/BV-03-C [Master Receiving LL_REJECT_IND]      ERR_1A_UNSUPPORTED_REMOTE_FEATURE
                //LL/SEC/MAS/BV-11-C [Master Receiving LL_REJECT_EXT_IND]  ERR_06_PIN_OR_KEY_MISSING
                    done = send_04_hciEvent_08_encryption_change(LEACL_conn_handle(pacl), 
                                                                 errorCode,
                                                                 0x00  //Encryption_Enabled, 0x01 Link Level Encryption is ON with AES-CCM for LE
                                                                );
            }
                if (done) {
                    //Master Tx LL_03_ENC_REQ     start InitiateProcedure
                    //       Rx LL_0D_REJECT_IND or LL_11_REJECT_EXT_IND
                    //Encryption start procedure is complete in Master when Master receives the LL_REJECT_IND or LL_REJECT_EXT_IND from Slave
                    LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_0D_REJECT_IND
                }
        }
        break;
    case 0x0F://LL_0F_CONNECTION_PARAM_REQ:
        if (LEACL_is_role_master(pacl)
            && (    (errorCode == 0x1A)     //ERR_1A_UNSUPPORTED_REMOTE_FEATURE, remote does not support the feature associated with the issued command, LMP or LL Control PDU
                                            //                                   master skip param asking and then directly send LL_00_CONNECTION_UPDATE_IND
                                            //LL/CON/MAS/BV-29-C [Initiating Conn Param Request - remote legacy host]
              // || (errorCode == 0x3B)     //ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                            //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#2 Master can then send LL_00_CONNECTION_UPDATE_IND
               )
           )
        {
                //LL/CON/MAS/BV-29-C [Initiating Conn Param Request - remote legacy host]
                //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#2 Master can then send LL_00_CONNECTION_UPDATE_IND

                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Tx LL_0F_CONNECTION_PARAM_REQ     start InitiateProcedure
                    //       Rx LL_11_REJECT_EXT_IND
                    //       Tx LL_00_CONNECTION_UPDATE_IND          InitiateProcedure transition to Conn Update procedure
                    LLC_INITIATE_PROCEDURE_transition_to_llcOP(pacl, LL_00_CONNECTION_UPDATE_IND);       //Conn Update procedure
                }
        }
        else
        {
                //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject] ALT#1
                //LL/CON/SLA/BV-25-C [Initiating Conn Param Request - Reject]
                if (LLC_INITIATE_PROCEDURE_is_initiated_by_host(pacl)) {
                    done = send_04_hciEvent_3e_03_LE_conn_update_complete( pacl, errorCode );
                }
                else {
                    done = 1;
                }
                if (done) {
                    LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_11_REJECT_EXT_IND
                }
        }
        break;
    default:
            done = 1;
        break;
    }//end switch
    }//end LLC_INITIATE_PROCEDURE_is_active
    return (done);
}

bool RxLLcPDU_12_ping_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    //responding procedure
    bool done;
    if ((0 == LOCAL_FEATURE_is_supported_LE_ping())
       )
    {
        return TxLLcPDU_07_unknown_rsp(pacl, LL_12_PING_REQ); //unknownType
    }
    //    rxdlen no need to check, because LL_12_PING_REQ PDU does not have a CtrData field
    //if (rxdlen < sizeof(llcpdu_12_ping_req_TypeDef)) {
    //if (rxdlen > sizeof(llcpdu_12_ping_req_TypeDef)) {  adopt the PDU with larger rxdlen
    //respond
            done = TxLLcPDU_13_ping_rsp(pacl);
    return (done);
}

bool RxLLcPDU_13_ping_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_12_PING_REQ)) //LE Ping procedure
    {
    //llcInitiateProcedure is complete
        done = 1;
        ////////
        if (done) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_13_PING_RSP
        }
        return (done);
    }
    else
    {
        //ERROR: llcInitiateProcedure is not LE Ping procedure,
        //       but, roger LL_13_PING_RSP
        done = 1;
        return (done);
    }
}

bool RxLLcPDU_14_length_req (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    //responding procedure
    bool done;
    if ((0 == LOCAL_FEATURE_is_supported_LE_data_packet_length_extension()) ||
        (rxdlen < sizeof(llcpdu_14_length_req_TypeDef))
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            //step 6 option (a) The Lower Tester receives an LL_07_UNKNOWN_RSP PDU from the IUT
       )
    {
        return TxLLcPDU_07_unknown_rsp(pacl, LL_14_LENGTH_REQ); //unknownType
    }
  //else
  //if (rxdlen > sizeof(llcpdu_14_length_req_TypeDef)) {
        //step 6 option (c) The Lower Tester receives a PDU from the IUT that would be a response to the opcode sent in step 5 with VALID parameters
        //continue, adopt this PDU
  //}
    //save
            pacl->connRemoteMaxRxOctets = ht_read_hword( pPayload->ctrData+0 ); //uint16_t maxRxOctets; //ctrData[0~1]  sender's connMaxRxOctets
            pacl->connRemoteMaxRxTime   = ht_read_hword( pPayload->ctrData+2 ); //uint16_t maxRxTime  ; //ctrData[2~3]  sender's connMaxRxTime
            pacl->connRemoteMaxTxOctets = ht_read_hword( pPayload->ctrData+4 ); //uint16_t maxTxOctets; //ctrData[4~5]  sender's connMaxTxOctets
            pacl->connRemoteMaxTxTime   = ht_read_hword( pPayload->ctrData+6 ); //uint16_t maxTxTime  ; //ctrData[6~7]  sender's connMaxTxTime
    //respond
        done = TxLLcPDU_15_length_rsp(pacl);
    return (done);
}

bool RxLLcPDU_15_length_rsp (uint8_t rxdlen, llcpdu_PAYLOAD_TypeDef *pPayload, LEACL_TypeDef *pacl)
{
    bool done;
    if (LLC_INITIATE_PROCEDURE_is_llcOP(pacl, LL_14_LENGTH_REQ)) //Data Length Update procedure
    {
    //llcInitiateProcedure is complete
        if (rxdlen < sizeof(llcpdu_15_length_rsp_TypeDef)) {
            //LL/PAC/SLA/BI-01-C [Control PDUs with Invalid Length]
            //LL/PAC/MAS/BI-01-C [Control PDUs with Invalid Length]
            done = 1;
        }
        else {
        //save
            pacl->connRemoteMaxRxOctets = ht_read_hword( pPayload->ctrData+0 ); //uint16_t maxRxOctets; //ctrData[0~1]  sender's connMaxRxOctets
            pacl->connRemoteMaxRxTime   = ht_read_hword( pPayload->ctrData+2 ); //uint16_t maxRxTime  ; //ctrData[2~3]  sender's connMaxRxTime
            pacl->connRemoteMaxTxOctets = ht_read_hword( pPayload->ctrData+4 ); //uint16_t maxTxOctets; //ctrData[4~5]  sender's connMaxTxOctets
            pacl->connRemoteMaxTxTime   = ht_read_hword( pPayload->ctrData+6 ); //uint16_t maxTxTime  ; //ctrData[6~7]  sender's connMaxTxTime
        //effective
                    //LL/CON/MAS/BV-74-C [Master Data Length Update - Initiating Data Length Update Procedure; LE 1M PHY]
                    //LL/CON/SLA/BV-78-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE 1M PHY]
                    //LL/CON/MAS/BV-77-C [Master Data Length Update - Initiating Data Length Update Procedure; LE 2M PHY]
                    //LL/CON/SLA/BV-81-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE 2M PHY]
                    //LL/CON/MAS/BV-79-C [Master Data Length Update - Initiating Data Length Update Procedure; LE Coded PHY]
                    //LL/CON/SLA/BV-83-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE Coded PHY]
            done = hcmsgQ_push( HCMSG_CHECK_CONN_EFFECTIVE_MAX_TRX_OCTETS_TIME_CHANGE, 
                                LEACL_conn_handle(pacl), 
                                0,//length
                                (uint8_t *)0
                              );
        }
        ////////
        if (done) {
            LLC_INITIATE_PROCEDURE_end_complete(pacl); //complete InitiateProcedure, clear state upon roger LL_15_LENGTH_RSP
        }
        return (done);
    }
    else
    {
        //ERROR: llcInitiateProcedure is not Data Length Update procedure,
        //       but, roger LL_15_LENGTH_RSP
            done = 1;
        return (done);
    }
}

bool proc_hcmsg_check_conn_effective_max_trx_octets_time_change(hcmsgQ_node_TypeDef *qNode)
{
    bool done;
    uint16_t effectiveMaxRxOctets;  // the lesser of connMaxRxOctets and connRemoteMaxTxOctets
    uint16_t effectiveMaxTxOctets;  // the lesser of connMaxTxOctets and connRemoteMaxRxOctets
    uint16_t effectiveMaxRxTime;    //
    uint16_t effectiveMaxTxTime;    //
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    {
        //effective
                effectiveMaxRxOctets = lesser_u16( pacl->connMaxRxOctets, pacl->connRemoteMaxTxOctets );
                effectiveMaxTxOctets = lesser_u16( pacl->connMaxTxOctets, pacl->connRemoteMaxRxOctets );
                effectiveMaxRxTime   = 0x0148;//tmp debug
                effectiveMaxTxTime   = 0x0148;//tmp debug
        //Notify Host if any of connEffectiveMaxTxOctets, connEffectiveMaxRxOctets, connEffectiveMaxTxTime, or connEffectiveMaxRxTime have changed
        if (    effectiveMaxRxOctets != pacl->connEffectiveMaxRxOctets
            ||  effectiveMaxTxOctets != pacl->connEffectiveMaxTxOctets
            ||  effectiveMaxRxTime   != pacl->connEffectiveMaxRxTime
            ||  effectiveMaxTxTime   != pacl->connEffectiveMaxTxTime
           )
        {
                done = send_04_hciEvent_3e_07_LE_data_length_change(pacl,
                                                                    effectiveMaxTxOctets,
                                                                    effectiveMaxTxTime,
                                                                    effectiveMaxRxOctets,
                                                                    effectiveMaxRxTime
                                                                   );
            if (done) {
                pacl->connEffectiveMaxRxOctets = effectiveMaxRxOctets;
                pacl->connEffectiveMaxTxOctets = effectiveMaxTxOctets;
                pacl->connEffectiveMaxRxTime   = effectiveMaxRxTime;
                pacl->connEffectiveMaxTxTime   = effectiveMaxTxTime;
            }
        }
        else
        {
                done = 1;
        }
        return (done);
    }
}

/*  delta t = (   connIntervalNEW
 *              - ((Instant - ReferenceConnEventCount
 *                 ) * connIntervalOLD
 *                )  % connIntervalNEW
 *              + offset0
 *            ) % connIntervalNEW
 */
uint16_t calc_deltat(LEACL_TypeDef *pacl, uint16_t instant, uint16_t referenceConnEvtCnt)
{
    int32_t  deltat;
    int32_t  instant_minus_reference; //instant - ReferenceConnEventCount
    if ( ((instant & 0xC000) == 0xC000) && ((referenceConnEvtCnt & 0xC000) == 0x0000) ) {
        instant_minus_reference = instant - referenceConnEvtCnt
                                          - 0x00010000;
    }
    else
    if ( ((instant & 0xC000) == 0x0000) && ((referenceConnEvtCnt & 0xC000) == 0xC000) ) {
        instant_minus_reference = instant + 0x00010000
                                          - referenceConnEvtCnt;
    }
    else {
        instant_minus_reference = instant - referenceConnEvtCnt;
    }

        deltat = (   pacl->pTemplateConnParam->interval                         //  connIntervalNEW            unit 1.25 ms
                   - ( (   instant_minus_reference                              //  instant - ReferenceConnEventCount
                       ) * pacl->currParam.interval                             //x connIntervalOLD            unit 1.25 ms
                     )   % pacl->pTemplateConnParam->interval                   //% connIntervalNEW            unit 1.25 ms
                   + pacl->pTemplateConnParam->offset0                          //+ offset                     unit 1.25 ms
                 ) % pacl->pTemplateConnParam->interval;                        //% connIntervalNEW            unit 1.25 ms
    return ((uint16_t)deltat);
};

bool TxLLcPDU_00_connection_update_ind(LEACL_TypeDef *pacl)
{
    bool done;
    uint16_t deltaT00;
    uint16_t instant00;
    uint16_t winOffset00;
    LLCONNECTIONUPDATE_TypeDef *pUpdat;
    llcpdu_00_connection_update_ind_TypeDef ind00 =
    {
        .opcode   = 0x00, //LL_00_CONNECTION_UPDATE_IND,
        .winSize  = 16,                     //pCtrData[0]
      //.winOffset= 1,                      //pCtrData[1~2]
      //.interval = 12+200,//800,           //pCtrData[3~4]
      //.latency  = 0,                      //pCtrData[5~6]
      //.timeout  = 0x0C80,                 //pCtrData[7~8]
      //.instant  = pacl->connEventCount+6  //pCtrData[9~10]
      //Master should allow a minimum of 6 CEs that Slave will be listening for before the instant occurs
    };
        instant00 = pacl->connEventCount+6;
        pacl->pTemplateConnParam->interval = pacl->pTemplateConnParam->maxInterval;
    if (pacl->pTemplateConnParam != 0) {
        ht_write_hword( (uint8_t *)&(ind00.interval), pacl->pTemplateConnParam->interval    );  //pCtrData[3~4]
        ht_write_hword( (uint8_t *)&(ind00.latency ), pacl->pTemplateConnParam->latency     );  //pCtrData[5~6]
        ht_write_hword( (uint8_t *)&(ind00.timeout ), pacl->pTemplateConnParam->timeout     );  //pCtrData[7~8]
    }
        ht_write_hword( (uint8_t *)&(ind00.instant ), instant00                             );  //pCtrData[9~10]

//pacl->pTemplateConnParam->referenceConnEventCount = pacl->connEventCount+9;
//pacl->pTemplateConnParam->referenceConnEventCount = 0x3FFF;//debug the ReferenceConnEventCount and Instant are on different sides of the eventCount wraparound
  pacl->pTemplateConnParam->referenceConnEventCount = 0xC000;//debug the ReferenceConnEventCount and Instant are on different sides of the eventCount wraparound
  pacl->pTemplateConnParam->offset0                 = 0x0014;
//debug_RFLA_pulse_to_trigger_LA();
        deltaT00 = calc_deltat(pacl, instant00,
                                     pacl->pTemplateConnParam->referenceConnEventCount
                              );
//debug_RFLA_pulse_to_trigger_LA();
        winOffset00 = deltaT00 - 1; //1:pUpdat->iMaster_deltaT_minus_winOffset
        ht_write_hword( (uint8_t *)&(ind00.winOffset), winOffset00                          );  //pCtrData[1~2]

    done = PoTxllcPDU(sizeof(llcpdu_00_connection_update_ind_TypeDef), //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind00),
                      pacl
                     );
    //--------------
    if (done)
    {
    if (pacl->pNewUpdate == 0)
        #if    __MALLOC_METHOD__  == 1
        pUpdat = (LLCONNECTIONUPDATE_TypeDef *)      malloc(sizeof(LLCONNECTIONUPDATE_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pUpdat = (LLCONNECTIONUPDATE_TypeDef *)pvPortMalloc(sizeof(LLCONNECTIONUPDATE_TypeDef));
        #else
        ...
        #endif
    else
        pUpdat = pacl->pNewUpdate ;
    if (pUpdat != 0)
    {
    //first set value
    //final assign the pointer pNewUpdate = pUpdat
        pUpdat->winSize   = ind00.winSize;
        pUpdat->winOffset = ind00.winOffset;
        pUpdat->interval  = ind00.interval;
        pUpdat->latency   = ind00.latency;
        pUpdat->timeout   = ind00.timeout;
        pUpdat->instant   = ind00.instant;
        
        pUpdat->iMaster_deltaT_minus_winOffset = 1;
      //         Master not use iSlave_updateState and iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger
      //pUpdat->iSlave_updateState = 0x00;
      //pUpdat->iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger = 0;

    //final assign the pointer pNewUpdate = pUpdat
        pacl->pNewUpdate = pUpdat; //send LL_00_CONNECTION_UPDATE_IND
    };//pUpdat
    };//done
    //--------------
    return (done);
}

bool proc_llcmsg_initiate_llc_conn_update_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_00_CONNECTION_UPDATE_IND
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    if (LEACL_is_role_slave(pacl))
    {
        //Slave cannot initiate send LL_00_CONNECTION_UPDATE_IND
        //coding error !!!
        return TxLLcPDU_02_terminate_ind(pacl, 0x03); //ERR_03_HARDWARE_FAILURE
    }
    else
  //if (LEACL_is_role_master(pacl))
    {
        //Master can update
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
            done = TxLLcPDU_00_connection_update_ind(pacl);
        //////////////
        if (done) {
                //Master Tx LL_00_CONNECTION_UPDATE_IND    start InitiateProcedure
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT | FLAG_INITIATED_BY_HOST); //Conn Update procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT);                          //Conn Update procedure
            }
        }
        return (done);
    }
}

bool proc_llcmsg_initiate_llc_conn_params_request_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_0F_CONNECTION_PARAM_REQ
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
            done = TxLLcPDU_0f_connection_param_req(pacl);
          //done = 1;
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0F_CONNECTION_PARAM_REQ | FLAG_INVOLVE_INSTANT | FLAG_INITIATED_BY_HOST); //Conn Params Request procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0F_CONNECTION_PARAM_REQ | FLAG_INVOLVE_INSTANT);                          //Conn Params Request procedure
            }
        };//done
        return (done);
    }
}

bool proc_llcmsg_initiate_llc_channel_map_update_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_01_CHANNEL_MAP_IND
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    if (LEACL_is_role_slave(pacl))
    {
        //Slave cannot initiate send LL_01_CHANNEL_MAP_IND
        //coding error !!!
        return TxLLcPDU_02_terminate_ind(pacl, 0x03); //ERR_03_HARDWARE_FAILURE
      //return 1;
    }
    else
  //if (LEACL_is_role_master(pacl))
    {
        //Master can update the channel map by sending an LL_01_CHANNEL_MAP_IND
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
            done = TxLLcPDU_01_channel_map_ind(pacl);
          //done = 1;
        //////////////
        if (done) {
            //Master Tx LL_01_CHANNEL_MAP_IND    start InitiateProcedure
            LLC_INITIATE_PROCEDURE_start(pacl, LL_01_CHANNEL_MAP_IND | FLAG_INVOLVE_INSTANT); //Channel Map Update procedure
        }
        return (done);
    }
}

bool TxLLcPDU_01_channel_map_ind(LEACL_TypeDef *pacl)
{
    bool done;
    LLCHANNELMAP_TypeDef *pMap;
    llcpdu_01_channel_map_ind_TypeDef ind01 =
    {
        .opcode = 0x01, //LL_01_CHANNEL_MAP_IND
        .chM[0] = leconfig_hostChannelClass.channel_map[0],     //pCtrData[0]
        .chM[1] = leconfig_hostChannelClass.channel_map[1],     //pCtrData[1]
        .chM[2] = leconfig_hostChannelClass.channel_map[2],     //pCtrData[2]
        .chM[3] = leconfig_hostChannelClass.channel_map[3],     //pCtrData[3]
        .chM[4] = leconfig_hostChannelClass.channel_map[4],     //pCtrData[4]
        .instant= pacl->connEventCount+5                        //pCtrData[5~6]
    };
    done = PoTxllcPDU(sizeof(llcpdu_01_channel_map_ind_TypeDef), //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind01),
                      pacl
                     );
    //--------------
    if (done)
    {
    if (pacl->pNewChM == 0)
        #if    __MALLOC_METHOD__  == 1
        pMap = (LLCHANNELMAP_TypeDef *)      malloc(sizeof(LLCHANNELMAP_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pMap = (LLCHANNELMAP_TypeDef *)pvPortMalloc(sizeof(LLCHANNELMAP_TypeDef));
        #else
        ...
        #endif
    else
        pMap = pacl->pNewChM ;
    if (pMap != 0)
    {
        pMap->chM[0]  = ind01.chM[0];
        pMap->chM[1]  = ind01.chM[1];
        pMap->chM[2]  = ind01.chM[2];
        pMap->chM[3]  = ind01.chM[3];
        pMap->chM[4]  = ind01.chM[4];
        pMap->instant = ind01.instant;
        
        pacl->pNewChM = pMap;
        
        calc_usedChannel_byRemapIndex(pacl->pNewChM);
    };//pMap
    };//done
    //--------------
    return (done);
}

bool proc_llcmsg_initiate_llc_acl_termination_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_02_TERMINATE_IND
{
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  reason;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
    reason = ht_read_byte (qNode->msgparam+2); //param[2]     reason
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1;
    }
    else
    {
        //Termination procedure may be initiated at any time, even if any other LL control procedure is currently active
        return TxLLcPDU_02_terminate_ind(pacl, reason); //ERR_13_OTHER_END_TERMINATED_CONN_USER_END
    }
}

bool TxLLcPDU_02_terminate_ind(LEACL_TypeDef *pacl, uint8_t reason)
{
    bool done;
    llcpdu_02_terminate_ind_TypeDef ind02 =
    {
        .opcode    = 0x02, //LL_02_TERMINATE_IND,
        .errorCode = reason
    };
    done = PoTxllcPDU_high(sizeof(llcpdu_02_terminate_ind_TypeDef), //1+1, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind02),
                      pacl
                     );
    return (done);
}

bool proc_llcmsg_initiate_llc_encryption_start_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_03_ENC_REQ, wait LL_04_ENC_RSP
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1] conn handle
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return (1);
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
        if (LEACL_is_role_master(pacl)) {
                done = TxLLcPDU_03_enc_req(pacl, 0);//0:high_priority==0
        }
        else { //Slave
            return (1); //todo: slave not accept hcmd_2019_le_enable_encryption(), error code Command Disallowed (0x0C)
        }
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_03_ENC_REQ | FLAG_INVOLVE_INSTANT | FLAG_INITIATED_BY_HOST); //Encryption Start procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_03_ENC_REQ | FLAG_INVOLVE_INSTANT);                          //Encryption Start procedure
            }
        }
        return (done);
    }
}

bool TxLLcPDU_03_enc_req (LEACL_TypeDef *pacl, bool high_priority)
{
    bool done;
    llcpdu_03_enc_req_TypeDef req03 =
    {
        .opcode       = 0x03 //LL_03_ENC_REQ
    //  .rand[8];                //ctrData[0~7]
    //  .ediv[2];                //ctrData[8~9]
    //  .SKDm[8];                //ctrData[10~17]
    //  .IVm[4];                 //ctrData[18~21]
    };
    #if 0 //#if AESCCM_LL_SIGNALING == 1
    {
            ht_memory_copy( pacl->encrypt.SKDm, vol6_partc_1_SKDm,  8); //debug test
            ht_memory_copy( pacl->encrypt.IVm,  vol6_partc_1_2_IVm, 4); //debug test
    }
    #else
    {
        uint8_t i;
        for(i=0; i<8; i++) {
            pacl->encrypt.SKDm[i] = rand()+0x33;
        }
        for(i=0; i<4; i++) {
            pacl->encrypt.IVm[i]  = rand()+0x33;
        }
    }
    #endif
            ht_memory_copy( req03.rand, pacl->encrypt.rand, 8 );    // Rand and EDIV are provided by Host
            ht_memory_copy( req03.ediv, pacl->encrypt.ediv, 2 );    // Rand and EDIV are provided by Host
            ht_memory_copy( req03.SKDm, pacl->encrypt.SKDm, 8 );
            ht_memory_copy( req03.IVm,  pacl->encrypt.IVm,  4 );
    if (high_priority)
    done = PoTxllcPDU_high(sizeof(llcpdu_03_enc_req_TypeDef), //1+22, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req03),
                      pacl
                     );
    else
    done = PoTxllcPDU(sizeof(llcpdu_03_enc_req_TypeDef), //1+22, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req03),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_04_enc_rsp (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_04_enc_rsp_TypeDef rsp04 =
    {
        .opcode       = 0x04 //LL_04_ENC_RSP
    //  .SKDs[8];                //ctrData[0~7]
    //  .IVs[4];                 //ctrData[8~11]
    };
    #if 0 //#if AESCCM_LL_SIGNALING == 1
    {
            ht_memory_copy( pacl->encrypt.SKDs, vol6_partc_1_SKDs,  8); //debug test
            ht_memory_copy( pacl->encrypt.IVs,  vol6_partc_1_2_IVs, 4); //debug test
    }
    #else
    {
        uint8_t i;
        for(i=0; i<8; i++) {
            pacl->encrypt.SKDs[i] = rand()+0x55;
        }
        for(i=0; i<4; i++) {
            pacl->encrypt.IVs[i]  = rand()+0x55;
        }
    }
    #endif
            ht_memory_copy( rsp04.SKDs, pacl->encrypt.SKDs, 8 );
            ht_memory_copy( rsp04.IVs,  pacl->encrypt.IVs,  4 );
    done = PoTxllcPDU_high(sizeof(llcpdu_04_enc_rsp_TypeDef), //1+22, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp04),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_05_start_enc_req (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_05_start_enc_req_TypeDef req05 =
    {
        .opcode       = 0x05 //LL_05_START_ENC_REQ
    };
    done = PoTxllcPDU_high(sizeof(llcpdu_05_start_enc_req_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req05),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_06_start_enc_rsp (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_06_start_enc_rsp_TypeDef rsp06 =
    {
        .opcode       = 0x06 //LL_06_START_ENC_RSP
    };
    done = PoTxllcPDU_high(sizeof(llcpdu_06_start_enc_rsp_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp06),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_07_unknown_rsp(LEACL_TypeDef *pacl, uint8_t unknownType)
{
    bool done;
    llcpdu_07_unknown_rsp_TypeDef rsp07 =
    {
        .opcode      = 0x07, //LL_07_UNKNOWN_RSP,
        .unknownType = unknownType
    };
    done = PoTxllcPDU(sizeof(llcpdu_07_unknown_rsp_TypeDef), //1+1, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp07),
                      pacl
                     );
uart_puts("llcTx unknown_rsp "); uart_putu8(unknownType); uart_putchar_n('\n');
 //printf("llcTx unknown_rsp %02X\n", unknownType);
    return (done);
}

bool proc_llcmsg_initiate_llc_feature_exchange_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1] conn handle
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return (1);
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
        if (LEACL_is_role_master(pacl)) {
                done = TxLLcPDU_08_feature_req(pacl);
        }
        else { //Slave
            if (LOCAL_FEATURE_is_supported_slave_initiated_features_exchange()) {
                done = TxLLcPDU_0E_slave_feature_req(pacl);
            }
            else {
                return (1); //disabled local
            }
        }
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_08_FEATURE_REQ | FLAG_INITIATED_BY_HOST); //Feature Exchange procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_08_FEATURE_REQ);                          //Feature Exchange procedure
            }
        }
        return (done);
    }
}

bool TxLLcPDU_08_feature_req(LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_08_feature_req_TypeDef req08 =
    {
        .opcode       = 0x08, //LL_08_FEATURE_REQ,
        .featureSet[0]= LOCAL_FEATURE_get_featureSet(0),//pCtrData[0]
        .featureSet[1]= LOCAL_FEATURE_get_featureSet(1),//pCtrData[1]
        .featureSet[2]= LOCAL_FEATURE_get_featureSet(2),//pCtrData[2]
        .featureSet[3]= LOCAL_FEATURE_get_featureSet(3),//pCtrData[3]
        .featureSet[4]= LOCAL_FEATURE_get_featureSet(4),//pCtrData[4]
        .featureSet[5]= LOCAL_FEATURE_get_featureSet(5),//pCtrData[5]
        .featureSet[6]= LOCAL_FEATURE_get_featureSet(6),//pCtrData[6]
        .featureSet[7]= LOCAL_FEATURE_get_featureSet(7) //pCtrData[7]
    };
    done = PoTxllcPDU(sizeof(llcpdu_08_feature_req_TypeDef), //1+8, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req08),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_09_feature_rsp(LEACL_TypeDef *pacl)
{
    //responding procedure
    bool done;
    llcpdu_09_feature_rsp_TypeDef rsp09 =
    {
        .opcode       = 0x09, //LL_09_FEATURE_RSP,
        .featureSet[0]= LOCAL_FEATURE_get_featureSet(0) & REMOTE_FEATURE_get_featureSet(pacl,0),//pCtrData[0]
   //RSP FeatureSet[0]   shall contain a set of features supported by the LL of both the Master and Slave
   //RSP FeatureSet[1-7] shall contain a set of features supported by the LL that transmits this PDU
        .featureSet[1]= LOCAL_FEATURE_get_featureSet(1),//pCtrData[1]
        .featureSet[2]= LOCAL_FEATURE_get_featureSet(2),//pCtrData[2]
        .featureSet[3]= LOCAL_FEATURE_get_featureSet(3),//pCtrData[3]
        .featureSet[4]= LOCAL_FEATURE_get_featureSet(4),//pCtrData[4]
        .featureSet[5]= LOCAL_FEATURE_get_featureSet(5),//pCtrData[5]
        .featureSet[6]= LOCAL_FEATURE_get_featureSet(6),//pCtrData[6]
        .featureSet[7]= LOCAL_FEATURE_get_featureSet(7) //pCtrData[7]
    };
    if (pacl->pRemoteFeature) {
   //RSP   FeatureSet[0]   shall contain a set of features supported by the LL of both the Master and Slave
     rsp09.featureSet[0]= LOCAL_FEATURE_get_featureSet(0) & ((pacl->pRemoteFeature)->featureSet[0]);//pCtrData[0]
    }
    done = PoTxllcPDU(sizeof(llcpdu_09_feature_rsp_TypeDef), //1+8, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp09),
                      pacl
                     );
    return (done);
}

bool proc_llcmsg_initiate_llc_encryption_pause_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_0A_PAUSE_ENC_REQ
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1] conn handle
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return (1);
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
        if (LEACL_is_role_master(pacl)) {
                done = TxLLcPDU_0a_pause_enc_req(pacl);
        }
        else { //Slave
            return (1); //todo: slave not accept hcmd_2019_le_enable_encryption(), error code Command Disallowed (0x0C)
        }
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0A_PAUSE_ENC_REQ | FLAG_INVOLVE_INSTANT | FLAG_INITIATED_BY_HOST); //Encryption Pause procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0A_PAUSE_ENC_REQ | FLAG_INVOLVE_INSTANT);                          //Encryption Pause procedure
            }
        }
        return (done);
    }
}

bool TxLLcPDU_0a_pause_enc_req (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0a_pause_enc_req_TypeDef req0a =
    {
        .opcode       = 0x0A //LL_0A_PAUSE_ENC_REQ
    };
    //     PoTxllcPDU_high() no need for TxLLcPDU_0a_pause_enc_req()
    done = PoTxllcPDU(sizeof(llcpdu_0a_pause_enc_req_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req0a),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_0b_pause_enc_rsp (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0b_pause_enc_rsp_TypeDef rsp0b =
    {
        .opcode       = 0x0B //LL_0B_PAUSE_ENC_RSP
    };
    done = PoTxllcPDU_high(sizeof(llcpdu_0b_pause_enc_rsp_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp0b),
                      pacl
                     );
    return (done);
}

bool proc_llcmsg_initiate_llc_version_exchange_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_0C_VERSION_IND, wait LL_0C_VERSION_IND
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1] conn handle
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1;
    }
    //////////////
    if (pacl->flag0.field.alreadySentLLVersionInd == 0)
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
            done = TxLLcPDU_0c_version_ind(pacl);
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0C_VERSION_IND | FLAG_INITIATED_BY_HOST); //Version Exchange procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_0C_VERSION_IND);                          //Version Exchange procedure
            }
        }
        return (done);
    }
    else
    {
        if (byHost)
            return send_04_hciEvent_0c_read_remote_version_info_complete(pacl, 0x00); //ERR_00_SUCCESS
        else
            return 1;
    }
}

bool TxLLcPDU_0c_version_ind(LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0c_version_ind_TypeDef ind0c =
    {
        .opcode       = 0x0C, //LL_0C_VERSION_IND,
        .llVersNr     = leconfig_versionInfo.versNr,    //pCtrData[0] Link Layer Version
                                                        //              0x06: Bluetooth spec 4.0
                                                        //              0x07: Bluetooth spec 4.1
                                                        //              0x08: Bluetooth spec 4.2
                                                        //              0x09: Bluetooth spec 5.0
                                                        //              0x0A: Bluetooth spec 5.1
                                                        //              0x0B: Bluetooth spec 5.2
                                                        //              0x0C: Bluetooth spec 5.3
        .compId       = leconfig_versionInfo.compId,    //pCtrData[1~2] Company identifiers
                                                        //              0x0046    MediaTek, Inc
        .llSubVersNr  = leconfig_versionInfo.subVersNr, //pCtrData[3~4]
    };
    done = PoTxllcPDU(sizeof(llcpdu_0c_version_ind_TypeDef), //1+5, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind0c),
                      pacl
                     );
    if (done) {
        pacl->flag0.field.alreadySentLLVersionInd = 1;
    }
    return (done);
}

bool TxLLcPDU_0d_reject_ind(LEACL_TypeDef *pacl, uint8_t reason)
{
    bool done;
    llcpdu_0d_reject_ind_TypeDef ind0d =
    {
        .opcode    = 0x0D, //LL_0D_REJECT_IND,
      //.errorCode = reason  //ctrData[0]
    };
        ht_write_byte( (uint8_t *)&(ind0d.errorCode), reason );  //ctrData[0]
    done = PoTxllcPDU_high(sizeof(llcpdu_0d_reject_ind_TypeDef), //1+1, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind0d),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_0E_slave_feature_req(LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0e_slave_feature_req_TypeDef req0e =
    {
        .opcode       = 0x0E, //LL_0E_SLAVE_FEATURE_REQ,
        .featureSet[0]= LOCAL_FEATURE_get_featureSet(0),//pCtrData[0]
        .featureSet[1]= LOCAL_FEATURE_get_featureSet(1),//pCtrData[1]
        .featureSet[2]= LOCAL_FEATURE_get_featureSet(2),//pCtrData[2]
        .featureSet[3]= LOCAL_FEATURE_get_featureSet(3),//pCtrData[3]
        .featureSet[4]= LOCAL_FEATURE_get_featureSet(4),//pCtrData[4]
        .featureSet[5]= LOCAL_FEATURE_get_featureSet(5),//pCtrData[5]
        .featureSet[6]= LOCAL_FEATURE_get_featureSet(6),//pCtrData[6]
        .featureSet[7]= LOCAL_FEATURE_get_featureSet(7) //pCtrData[7]
    };
    done = PoTxllcPDU(sizeof(llcpdu_0e_slave_feature_req_TypeDef), //1+8, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req0e),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_0f_connection_param_req (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_0f_connection_param_req_TypeDef req0f =
    {
        .opcode       = 0x0F, //LL_0F_CONNECTION_PARAM_REQ
    //  .minInterval  = 12+200,  //ctrData[0~1]
    //  .maxInterval  = 12+200,  //ctrData[2~3]
    //  .latency      = 0x0000,  //ctrData[4~5]
    //  .timeout      = 0x0C80,  //ctrData[6~7]
    //  .preferredPeriodicity;   //ctrData[8]     value   0x00 means no preference
    //  .referenceConnEventCount;//ctrData[9~10]
    //  .offset0      = 0xFFFF,  //ctrData[11~12] value 0xFFFF means invalid
    //  .offset1      = 0xFFFF,  //ctrData[13~14] value 0xFFFF means invalid
    //  .offset2      = 0xFFFF,  //ctrData[15~16] value 0xFFFF means invalid
    //  .offset3      = 0xFFFF,  //ctrData[17~18] value 0xFFFF means invalid
    //  .offset4      = 0xFFFF,  //ctrData[19~20] value 0xFFFF means invalid
    //  .offset5      = 0xFFFF,  //ctrData[21~22] value 0xFFFF means invalid
    };
    if (pacl->pTemplateConnParam != 0) {
        pacl->pTemplateConnParam->minInterval = pacl->pTemplateConnParam->host.interval_min;
        pacl->pTemplateConnParam->maxInterval = pacl->pTemplateConnParam->host.interval_max;
        
        pacl->pTemplateConnParam->latency     = pacl->pTemplateConnParam->host.max_slave_latency;
        pacl->pTemplateConnParam->timeout     = pacl->pTemplateConnParam->host.supervision_timeout;
        pacl->pTemplateConnParam->preferredPeriodicity    = 0x00; //value 0x00 means no preference
        pacl->pTemplateConnParam->referenceConnEventCount = 0;
        pacl->pTemplateConnParam->offset0     = 0xFFFF;
        pacl->pTemplateConnParam->offset1     = 0xFFFF;
        pacl->pTemplateConnParam->offset2     = 0xFFFF;
        pacl->pTemplateConnParam->offset3     = 0xFFFF;
        pacl->pTemplateConnParam->offset4     = 0xFFFF;
        pacl->pTemplateConnParam->offset5     = 0xFFFF;
      /*
                                     req0f.minInterval             = pacl->pTemplateConnParam->minInterval;
                                     req0f.maxInterval             = pacl->pTemplateConnParam->maxInterval;
                                     req0f.latency                 = pacl->pTemplateConnParam->latency;
                                     req0f.timeout                 = pacl->pTemplateConnParam->timeout;
                                     req0f.preferredPeriodicity    = pacl->pTemplateConnParam->preferredPeriodicity;
                                     req0f.referenceConnEventCount = pacl->pTemplateConnParam->referenceConnEventCount;
                                     req0f.offset0                 = pacl->pTemplateConnParam->offset0;
                                     req0f.offset1                 = pacl->pTemplateConnParam->offset1;
                                     req0f.offset2                 = pacl->pTemplateConnParam->offset2;
                                     req0f.offset3                 = pacl->pTemplateConnParam->offset3;
                                     req0f.offset4                 = pacl->pTemplateConnParam->offset4;
                                     req0f.offset5                 = pacl->pTemplateConnParam->offset5;
      */
        ht_write_hword( (uint8_t *)&(req0f.minInterval            ), pacl->pTemplateConnParam->minInterval );  //pCtrData[0~1]
        ht_write_hword( (uint8_t *)&(req0f.maxInterval            ), pacl->pTemplateConnParam->maxInterval );  //pCtrData[2~3]
        ht_write_hword( (uint8_t *)&(req0f.latency                ), pacl->pTemplateConnParam->latency     );  //pCtrData[4~5]
        ht_write_hword( (uint8_t *)&(req0f.timeout                ), pacl->pTemplateConnParam->timeout     );  //pCtrData[6~7]
        ht_write_byte ( (uint8_t *)&(req0f.preferredPeriodicity   ), pacl->pTemplateConnParam->preferredPeriodicity );
        ht_write_hword( (uint8_t *)&(req0f.referenceConnEventCount), pacl->pTemplateConnParam->referenceConnEventCount );
        ht_write_hword( (uint8_t *)&(req0f.offset0                ), pacl->pTemplateConnParam->offset0 );
        ht_write_hword( (uint8_t *)&(req0f.offset1                ), pacl->pTemplateConnParam->offset1 );
        ht_write_hword( (uint8_t *)&(req0f.offset2                ), pacl->pTemplateConnParam->offset2 );
        ht_write_hword( (uint8_t *)&(req0f.offset3                ), pacl->pTemplateConnParam->offset3 );
        ht_write_hword( (uint8_t *)&(req0f.offset4                ), pacl->pTemplateConnParam->offset4 );
        ht_write_hword( (uint8_t *)&(req0f.offset5                ), pacl->pTemplateConnParam->offset5 );
      //
    }
    done = PoTxllcPDU(sizeof(llcpdu_0f_connection_param_req_TypeDef), //1+23, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req0f),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_10_connection_param_rsp (LEACL_TypeDef *pacl)
{
    //Master shall not send the LL_10_CONNECTION_PARAM_RSP
    // Slave shall send an LL_10_CONNECTION_PARAM_RSP only in response to an LL_0F_CONNECTION_PARAM_REQ
    bool done;
    llcpdu_10_connection_param_rsp_TypeDef rsp10 =
    {
        .opcode       = 0x10 //LL_10_CONNECTION_PARAM_RSP
    //  .minInterval;            //ctrData[0~1]
    //  .maxInterval;            //ctrData[2~3]
    //  .latency;                //ctrData[4~5]
    //  .timeout;                //ctrData[6~7]
    //  .preferredPeriodicity;   //ctrData[8]
    //  .referenceConnEventCount;//ctrData[9~10]
    //  .offset0;                //ctrData[11~12] value 0xFFFF means invalid
    //  .offset1;                //ctrData[13~14] value 0xFFFF means invalid
    //  .offset2;                //ctrData[15~16] value 0xFFFF means invalid
    //  .offset3;                //ctrData[17~18] value 0xFFFF means invalid
    //  .offset4;                //ctrData[19~20] value 0xFFFF means invalid
    //  .offset5;                //ctrData[21~22] value 0xFFFF means invalid
    };
    
    //                           roger LL_0F_CONNECTION_PARAM_REQ
    //modify by
    //send_04_hciEvent_3e_06_LE_remote_conn_param_request
    //hcmd_2020_le_remote_conn_param_req_reply
    //                            send LL_10_CONNECTION_PARAM_RSP
    //  pacl->pTemplateConnParam used to send LL_10_CONNECTION_PARAM_RSP
    if (pacl->pTemplateConnParam != 0) {
      /*
        rsp10.minInterval             = pacl->pTemplateConnParam->minInterval;
        rsp10.maxInterval             = pacl->pTemplateConnParam->maxInterval;
        rsp10.latency                 = pacl->pTemplateConnParam->latency;
        rsp10.timeout                 = pacl->pTemplateConnParam->timeout;
        rsp10.preferredPeriodicity    = pacl->pTemplateConnParam->preferredPeriodicity;
        rsp10.referenceConnEventCount = pacl->pTemplateConnParam->referenceConnEventCount;
        rsp10.offset0                 = pacl->pTemplateConnParam->offset0;
        rsp10.offset1                 = pacl->pTemplateConnParam->offset1;
        rsp10.offset2                 = pacl->pTemplateConnParam->offset2;
        rsp10.offset3                 = pacl->pTemplateConnParam->offset3;
        rsp10.offset4                 = pacl->pTemplateConnParam->offset4;
        rsp10.offset5                 = pacl->pTemplateConnParam->offset5;
      */
        ht_write_hword( (uint8_t *)&(rsp10.minInterval             ), pacl->pTemplateConnParam->minInterval );
        ht_write_hword( (uint8_t *)&(rsp10.maxInterval             ), pacl->pTemplateConnParam->maxInterval );
        ht_write_hword( (uint8_t *)&(rsp10.latency                 ), pacl->pTemplateConnParam->latency );
        ht_write_hword( (uint8_t *)&(rsp10.timeout                 ), pacl->pTemplateConnParam->timeout );
        ht_write_byte ( (uint8_t *)&(rsp10.preferredPeriodicity    ), pacl->pTemplateConnParam->preferredPeriodicity );
        ht_write_hword( (uint8_t *)&(rsp10.referenceConnEventCount ), pacl->pTemplateConnParam->referenceConnEventCount );
        ht_write_hword( (uint8_t *)&(rsp10.offset0                 ), pacl->pTemplateConnParam->offset0 );
        ht_write_hword( (uint8_t *)&(rsp10.offset1                 ), pacl->pTemplateConnParam->offset1 );
        ht_write_hword( (uint8_t *)&(rsp10.offset2                 ), pacl->pTemplateConnParam->offset2 );
        ht_write_hword( (uint8_t *)&(rsp10.offset3                 ), pacl->pTemplateConnParam->offset3 );
        ht_write_hword( (uint8_t *)&(rsp10.offset4                 ), pacl->pTemplateConnParam->offset4 );
        ht_write_hword( (uint8_t *)&(rsp10.offset5                 ), pacl->pTemplateConnParam->offset5 );
      //
    }
    done = PoTxllcPDU(sizeof(llcpdu_10_connection_param_rsp_TypeDef), //1+23, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp10),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_11_reject_ext_ind(LEACL_TypeDef *pacl, uint8_t rejectOp, uint8_t reason)
{
    bool done;
    llcpdu_11_reject_ext_ind_TypeDef ind11 =
    {
        .opcode       = 0x11, //LL_11_REJECT_EXT_IND,
      //.rejectOpcode = rejectOp, //ctrData[0]
      //.errorCode    = reason    //ctrData[1]
    };
        ht_write_byte( (uint8_t *)&(ind11.rejectOpcode), rejectOp );  //ctrData[0]
        ht_write_byte( (uint8_t *)&(ind11.errorCode),    reason );    //ctrData[1]
    done = PoTxllcPDU_high(sizeof(llcpdu_11_reject_ext_ind_TypeDef), //1+2, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&ind11),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_11RejectExtInd_or_0dRejectInd(LEACL_TypeDef *pacl, uint8_t rejectOp, uint8_t reason)
{
    bool done;
    //issue an LL_11_REJECT_EXT_IND (if supported by both devices) 
    //      or LL_0D_REJECT_IND     (otherwise)
    if (REMOTE_FEATURE_is_supported_extended_reject_ind(pacl) &&  //bit 2  Extended Reject Indication ( LL_11_REJECT_EXT_IND )
        LOCAL_FEATURE_is_supported_extended_reject_ind() )        //bit 2  Extended Reject Indication ( LL_11_REJECT_EXT_IND )
    {
        llcpdu_11_reject_ext_ind_TypeDef ind11 =
        {
            .opcode       = 0x11, //LL_11_REJECT_EXT_IND,
            .rejectOpcode = rejectOp,
            .errorCode    = reason
        };
      //done = TxLLcPDU_11_reject_ext_ind()
        done = PoTxllcPDU(sizeof(llcpdu_11_reject_ext_ind_TypeDef), //1+2, //length
                          (llcpdu_PAYLOAD_TypeDef *)(&ind11),
                          pacl
                         );
    }
    else
    {
        llcpdu_0d_reject_ind_TypeDef ind0d =
        {
            .opcode    = 0x0D, //LL_0D_REJECT_IND,
            .errorCode = reason
        };
      //done = TxLLcPDU_0d_reject_ind()
        done = PoTxllcPDU(sizeof(llcpdu_0d_reject_ind_TypeDef), //1+1, //length
                          (llcpdu_PAYLOAD_TypeDef *)(&ind0d),
                          pacl
                         );
    }
    return (done);
}

bool proc_llcmsg_initiate_llc_le_ping_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_12_PING_REQ, wait LL_13_PING_RSP
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
        if (REMOTE_VERS_INFO_get_versNr(pacl) >= 0x07 &&    //0x07: Bluetooth spec 4.1
            LOCAL_FEATURE_is_supported_LE_ping() &&         //bit 4  LE Ping
            REMOTE_FEATURE_is_supported_LE_ping(pacl)       //bit 4  LE Ping
           )
        {
            done = TxLLcPDU_12_ping_req(pacl);
        }
        else
        {
            done = TxLLcPDU_12_ping_req(pacl);
          //done = 1;
        }
        //////////////
        if (done) {
            LLC_INITIATE_PROCEDURE_start(pacl, LL_12_PING_REQ); //LE Ping procedure
        }
        return (done);
    }
}

bool TxLLcPDU_12_ping_req (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_12_ping_req_TypeDef req12 =
    {
        .opcode       = 0x12 //LL_12_PING_REQ
    };
    done = PoTxllcPDU(sizeof(llcpdu_12_ping_req_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req12),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_13_ping_rsp (LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_13_ping_rsp_TypeDef rsp13 =
    {
        .opcode       = 0x13 //LL_13_PING_RSP
    };
    done = PoTxllcPDU(sizeof(llcpdu_13_ping_rsp_TypeDef), //1+0, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp13),
                      pacl
                     );
    return (done);
}

bool proc_llcmsg_initiate_llc_data_length_update_procedure(llcmsgQ_node_TypeDef *qNode) //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP
{
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint8_t  byHost;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1] conn handle
    byHost = ht_read_byte (qNode->msgparam+2); //param[2]     0x00:autonomously by LL, 0x01:by Host
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1; //1:abort msg, because conn not found
    }
    else
    {
        if (LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(pacl)) {
            //only one LL control procedure shall be initiated in the LL at a time per connection per device
            return (0); //0:keep msg
        }
        //////////////
        if (REMOTE_VERS_INFO_get_versNr(pacl) >= 0x08 &&                      //0x08: Bluetooth spec 4.2
            LOCAL_FEATURE_is_supported_LE_data_packet_length_extension() &&   //bit 5  LE Data Packet Length Extension
            REMOTE_FEATURE_is_supported_LE_data_packet_length_extension(pacl) //bit 5  LE Data Packet Length Extension
           )
        {
                done = TxLLcPDU_14_length_req(pacl);
        }
        else
        {
                done = TxLLcPDU_14_length_req(pacl);
              //done = 1;
        }
        //////////////
        if (done) {
            if (byHost) {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_14_LENGTH_REQ | FLAG_INITIATED_BY_HOST); //Data Length Update procedure
            }
            else {
                LLC_INITIATE_PROCEDURE_start(pacl, LL_14_LENGTH_REQ);                          //Data Length Update procedure
            }
        }
        return (done);
    }
}

bool TxLLcPDU_14_length_req(LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_14_length_req_TypeDef req14 =
    {
        .opcode      = 0x14, //LL_14_LENGTH_REQ,
      //.maxRxOctets = pacl->connMaxRxOctets,  //pCtrData[0~1]
      //.maxRxTime   = pacl->connMaxRxTime,    //pCtrData[2~3]
      //.maxTxOctets = pacl->connMaxTxOctets,  //pCtrData[4~5]
      //.maxTxTime   = pacl->connMaxTxTime     //pCtrData[6~7]
    };
        ht_write_hword( (uint8_t *)&(req14.maxRxOctets), pacl->connMaxRxOctets );  //pCtrData[0~1]
        ht_write_hword( (uint8_t *)&(req14.maxRxTime  ), pacl->connMaxRxTime   );  //pCtrData[2~3]
        ht_write_hword( (uint8_t *)&(req14.maxTxOctets), pacl->connMaxTxOctets );  //pCtrData[4~5]
        ht_write_hword( (uint8_t *)&(req14.maxTxTime  ), pacl->connMaxTxTime   );  //pCtrData[6~7]
    done = PoTxllcPDU(sizeof(llcpdu_14_length_req_TypeDef), //1+8, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&req14),
                      pacl
                     );
    return (done);
}

bool TxLLcPDU_15_length_rsp(LEACL_TypeDef *pacl)
{
    bool done;
    llcpdu_15_length_rsp_TypeDef rsp15 =
    {
        .opcode      = 0x15, //LL_15_LENGTH_RSP,
      //.maxRxOctets = pacl->connMaxRxOctets,  //pCtrData[0~1]
      //.maxRxTime   = pacl->connMaxRxTime,    //pCtrData[2~3]
      //.maxTxOctets = pacl->connMaxTxOctets,  //pCtrData[4~5]
      //.maxTxTime   = pacl->connMaxTxTime     //pCtrData[6~7]
    };
        //IUT select a value maxRxOctets between connMinOctetsLimitSpec and supportedMaxRxOctetsIUT
        //IUT select a value maxRxTime   between connMinTimeLimitSpec   and supportedMaxRxTimeIUT
        //IUT select a value maxTxOctets between connMinOctetsLimitSpec and supportedMaxTxOctetsIUT
        //IUT select a value maxTxTime   between connMinTimeLimitSpec   and supportedMaxTxTimeIUT
        ht_write_hword( (uint8_t *)&(rsp15.maxRxOctets), pacl->connMaxRxOctets );  //pCtrData[0~1]
        ht_write_hword( (uint8_t *)&(rsp15.maxRxTime  ), pacl->connMaxRxTime   );  //pCtrData[2~3]
        ht_write_hword( (uint8_t *)&(rsp15.maxTxOctets), pacl->connMaxTxOctets );  //pCtrData[4~5]
        ht_write_hword( (uint8_t *)&(rsp15.maxTxTime  ), pacl->connMaxTxTime   );  //pCtrData[6~7]
    done = PoTxllcPDU(sizeof(llcpdu_15_length_rsp_TypeDef), //1+8, //length
                      (llcpdu_PAYLOAD_TypeDef *)(&rsp15),
                      pacl
                     );
    if (done) {
        //effective
                    //LL/CON/MAS/BV-73-C [Master Data Length Update - Responding to Data Length Update Procedure; LE 1M PHY]
                    //LL/CON/SLA/BV-77-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE 1M PHY]
                    //LL/CON/MAS/BV-76-C [Master Data Length Update - Responding to Data Length Update Procedure; LE 2M PHY]
                    //LL/CON/SLA/BV-80-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE 2M PHY]
                    //LL/CON/MAS/BV-78-C [Master Data Length Update - Responding to Data Length Update Procedure; LE Coded PHY]
                    //LL/CON/SLA/BV-82-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE Coded PHY]
                    hcmsgQ_push( HCMSG_CHECK_CONN_EFFECTIVE_MAX_TRX_OCTETS_TIME_CHANGE, 
                                 LEACL_conn_handle(pacl), 
                                 0,//length
                                 (uint8_t *)0
                               );
    }
    return (done);
}

void calc_usedChannel_byRemapIndex(LLCHANNELMAP_TypeDef *pMap)
{
    uint8_t i,numUsed;
    for( i=0,numUsed=0; i<37; i++)
    {
        if (pMap->chM[i>>3] & (1<<(i&0x07)))
        {
            pMap->usedChIndex_byRemapIndex[numUsed] = i ; //(i<=10) ? (4+(i<<1)) : (6+(i<<1)) ;
                                           numUsed++;
            //           index   0  1  2  3  4  5  6  7  8  9 10
            //unmapped channel   4  6  8 10 12 14 16 18 20 22 24
            //           index  11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36
            //unmapped channel  28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78
        }
    }
            pMap->numUsedChannels = numUsed;
    /*
        uart_putchar_n('\n');
        uart_puts(" numUsedChannels="); uart_putu8(pMap->numUsedChannels); uart_putchar_n('\n');
        uart_puts(" usedChIndex_byRemapIndex[]="); 
    for( i=0; i<10; i++) {
        uart_putu8(pMap->usedChIndex_byRemapIndex[i]); uart_puts(" "); 
    }
        uart_putchar_n('\n');
    for( i=10; i<20; i++) {
        uart_putu8(pMap->usedChIndex_byRemapIndex[i]); uart_puts(" "); 
    }
        uart_putchar_n('\n');
    for( i=20; i<37; i++) {
        uart_putu8(pMap->usedChIndex_byRemapIndex[i]); uart_puts(" "); 
    }
        uart_putchar_n('\n');
    */
};

void calc_pacl_encrypt_SK(LEACL_TypeDef *pacl)
{
        uint8_t state[16];
        ht_memory_copy(state+0, pacl->encrypt.SKDm, 8);
        ht_memory_copy(state+8, pacl->encrypt.SKDs, 8);
        aes128_encrypt___lsbyte_to_msbyte(state+0, pacl->encrypt.LTK+0);
        ht_memory_copy(pacl->encrypt.SK,  state+0, 16);
}

