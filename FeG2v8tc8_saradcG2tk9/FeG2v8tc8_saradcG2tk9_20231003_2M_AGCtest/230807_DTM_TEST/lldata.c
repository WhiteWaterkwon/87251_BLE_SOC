/*********************************************************************************************************//**
 * @file    lldata.c
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
#include <stdint.h>
#include <string.h>                          //memcmp, memcpy
#include <stdbool.h>                         //bool

#include "bc5602b_host.h"                    //#define  __MALLOC_METHOD__
#include "pdma.h"                            //pdma_ram_bleRx_read_payload()
#include "usart.h"                           //uart_puts()
#include "lldata.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"


/* Private function prototypes -----------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/


/* Private types -------------------------------------------------------------------------------------------*/


/* Global functions ----------------------------------------------------------------------------------------*/


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
bool send_02_hciAclDataPacket_tohost(uint16_t bc_pb_hdl, uint16_t dataTotalLength, uint8_t *pData)
                                     //bc_pb_hdl[15:14]Broadcast_Flag       2 bits
                                     //                0b00 Point-to-point (ACL-U or LE-U)
                                     //bc_pb_hdl[13:12]Packet_Boundary_Flag 2 bits
                                     //                0b10 First automatically flushable packet of a higher layer message
                                     //                0b01 Continuing fragment of a higher layer message
                                     //bc_pb_hdl[11: 0]Connection_Handle   12 bits
{
  #if _UART_PUT_DEBUG_ == 1
        //
  #else
            uart_putchar(0x02);      //HCI_PTYPE_02_ACL
            uart_putchar(bc_pb_hdl);
            uart_putchar(bc_pb_hdl>>8);
            uart_putchar(dataTotalLength);
            uart_putchar(dataTotalLength>>8);
        while( dataTotalLength ) {
            uart_putchar(*pData);
            pData ++;
            dataTotalLength --;
        }
  #endif
        return (1);
}

bool receive_02_hciAclDataPacket_fromhost(uint16_t bc_pb_hdl, uint16_t dataTotalLength, uint8_t *pData)
                                     //bc_pb_hdl[15:14]Broadcast_Flag       2 bits
                                     //                0b00 Point-to-point (ACL-U or LE-U)
                                     //bc_pb_hdl[13:12]Packet_Boundary_Flag 2 bits
                                     //                0b00 First non-automatically-flushable packet of a higher layer message
                                     //                0b01 Continuing fragment of a higher layer message
                                     //bc_pb_hdl[11: 0]Connection_Handle   12 bits
{
    LEACL_TypeDef *pacl;
    hc_leacl_data_buffer_toair_TypeDef *pBuff;

        pacl = leacl_with_connHandle( bc_pb_hdl&0x0FFF );
    if (pacl == 0) {
//uart_puts("receive_hciAclData fail____"); uart_putchar_n('\n');
        return(0);
    }
    
        pBuff = ur0isr_hcAclDataBuffer_toair_allocate( bc_pb_hdl, dataTotalLength );
    if (pBuff == 0) {
//uart_puts("receive_hciAclData fail____"); uart_putchar_n('\n');
        return(0);
    }
    else
    {
        pdma_ram_bleTx_write_payload (pBuff->ram_start_address + 0x0010, // 0x0010: first 16 bytes for Session Key (SK)
                                      dataTotalLength, 
                                      pData
                                     );
        /*
        uint8_t *dst;
        dst = pBuff->octet + 0;
        while ( dataTotalLength ) {
            *dst = *pData;
            dst ++; pData ++;
            dataTotalLength --;
        }
        */
    }
    if( RINGBUF_isFull(pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ ) ) // full Q
    {
        // full Q
        hcAclDataBuffer_toair_free( pBuff );
        return(0);
    }
    else
    {
        //push this buffer to pacl->lldataTxQ
        RINGBUF_push( pacl->lldataTxQ, pBuff, TOTAL_NUM_ELEMENTS_lldataTxQ );
        return(1);
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
static unsigned char m2s_1[27] = {0x17,0x00, 0x04,0x00, 0x06,0x01,0x00,0xFF,0xFF,0x00,0x28,0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x01,0x00,0x40,0x6E};
static unsigned char m2s_2[11] = {0x07,0x00, 0x04,0x00, 0x08,0x09,0x00,0xFF,0xFF,0x03,0x28};
static unsigned char m2s_3[11] = {0x07,0x00, 0x04,0x00, 0x08,0x0C,0x00,0xFF,0xFF,0x03,0x28};
static unsigned char m2s_4[10] = {0x06,0x00, 0x05,0x00, 0x13,0x02,0x02,0x00,0x00,0x00};

static unsigned char s2m_1[ 9] = {0x05,0x00, 0x04,0x00, 0x07,0x09,0x00,0xFF,0xFF};
static unsigned char s2m_2[27] = {0x17,0x00, 0x04,0x00, 0x09,0x15,0x0A,0x00,0x10,0x0B,0x00,0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x03,0x00,0x40,0x6E};
static unsigned char s2m_3[16] = {0x0C,0x00, 0x05,0x00, 0x12,0x02,0x08,0x00,0x10,0x00,0x3C,0x00,0x00,0x00,0x90,0x01};
static unsigned char s2m_4[27] = {0x17,0x00, 0x04,0x00, 0x09,0x15,0x0D,0x00,0x0C,0x0E,0x00,0x9E,0xCA,0xDC,0x24,0x0E,0xE5,0xA9,0xE0,0x93,0xF3,0xA3,0xB5,0x02,0x00,0x40,0x6E};
                         //27: 0~(31-4) octets   4:MIC
bool debug_send_m2s_1(LEACL_TypeDef *pacl)
{
    return receive_02_hciAclDataPacket_fromhost(LEACL_conn_handle(pacl)|0x0000, sizeof(m2s_1), m2s_1+0);//debug_respond_l2cap_51822_ble_central_ble_app_uart()
}
bool debug_respond_l2cap_51822_ble_central_ble_app_uart(uint16_t connHandle, hc_leacl_data_buffer_tohost_TypeDef *p)
{
    if      ( memcmp( p->octet+0, m2s_1+0, sizeof(m2s_1) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(s2m_1), s2m_1+0);
    }
    else if ( memcmp( p->octet+0, m2s_2+0, sizeof(m2s_2) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(s2m_2), s2m_2+0);
    }
    else if ( memcmp( p->octet+0, m2s_3+0, sizeof(m2s_3) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(s2m_3), s2m_3+0);
    }
    else if ( memcmp( p->octet+0, m2s_4+0, sizeof(m2s_4) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(s2m_4), s2m_4+0);
    }
    else if ( memcmp( p->octet+0, s2m_1+0, sizeof(s2m_1) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(m2s_2), m2s_2+0);
    }
    else if ( memcmp( p->octet+0, s2m_2+0, sizeof(s2m_2) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(m2s_3), m2s_3+0);
    }
    else if ( memcmp( p->octet+0, s2m_3+0, sizeof(s2m_3) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(m2s_4), m2s_4+0);
    }
    else if ( memcmp( p->octet+0, s2m_4+0, sizeof(s2m_4) ) == 0 ) {
        return receive_02_hciAclDataPacket_fromhost(connHandle|0x0000, sizeof(m2s_1), m2s_1+0); //debug, repeat...
    }
    else {
//          uart_puts("respond_l2cap error!!!!!!!!!!!");//debug
//          uart_putchar_n('\n');//debug
        return (1);
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
unsigned char irq1isr_read_rxfifo_to_lldataRxQ(LEACL_TypeDef *pacl)
{
    hc_leacl_data_buffer_tohost_TypeDef *pBuff;
    if( pacl->receivedHdr.field.receivedLength == 0 ) {
        return 0;
    }
        pBuff = irq1isr_hcAclDataBuffer_tohost_alloc();
    if (pBuff == 0) {
//uart_puts("rxfifo_to_lldataRxQ fail____"); uart_putchar_n('\n');
        return 0;
    }
    /////////
    {
            pBuff->received_AESCCM_MIC_F_C0h = RF_RD08(0xC0);//debug
if (pacl->flag1.field.Rx_aesccm_enabled == 0) {
        pdma_ram_bleRx_read_payload(RX_CH1_DESADDR, pacl->receivedHdr.field.receivedLength, (unsigned char *)&(pBuff->octet) );
}
else {
        pdma_ram_bleRx_read_payload(RX_CH1_DESADDR, pacl->receivedHdr.field.receivedLength, (unsigned char *)&(pBuff->octet) );
}
        /////////
            pBuff->length                    = pacl->receivedHdr.field.receivedLength ;
            pBuff->connection_handle         = LEACL_conn_handle(pacl) ;
        if( pacl->receivedHdr.field.receivedLLID == 2 )        // llid=0b10 L2CAP Start
            pBuff->hci_pb_start=0x2000;//0x2000:start, 0x1000:continue
        else //if( pacl->receivedHdr.field.receivedLLID == 1 ) // llid=0b01 L2CAP Continue
            pBuff->hci_pb_start=0x1000;//0x2000:start, 0x1000:continue
    }
    /////////
    //push this buffer to pacl->lldataRxQ
    if( RINGBUF_isFull(pacl->lldataRxQ, TOTAL_NUM_ELEMENTS_lldataRxQ ) ) // full Q
    {
        hcAclDataBuffer_tohost_free( pBuff );
        return(0);
    }
    else
    {
        RINGBUF_push( pacl->lldataRxQ, pBuff, TOTAL_NUM_ELEMENTS_lldataRxQ );
        return pacl->receivedHdr.field.receivedLength;
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
void process_lldataRxQ___debug_print(LEACL_TypeDef *pacl, hc_leacl_data_buffer_tohost_TypeDef *pBuff)
{
            uart_putchar_n('\n');
    #if 0
    {
            uint8_t length;
            uart_puts("lldataRxQ.len=");
            length = RINGBUF_length( pacl->lldataRxQ );
            uart_putu8(length);
            uart_puts(",TxQ.len=");
            length = RINGBUF_length( pacl->lldataTxQ );
            uart_putu8(length);
    }
    #endif
            uart_puts(",C0h=");
            uart_putu8(pBuff->received_AESCCM_MIC_F_C0h);
            
            uart_puts(",lldataRxQ["); uart_putu8(pBuff->length); uart_puts("]");
    #if 1
    {
            uint16_t i;
        for(i=0; i<pBuff->length; i++) {
            uart_putu8(pBuff->octet[i]);
            uart_puts(" ");
        }
    }
    #endif
    #if 1 //#if   _CODE_VOL6_PARTC_1_2_ == 3           // 1.2   DERIVATION OF THE MIC AND ENCRYPTED DATA
    {
          uint16_t *pU16;
          if( pBuff->octet[2] == vol6_partc_1_2_3_plaindata[2] )
          {
            if ( ht_memory_compare(pBuff->octet+0, vol6_partc_1_2_3_plaindata, *(pU16 = (uint16_t *)(vol6_partc_1_2_3_plaindata+0)) + 4) == 0) {
                uart_puts("same, ");
            }
            else {
//RF_WT08(0x100, 0x1); //debug_RFLA_pulse_to_trigger_LA(); //debug
                uart_puts("different !!!!____");
            }
                uart_puts("[3]=");  uart_putu8(pBuff->octet[3]);
              //uart_puts(",PacketCounter=");  uart_putu32(pacl->RxPacketCounter.reg>>32); uart_putu32(pacl->RxPacketCounter.reg);
              //              uart_puts(",");  uart_putu32(pacl->TxPacketCounter.reg>>32); uart_putu32(pacl->TxPacketCounter.reg);
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
            if ( ht_memory_compare(pBuff->octet+0, vol6_partc_1_2_4_plaindata, *(pU16 = (uint16_t *)(vol6_partc_1_2_4_plaindata+0)) + 4) == 0) {
                uart_puts("same, ");
            }
            else {
//RF_WT08(0x100, 0x1); //debug_RFLA_pulse_to_trigger_LA(); //debug
                uart_puts("different !!!!____");
            }
                uart_puts("[3]=");  uart_putu8(pBuff->octet[3]);
              //uart_puts(",PacketCounter=");  uart_putu32(pacl->RxPacketCounter.reg>>32); uart_putu32(pacl->RxPacketCounter.reg);
              //              uart_puts(",");  uart_putu32(pacl->TxPacketCounter.reg>>32); uart_putu32(pacl->TxPacketCounter.reg);
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
              /*
                uart_putu8 (pacl->RxPacketCounter.field.packetCounter>>32);//39 bits
                uart_putu32(pacl->RxPacketCounter.field.packetCounter);    //39 bits
                uart_puts(", ");
                uart_putu8 (pacl->TxPacketCounter.field.packetCounter>>32);//39 bits
                uart_putu32(pacl->TxPacketCounter.field.packetCounter);    //39 bits
              */
    }
    #endif
            uart_putchar_n('\n');
}

void process_lldataRxQ(LEACL_TypeDef *pacl)
{
    bool done;
    hc_leacl_data_buffer_tohost_TypeDef *pBuff;
    if( RINGBUF_isEmpty( pacl->lldataRxQ ) ) // empty q
    {
        return; //exit empty q
    }
        pBuff = (hc_leacl_data_buffer_tohost_TypeDef *)( RINGBUF_peek( pacl->lldataRxQ, TOTAL_NUM_ELEMENTS_lldataRxQ) );
    /////////////////////////
    //  process_lldataRxQ___debug_print(pacl, pBuff);
    /////////////////////////
      //done = debug_respond_l2cap_51822_ble_central_ble_app_uart(LEACL_conn_handle(pacl), pBuff);
        done = send_02_hciAclDataPacket_tohost( (LEACL_conn_handle(pacl))|(pBuff->hci_pb_start), pBuff->length, (unsigned char *)&(pBuff->octet));
    /////////////////////////
    if (done) {
        hcAclDataBuffer_tohost_free( pBuff );
        RINGBUF_incrementPopIndex( pacl->lldataRxQ ); // ridx ++   pop Q
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
  hc_leacl_data_buffer_toair_TypeDef  HcAclDataBUFF2air [TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR ];
  hc_leacl_data_buffer_tohost_TypeDef HcAclDataBUFF2host[TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST];
static uint32_t HcAclDataBUFF2air_RAM_START_ADDRESS[TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR] = {
    TX_CH0_SRCADDR_LLDATA,
    TX_CH0_SRCADDR_LLDATA + 0x0110 //CH1_OFFSET
};

void hcAclDataBuffer_toair_init(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR; i++) {
        hcAclDataBuffer_toair_free( HcAclDataBUFF2air + i );
    }
}
void hcAclDataBuffer_tohost_init(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST; i++) {
        hcAclDataBuffer_tohost_free( HcAclDataBUFF2host + i );
    }
}

hc_leacl_data_buffer_toair_TypeDef * ur0isr_hcAclDataBuffer_toair_alloc(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR; i++) {
        if( HcAclDataBUFF2air[i].tttstate == 0 ) { // 0x00:free, 0x01:booked
            HcAclDataBUFF2air[i].ram_start_address = HcAclDataBUFF2air_RAM_START_ADDRESS[i];
            HcAclDataBUFF2air[i].tttstate = 1;     // 0x00:free, 0x01:booked
            return(HcAclDataBUFF2air + i);
        }
    }
    //all used already
            return(0);//NULL:fail
}
hc_leacl_data_buffer_toair_TypeDef * ur0isr_hcAclDataBuffer_toair_allocate(uint16_t bc_pb_hdl, uint16_t hdr_len)
{
    hc_leacl_data_buffer_toair_TypeDef *pDest ;
    LEACL_TypeDef *conn;

    if(hdr_len > LENGTH_LE_ACL_DATA_PACKET_TOAIR) {
            return(0); //NULL:fail
    }
    switch(bc_pb_hdl&0x3000) {
    case 0x2000:    //(Not allowed for LE-U) start of      automatically-flushable L2CAP PDU
    case 0x3000:    //(Not allowed for LE-U) A complete    automatically flushable L2CAP PDU
            return(0); //NULL:fail
    }
    if((conn = ur0isr_leacl_with_connHandle(bc_pb_hdl&0x0FFF)) == 0) {
            return(0); //NULL:fail
    }
    if((pDest = ur0isr_hcAclDataBuffer_toair_alloc()) == 0) {
            return(0); //NULL:fail
    }
    //--------------------------------------------------------
            pDest->r_index=0;
            pDest->hci_hdr_bc_pb_hdl = bc_pb_hdl;    //[15:14]:BC flag, [13:12]:PB flag, [11:0]:handle
            pDest->hci_hdr_length    = hdr_len  ;    //[15: 0]:length
    switch(pDest->hci_hdr_bc_pb_hdl&0x3000) {
    case 0x0000:    //(    Allowed for LE-U) start of  non-automatically-flushable L2CAP PDU
    case 0x2000:    //(Not allowed for LE-U) start of      automatically-flushable L2CAP PDU
    case 0x3000:    //(Not allowed for LE-U) A complete    automatically flushable L2CAP PDU
            pDest->air_pb_start=1;
            break;
    case 0x1000:    //(    Allowed for LE-U) Continuing fragment of a higher layer message
            pDest->air_pb_start=0;
            break;
    }
    //--------------------------------------------------------
    return(pDest);
}

bool irq1isr_hcAclDataBuffer_tohost_isAvailable(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST; i++) {
        if( HcAclDataBUFF2host[i].rrrstate == 0 ) { // 0x00:free, 0x01:booked
            return(1);
        }
    }
    //all used already
            return(0);
}
hc_leacl_data_buffer_tohost_TypeDef * irq1isr_hcAclDataBuffer_tohost_alloc(void)
{
    uint32_t i;
    for(i=0; i < TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST; i++) {
        if( HcAclDataBUFF2host[i].rrrstate == 0 ) { // 0x00:free, 0x01:booked
            HcAclDataBUFF2host[i].rrrstate = 1;     // 0x00:free, 0x01:booked
            return(HcAclDataBUFF2host + i);
        }
    }
    //all used already
            return(0);//NULL:fail
}

