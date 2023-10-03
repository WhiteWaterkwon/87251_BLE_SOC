/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __STROBE_CMD_H
#define __STROBE_CMD_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include "ble_soc.h"                // RF_WT08() RF_RD08()

/* Exported types ------------------------------------------------------------------------------------------*/
typedef struct
{
    uint32_t byte4321 ;  // syncword
    uint8_t  byte0    ;  // LSByte, the LSByte must be unique for all 6 pipes
} address5602_TypeDef;

/* Exported functions --------------------------------------------------------------------------------------*/
static inline void strobe_software_reset(void)
{
    RF_WT08(0x1F, 0x80);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
                                    // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}
static inline void strobe_layer2_stop(void)
{
    RF_WT08(0x1F, 0x10);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
                                    // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}
static inline void strobe_TX(void)
{
    RF_WT08(0x1F, 0x01);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
                                    // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}
static inline void strobe_RX(void) //NOTICE: strobe_RX() RXM_START=1 only used for BLE mode
{
    RF_WT08(0x1F, 0x02);            //        [7]SWRST, [6]-, [5]BCNT_STOP, [4]L2_STOP, [3:2]-, [1]RXM_START, [0]TXM_START
                                    // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}


static inline void strobe_pdb_0_deepsleep(void)
{
    RF_WT08(0x1C, 0x00);            //PDB     [7:1]-,  [0]PDB_EN/RFPDB  RF power down bar enable, 0:deep sleep, 1:light sleep
}
static inline void strobe_pdb_1_lightsleep_wait_xclk_ready(void)
{
    RF_WT08(0x1C, 0x01);            //PDB     [7:1]-,  [0]PDB_EN/RFPDB  RF power down bar enable, 0:deep sleep, 1:light sleep
    while( (RF_RD08(0x31)&0x20)==0x00 ) //[5]XCLK_RDY
    {
    }
}


static inline void strobe_write_pipe_0_ID(const address5602_TypeDef addr)
{
    RF_WT08(0x00, addr.byte0);          //P0B0
    RF_WT08(0x01, addr.byte4321);       //P0B1
    RF_WT08(0x02, addr.byte4321>>8);    //P0B2
    RF_WT08(0x03, addr.byte4321>>16);   //P0B3
    RF_WT08(0x04, addr.byte4321>>24);   //P0B4
                     
}
/*
static inline void strobe_write_pipe_1_ID(const address5602_TypeDef addr)
{
    RF_WT08(0x05, addr.byte0);          //P1B0
    RF_WT08(0x06, addr.byte4321);       //P1B1
    RF_WT08(0x07, addr.byte4321>>8);    //P1B2
    RF_WT08(0x08, addr.byte4321>>16);   //P1B3
    RF_WT08(0x09, addr.byte4321>>24);   //P1B4
}*/
static inline void strobe_read_pipe_0_ID(address5602_TypeDef *pAddr)
{
    pAddr->byte4321 = RF_RD08(0x04);                       //P0B4
    pAddr->byte4321 = RF_RD08(0x03)+(pAddr->byte4321<<8);  //P0B3
    pAddr->byte4321 = RF_RD08(0x02)+(pAddr->byte4321<<8);  //P0B2
    pAddr->byte4321 = RF_RD08(0x01)+(pAddr->byte4321<<8);  //P0B1
    pAddr->byte0    = RF_RD08(0x00);                       //P0B0
}
/*
static inline void strobe_read_pipe_1_ID(address5602_TypeDef *pAddr)
{
    pAddr->byte4321 = RF_RD08(0x09);                       //P1B4
    pAddr->byte4321 = RF_RD08(0x08)+(pAddr->byte4321<<8);  //P1B3
    pAddr->byte4321 = RF_RD08(0x07)+(pAddr->byte4321<<8);  //P1B2
    pAddr->byte4321 = RF_RD08(0x06)+(pAddr->byte4321<<8);  //P1B1
    pAddr->byte0    = RF_RD08(0x05);                       //P1B0
}*/
/*
static inline unsigned char strobe_txfifo_write(unsigned int remainingbytes, unsigned char *pDeta)
{
    unsigned char j,Length;
    if(remainingbytes==0x00) {return 0x00;}
        RF_WT08(0x1B, 0x60|0x00);       //[6:4]PIPEAP_SEL: pipe selection PRX write Ack Payload  0h:pipe0, 1h:pipe1, 2h:pipe2, 3h:pipe3 4h:pipe4, 5h:pipe5, 6h:idle
                                        //[1:0]TXFIFO_SEL: 0h:txfifo_ack, 1h:txfifo_no_ack, 2h:idle
    if(remainingbytes>32) {Length=32;}
    else                  {Length=remainingbytes;}
    for(j=0;j<Length;j++) {
        RF_WT08(0x20, pDeta[j]);        //TXFIFO_DATA
    }
        RF_WT08(0x17, 0x10);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
    return Length;
}*/
/*
static inline unsigned char strobe_txfifo_write_no_ack(unsigned int remainingbytes, unsigned char *pDeta)
{
    unsigned char j,Length;
    if(remainingbytes==0x00) {return 0x00;}
        RF_WT08(0x1B, 0x60|0x01);       //[6:4]PIPEAP_SEL: pipe selection PRX write Ack Payload  0h:pipe0, 1h:pipe1, 2h:pipe2, 3h:pipe3 4h:pipe4, 5h:pipe5, 6h:idle
                                        //[1:0]TXFIFO_SEL: 0h:txfifo_ack, 1h:txfifo_no_ack, 2h:idle
    if(remainingbytes>32) {Length=32;}
    else                  {Length=remainingbytes;}
    for(j=0;j<Length;j++) {
        RF_WT08(0x20, pDeta[j]);        //TXFIFO_DATA
    }
        RF_WT08(0x17, 0x10);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
    return Length;
}*/
/*
static inline unsigned char strobe_txfifo_prx_w_ack_payload(unsigned char pipe, unsigned int remainingbytes, unsigned char *pDeta)
{
    unsigned char j,Length;
    if(remainingbytes==0x00) {return 0x00;}
        RF_WT08(0x1B, (pipe<<4)|0x02);  //[6:4]PIPEAP_SEL: pipe selection PRX write Ack Payload  0h:pipe0, 1h:pipe1, 2h:pipe2, 3h:pipe3 4h:pipe4, 5h:pipe5, 6h:idle
                                        //[1:0]TXFIFO_SEL: 0h:txfifo_ack, 1h:txfifo_no_ack, 2h:idle
    if(remainingbytes>32) {Length=32;}
    else                  {Length=remainingbytes;}
    for(j=0;j<Length;j++) {
        RF_WT08(0x20, pDeta[j]);        //TXFIFO_DATA
    }
        RF_WT08(0x17, 0x10);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
    return Length;
}*/
/*
static inline void strobe_rxfifo_read(unsigned char read_length, unsigned char *pch)
{
    unsigned char i;
    for(i=0;i<read_length;i++) {
        *pch = RF_RD08(0x24);           //RXFIFO_DATA     <<<<<<<<<<<
        pch++;
    }
        RF_WT08(0x17, 0x10);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}*/

/*  nRF24 mode with 3 TXFIFO
 *                                 -----------> address 0 to 32
 *                    TXFIFO idx 0 |  
 *                    TXFIFO idx 1 |
 *                    TXFIFO idx 2 V
 *  nRF24 mode with 3 RXFIFO
 *                                 -----------> address 0 to 32
 *                    RXFIFO idx 0 |  
 *                    RXFIFO idx 1 |
 *                    RXFIFO idx 2 V
 *    BLE mode with 1 TXRING
 *                                 -----------> address 0 to 256
 *                    TXRING idx 0 V  
 *    BLE mode with 1 RXRING
 *                                 -----------> address 0 to 256
 *                    RXRING idx 0 V  
 *
 */
/** [0]TXFIFO_FLUSH
 *  regardless of mode BLE or nRF24, clear idx_txi(TX input  index)  =0 and
 *                                         idx_txo(TX output index)  =0 and
 *                                         add_txi(TX input  address)=0 and
 *                                         add_txo(TX output address)=0
 */
/** [1]RXFIFO_FLUSH
 *  regardless of mode BLE or nRF24, clear idx_rxi(RX input  index)  =0 and
 *                                         idx_rxo(RX output index)  =0 and
 *                                         add_rxi(RX input  address)=0 and
 *                                         add_rxo(RX output address)=0
 */
/** [2]RSTFFINDEX
 *  regardless of mode BLE or nRF24, clear add_txo(TX output address)=0 only
 *  to resend the same payload TXRING
 */
static inline void strobe_txfifo_flush(void)
{
        RF_WT08(0x17, 0x01);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}
static inline void strobe_rxfifo_flush(void)
{
        RF_WT08(0x17, 0x02);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}
static inline void strobe_reset_output_index_of_txfifo(void)
{
        RF_WT08(0x17, 0x04);            //[7:5]-, [4]FIFO_END       FIFO ending command in 2.4G mode
                                        //[3]-,   [2]RSTFFINDEX     Reset output index of TX FIFO
                                        //        [1]RXFIFO_FLUSH   RX FIFO Flush command
                                        //        [0]TXFIFO_FLUSH   TX FIFO Flush command
                                        // Note¡GAbove commands set up 1 to generate strobe and then clear to 0 automatically
}

static inline void set_0bh_0ah_timeout_200us(void)
{
  #if   D_RATE == 0 //250 kbps
    RF_WT16(0x0A, 13 );                 //0x0A TIMEOUT[7:0]  unit  25us (13+1)* 25us = 350us > 310us  (IFS 150 + preamble 8*4us + syncword 32*4us)
                                        //0x0B TIMEOUT[15:8]
  #elif D_RATE == 1 //1 Mbps
    RF_WT16(0x0A, 8 );                  //0x0A TIMEOUT[7:0]  unit  25us  (8+1)* 25us = 225us > 190us  (IFS 150 + preamble 8 + syncword 32)
                                        //0x0B TIMEOUT[15:8]
  #elif D_RATE == 2 //2 Mbps
    RF_WT16(0x0A, 8 );                  //0x0A TIMEOUT[7:0]  unit  25us  (8+1)* 25us = 225us > 190us  (IFS 150 + preamble 8 + syncword 32)
                                        //0x0B TIMEOUT[15:8]
  #else
  ...
  #endif
}
static inline bool disabledTxen_is_set_1(void)
{
    return (RF_RD08(0x1E)&0x80);        //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline bool disabledRxen_is_set_1(void)
{
    return (RF_RD08(0x1E)&0x40);        //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_disabledTxen0_disabledRxen0(void)
{
    /*
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0x3F);         //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN,
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
    */
    RF_WT08(0x1E, 0x00);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_disabledTxen0_disabledRxen1(void)
{
    /*
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0x3F)|0x40;    //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN,
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
    */
    RF_WT08(0x1E, 0x40);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_disabledTxen1_disabledRxen0(void)
{
    /*
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0x3F)|0x80;    //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN,
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
    */
    RF_WT08(0x1E, 0x80);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_disabledTxen1_disabledRxen1(void)
{
    /*
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0x3F)|0xC0;    //1Eh     [7]DISABLED_TXEN, [6]DISABLED_RXEN,
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
    */
    RF_WT08(0x1E, 0xC0);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_PTX_mode_prmrx_0(void)
{
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0xDF);         //1Eh[5]PRM_RX=0
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_PRX_mode_prmrx_1(void)
{
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0xDF)|0x20;    //1Eh[5]PRM_RX=1
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}

static inline void reset_1Eh_CE_0(void)
{
    uint8_t rrrr;
    rrrr =(RF_RD08(0x1E)&0xEF);         //1Eh[4]CE=0
    RF_WT08(0x1E, rrrr);                //        [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}
static inline void set_abort_rx(void)
{
    //notice: must first reset CE=0
    reset_1Eh_CE_0();                   //1Eh[4]CE=0
    strobe_layer2_stop();               //abort rx
    set_disabledTxen0_disabledRxen0();  //1Eh  [7]DISABLED_TXEN, [6]DISABLED_RXEN, [5]PRM_RX, [4]CE, [3:0]-
}

static inline void set_amble2_0_preamble_1_byte(void)
{
    uint8_t rrrr;
    rrrr =(RF_RD08(0x41)&0xBF);         //41h[6]AMBLE2=0
    RF_WT08(0x41, rrrr);                //        
                                        //        [7]  STEP_EN    Phase step enable. 0:disable, 1:enable
                                        //        [6]  AMBLE2     Preamble length selection   0:Preamble one byte,  1:Preamble two bytes
                                        //       1[5]  SKIP_PRE   Skip preamble detect in BLE mode
                                        //        [4]  MDCG_EN    Match detect clock-gating enable for DCLK2. 0h¡GDisable, 1h¡GEnable
                                        //        [3:0]RSSI_CTHD  RSSI threshold for carrier detection in -dBm   => (RSSI_CTHD * 2 + 1) + 74 = RSSI threshold for carrier detection
}
static inline void set_amble2_1_preamble_2_bytes(void)
{
    uint8_t rrrr;
    rrrr =(RF_RD08(0x41)&0xBF)|0x40;    //41h[6]AMBLE2=1
    RF_WT08(0x41, rrrr);                //        
                                        //        [7]  STEP_EN    Phase step enable. 0:disable, 1:enable
                                        //        [6]  AMBLE2     Preamble length selection   0:Preamble one byte,  1:Preamble two bytes
                                        //       1[5]  SKIP_PRE   Skip preamble detect in BLE mode
                                        //        [4]  MDCG_EN    Match detect clock-gating enable for DCLK2. 0h¡GDisable, 1h¡GEnable
                                        //        [3:0]RSSI_CTHD  RSSI threshold for carrier detection in -dBm   => (RSSI_CTHD * 2 + 1) + 74 = RSSI threshold for carrier detection
}

#endif /* __STROBE_CMD_H */
