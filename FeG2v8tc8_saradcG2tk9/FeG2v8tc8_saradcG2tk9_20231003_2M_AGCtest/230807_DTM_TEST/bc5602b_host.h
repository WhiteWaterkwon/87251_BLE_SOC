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
#ifndef __BC5602_HOST_H
#define __BC5602_HOST_H


/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdbool.h>                         //bool
#include <stdint.h>                          //uint8_t


/* Exported constants --------------------------------------------------------------------------------------*/
#define  MAX_ROUNDTRIPS_CE                  1
#define  __MALLOC_METHOD__                  1               // 1,2
#define Debug_COUNT_CRCF     2

// <<< Use Configuration Wizard in Context Menu >>>

// <o> M3 power saving
//    <0=>  none
//    <50=> Sleep
//    <51=> Deep Sleep 1
//    <52=> Deep Sleep 2
//    <54=> Power Down
#define PWR_SAVING_MODE            0

// <o> M3 RTC clock source from
//    <0=> LSI
//    <1=> LSE
#define RTC_CLK_FROM_LSE           1
// <o> RF_SEL 
//    <0=> 5602B
//    <1=> 5602G2TC8_v8
#define RF_SEL                  1
#define AGC_RXG_SEL		57
// <o> RF_CH 0~127, (2400+RF_CH)Mhz
//    <0-127:1>
#define RFCHANNEL                  80
// <o> FrontEnd RF XCLK
/*    <16=> 16 MHz */
//    <32=> 32 MHz
#define _FRONTEND_RF_XCLK_MHZ_     32

// <o> Mode 
//    <0=> PRX - Master
//    <1=> PTX - Slave
#define _HOST_TESTMODE_            1

// <o> Data Rate
//    <0=> 250 kbps
//    <1=>   1 Mbps
//    <2=>   2 Mbps
//    <4=> TX LE coded S=8, RX is auto switch by CI
//    <5=> TX LE coded S=2, RX is auto switch by CI
#define D_RATE                     2

// <o> RC2[7] CAR_EN   Clear After Read IRQ1
//    <0x00=> Disabled
//    <0x80=> Enabled CAR (no need write 0xFF to clear IRQ1)
#define CAR_EN_IRQ1                0x80

//<h> Address = Syncword + node
// <o> 42h[5:4] Address Length
/*    <0x10=> 3 bytes Address */
//    <0x20=> 4 bytes Address
/*    <0x30=> 5 bytes Address */
#define AW                         0x20    // 4 bytes Syncword
// <o> Syncword   pipe 0 Address [31:0]
//    <0x8E89BED6=> BLE  access address 8E89BED6
//    <0xBF8B38ED=> test 7262 use BF8B38ED
//    <0xDC00000D=> kidd access address reversed B000003B barker 7
//    <0xA17A00ED=> kidd access address reversed B7005E85 barker 11
//    <0xFC6C00CA=> kidd access address reversed 5300363F barker 13
#define P0_ACCESS_ADDR        0x8E89BED6
#define P0_SYNCWORD           (P0_ACCESS_ADDR>>8)
#define P0_LSBYTE             (P0_ACCESS_ADDR&0xFF)

//<h> RX
// <o> CFG1[6] AGC_EN
//    <0x00=> Disabled
//    <0x40=> Enabled
#define CFG1_AGC_EN                0x00
// <o> RXPL[7] RXPL_EN
//    <0x00=> Disabled
//    <0x80=> Enabled
#define RXPL_EN                    0x80
#define CONN_RXPL_EN               0x00
// <o> RXPL[6:0] MAX_RXPL
//    <0x00-0x7F:0x1>
#define MAX_RXPL                   (0x25  &0x7F) //                          notice bit-7 is RXPL_EN
#define CONN_MAX_RXPL              ((31-4)&0x7F) //(31-4)~(0x7F)    -4:MIC   notice bit-7 is RXPL_EN
//</h>


//<h> CRC
// <o> PKT1 [1:0]CRC_LENGTH
//    <0x00=> CRC disabled
//    <0x40=> CRC enabled, CRC8
//    <0x80=> CRC enabled, CRC16
//    <0xC0=> CRC enabled, CRC24
#define PKT1_CRCLEN                0xC0
// <o> PKT1 [7]CRC_ADD_EN
//    <0x00=> NOT include sync
//    <0x80=> include sync
#define PKT1_CRC_ADD_EN            0x00
//</h>

//<h> Whitening
// <o> PKT2[7] Whitening
//    <0x00=> Disabled
//    <0x80=> Enabled
#define PKT2_WHT_EN                0x80
// <o> PKT2[6:0] Whitening Seed
//    <0x53=> 0x53 for 2402 MHz channel index 37
//    <0x33=> 0x33 for 2426 MHz channel index 38
//    <0x73=> 0x73 for 2480 MHz channel index 39
#define PKT2_WHT_SEED              0x73
//</h>

// <o> E0h[7] MASK_ANCHOR
//    <0x00=> 0x00 Bypass,  IRQ_ANCHOR be send to MCU when Anchor Point happen
//    <0x80=> 0x80 Mask, No IRQ_ANCHOR when Anchor Point happen
#define E0h_80_MASK_ANCHOR_AT_RTR_EVENT  0x00
#define E0h_80_MASK_ANCHOR_AT_TRT_EVENT  0x80
#define E0h_80_MASK_ANCHOR_AT_MASTER_CE  0x80
#define E0h_80_MASK_ANCHOR_AT_SLAVE_CE   0x00
#define E0h_80_MASK_ANCHOR_AT_EXT_TX_EVENT 0x80

// <o> E0h[4] EN_TEST_LO
//    <0x00=> 0x00 
//    <0x10=> 0x10 LO test enable, used in engineering mode
#define E0h_10_EN_TEST_LO                0x00

// <<< end of configuration section >>>


#define  CH1_OFFSET                         0x0110          // 0x0010 by  16 bytes Session Key
                                                            // 0x0100 by 256 bytes maximum payload
#define  TX_CH0_SRCADDR_LLDATA              0x0000          // 0x1000 minus 0x0100   000,110,  220,260,  2A0
#define  TX_CH0_SRCADDR_LLC                 0x0220          // 0x1000 minus 0x0100   000,110,  220,260,  2A0
#define  TX_CH0_SRCADDR_EMPTY_PDU           0x02A0          // 0x1000 minus 0x0100   000,110,  220,260,  2A0
#define  TX_CH1_DESADDR                     0x02B0          // 0x1000 minus 0x0100   2B0

#define  RX_CH0_SRCADDR                     0x03B0          // 0x1000 minus 0x0100   3B0
#define  RX_CH1_DESADDR                     0x04C0          // 0x1000 minus 0x0100   4C0,5C0,  6C0,7C0
#define  CH2TXADDR_TRT_T1                   0x0AC0          // 0x1000 minus 0x0100
#define  CH2TXADDR_TRT_T2_SCANRSP           0x0BC0          // 0x1000 minus 0x0100
#define  CH2TXADDR_RTR_TX_SCANREQ           0x0CC0          // 0x1000 minus 0x0100
#define  CH2TXADDR_RTR_TX_CONNECTIND        0x0CD0          // 0x1000 minus 0x0100
#define  CH2RXADDR_RTR_R1                   0x0D00          // 0x1000 minus 0x0100
#define  CH2RXADDR_RTR_R2_SCANRSP           0x0E00          // 0x1000 minus 0x0100
#define  CH2RXADDR_TRT_RX                   0x0F00          // 0x1000 minus 0x0100
#define  MASK_BCMATCH_IRQ_AT_TX_AESCCM_ON   0x04            //33h[2]MASK_BCMATCH  0x04:disable IRQ_BCMATCH, 0x00:enable IRQ_BCMATCH
#define  MASK_BCMATCH_IRQ_AT_TX_AESCCM_OFF  0x04            //33h[2]MASK_BCMATCH  0x04:disable IRQ_BCMATCH, 0x00:enable IRQ_BCMATCH
#define  DV_ADDR_0_ADDRESS                  0x3A            //0x39: 39h~3Eh: 
                                                            //0x3A: 3Ah~3Fh: 2022oct11, HT32F87251.sof
#define  AESCCM_LL_SIGNALING                1
extern uint8_t  vol6_partc_1_SKDm[8];
extern uint8_t  vol6_partc_1_SKDs[8];
extern uint8_t  vol6_partc_1_2_IVm[4];
extern uint8_t  vol6_partc_1_2_IVs[4];
extern uint8_t  vol6_partc_1_LTK_lso_to_mso[16];
extern uint8_t  vol6_partc_1_2_SK[16];
extern uint8_t  vol6_partc_1_2_1_plaindata[16];     // 1.2.  1. LL_START_ENC_RSP1(0x06) (packetCounter 0, M --> S)
extern uint8_t  vol6_partc_1_2_2_plaindata[16];     // 1.2.  2. LL_START_ENC_RSP1(0x06) (packetCounter 0, S --> M)
extern uint8_t  vol6_partc_1_2_3_plaindata[256];    // 1.2.  3. Data packet1            (packetCounter 1, M --> S)
extern uint8_t  vol6_partc_1_2_4_plaindata[256];    // 1.2.  4. Data packet2            (packetCounter 1, S --> M)




/* Exported types ------------------------------------------------------------------------------------------*/


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/
void bc5602B_initial(void);
void bc5602B_process(void);
void debug_hcfsm_state_ST_0(void);
void debug_hcfsm_state_ST_Sean_IDLE(void);

/*
unsigned char debug_rssi_id_ok_note_max[100];
unsigned char debug_rssi_id_ok_note_min[100];
unsigned char debug_rssi_gainsel_note_max[100];
unsigned char debug_rssi_gainsel_note_min[100];
*/
extern unsigned char debug_power;

void set_txHeader_txPayload_advIND(uint8_t *pAdvA, uint8_t advDataLength, uint8_t *pAdvData);

uint32_t tmr625usGetCurrentTicks(void);
bool tmr625usIsExpired(uint32_t offset_ticks, uint32_t Texpire);    //Texpire in unit of 625us
void delay_unit625us(uint32_t Texpire);

void init_0rf_1adc_dtm_mode(uint8_t phy);

void kidd_taskENTER_CRITICAL(void);
void kidd_taskEXIT_CRITICAL(void);

void     ht_memory_copy   (uint8_t *dst, uint8_t *src, uint32_t len);
void     ht_memory_set    (uint8_t *dst, uint8_t v,    uint32_t len);
uint32_t ht_memory_compare(uint8_t *s1,  uint8_t *s2,  uint32_t count);

void init_2fpga_directed_test_mode(uint8_t tx_rx_select, uint8_t channel,uint8_t phy,uint8_t modulation_index,uint8_t expected_cte_length,uint8_t expected_cte_type);

static inline uint8_t ht_read_byte( uint8_t *src )
{
    uint8_t r;
    r = src[0];
    return r;
}
static inline uint16_t ht_read_hword( uint8_t *src )
{
    uint16_t r;
    r = (((uint16_t)src[1]) << 8) +
                    src[0];
    return r;
}
static inline uint32_t ht_read_word( uint8_t *src )
{
    uint32_t r;
    r = (((uint32_t)src[3]) << 24) +
        (((uint32_t)src[2]) << 16) +
        (((uint32_t)src[1]) <<  8) +
                    src[0];
    return r;
}
static inline void ht_write_byte( uint8_t *dst, uint8_t v )
{
    dst[0] = (uint8_t)(v);
}
static inline void ht_write_hword( uint8_t *dst, uint16_t v )
{
    dst[0] = (uint8_t)(v);
    dst[1] = (uint8_t)(v >>  8);
}
static inline void ht_write_word( uint8_t *dst, uint32_t v )
{
    dst[0] = (uint8_t)(v);
    dst[1] = (uint8_t)(v >>  8);
    dst[2] = (uint8_t)(v >> 16);
    dst[3] = (uint8_t)(v >> 24);
}



#endif /* __BC5602_HOST_H */
