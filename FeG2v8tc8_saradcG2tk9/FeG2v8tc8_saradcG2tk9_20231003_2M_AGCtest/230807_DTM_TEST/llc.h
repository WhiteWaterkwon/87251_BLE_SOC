/*********************************************************************************************************//**
 * @file    llc.h
 * @version $Rev:: 101          $
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

/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef HT_LLC_H
#define HT_LLC_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include "leacl.h"          //LEACL_TypeDef



/* Exported constants --------------------------------------------------------------------------------------*/
/*
#define LL_00_CONNECTION_UPDATE_IND        0x00    // spec 4.0
#define LL_01_CHANNEL_MAP_IND              0x01    // spec 4.0
#define LL_02_TERMINATE_IND                0x02    // spec 4.0
#define LL_03_ENC_REQ                      0x03    // spec 4.0
#define LL_04_ENC_RSP                      0x04    // spec 4.0
#define LL_05_START_ENC_REQ                0x05    // spec 4.0
#define LL_06_START_ENC_RSP                0x06    // spec 4.0
#define LL_07_UNKNOWN_RSP                  0x07    // spec 4.0
#define LL_08_FEATURE_REQ                  0x08    // spec 4.0
#define LL_09_FEATURE_RSP                  0x09    // spec 4.0
#define LL_0A_PAUSE_ENC_REQ                0x0A    // spec 4.0
#define LL_0B_PAUSE_ENC_RSP                0x0B    // spec 4.0
#define LL_0C_VERSION_IND                  0x0C    // spec 4.0
#define LL_0D_REJECT_IND                   0x0D    // spec 4.0
#define LL_0E_SLAVE_FEATURE_REQ            0x0E    // spec 4.1
#define LL_0F_CONNECTION_PARAM_REQ         0x0F    // spec 4.1
#define LL_10_CONNECTION_PARAM_RSP         0x10    // spec 4.1
#define LL_11_REJECT_EXT_IND               0x11    // spec 4.1
#define LL_12_PING_REQ                     0x12    // spec 4.1
#define LL_13_PING_RSP                     0x13    // spec 4.1
#define LL_14_LENGTH_REQ                   0x14    // spec 4.2
#define LL_15_LENGTH_RSP                   0x15    // spec 4.2
#define LL_16_PHY_REQ                      0x16    // spec 5.0
#define LL_17_PHY_RSP                      0x17    // spec 5.0
#define LL_18_PHY_UPDATE_IND               0x18    // spec 5.0
#define LL_19_MIN_USED_CHANNELS_IND        0x19    // spec 5.0
#define LL_1A_CTE_REQ                      0x1A    // spec 5.1
#define LL_1B_CTE_RSP                      0x1B    // spec 5.1
#define LL_1C_PERIODIC_SYNC_IND            0x1C    // spec 5.1
#define LL_1D_CLOCK_ACCURACY_REQ           0x1D    // spec 5.1
#define LL_1E_CLOCK_ACCURACY_RSP           0x1E    // spec 5.1
#define LL_1F_CIS_REQ                      0x1F    // spec 5.2
#define LL_20_CIS_RSP                      0x20    // spec 5.2
#define LL_21_CIS_IND                      0x21    // spec 5.2
#define LL_22_CIS_TERMINATE_IND            0x22    // spec 5.2
#define LL_23_POWER_CONTROL_REQ            0x23    // spec 5.2
#define LL_24_POWER_CONTROL_RSP            0x24    // spec 5.2
#define LL_25_POWER_CHANGE_IND             0x25    // spec 5.2
#define LL_26_SUBRATE_REQ                  0x26    // spec 5.3
#define LL_27_SUBRATE_IND                  0x27    // spec 5.3
#define LL_28_CHANNEL_REPORTING_IND        0x28    // spec 5.3
#define LL_29_CHANNEL_STATUS_IND           0x29    // spec 5.3
*/
typedef enum {
    LL_00_CONNECTION_UPDATE_IND = 0x00,
    LL_01_CHANNEL_MAP_IND,
    LL_02_TERMINATE_IND,
    LL_03_ENC_REQ,
    LL_04_ENC_RSP,
    LL_05_START_ENC_REQ,
    LL_06_START_ENC_RSP,
    LL_07_UNKNOWN_RSP,
    LL_08_FEATURE_REQ = 0x08,
    LL_09_FEATURE_RSP,
    LL_0A_PAUSE_ENC_REQ,
    LL_0B_PAUSE_ENC_RSP,
    LL_0C_VERSION_IND,
    LL_0D_REJECT_IND,
    LL_0E_SLAVE_FEATURE_REQ,
    LL_0F_CONNECTION_PARAM_REQ,
    LL_10_CONNECTION_PARAM_RSP = 0x10,
    LL_11_REJECT_EXT_IND,
    LL_12_PING_REQ,
    LL_13_PING_RSP,
    LL_14_LENGTH_REQ,
    LL_15_LENGTH_RSP,
    LL_16_PHY_REQ,
    LL_17_PHY_RSP,
    LL_18_PHY_UPDATE_IND = 0x18,
    LL_19_MIN_USED_CHANNELS_IND,
    LL_1A_CTE_REQ,
    LL_1B_CTE_RSP,
    LL_1C_PERIODIC_SYNC_IND,
    LL_1D_CLOCK_ACCURACY_REQ,
    LL_1E_CLOCK_ACCURACY_RSP,
    LL_1F_CIS_REQ,
    LL_20_CIS_RSP = 0x20,
    LL_21_CIS_IND,
    LL_22_CIS_TERMINATE_IND,
    LL_23_POWER_CONTROL_REQ,
    LL_24_POWER_CONTROL_RSP,
    LL_25_POWER_CHANGE_IND,
    LL_26_SUBRATE_REQ,
    LL_27_SUBRATE_IND,
    LL_28_CHANNEL_REPORTING_IND = 0x28,
    LL_29_CHANNEL_STATUS_IND
} OpcodeLLC_t;

/* Exported types ------------------------------------------------------------------------------------------*/


#define LEN_LE_LLC_PKT          (31-4)     // 0~( 31-4) octets   4:MIC
                                           // 0~(255-4) octets   4:MIC
typedef struct __attribute__((packed))
{
    uint8_t                     opcode;                     // opcode        1 octet
    uint8_t                     ctrData[LEN_LE_LLC_PKT  ];  // ctrData[] 0~250 octets       255-4-1     4:MIC 4 octets, 1:opcode 1 octet
  //uint8_t                     ctrData[LEN_LE_LLC_PKT-1];  // ctrData[] 0~250 octets       255-4-1     4:MIC 4 octets, 1:opcode 1 octet

} llcpdu_PAYLOAD_TypeDef;       // LL Control PDU Payload
/*
typedef struct //__attribute__((packed))
{
    htQueue_node_TypeDef node;//------------------------------------------4 octets---1st variable
    llcpdu_PAYLOAD_TypeDef      llcpdu_payload;         // 0~251 octets   address of 2nd variable __packed llcpdu_payload must be 4 multiple
    uint8_t                     length;
    uint16_t                    connection_handle;

} llcTxQ_node_TypeDef ;
*/
#define TOTAL_NUM_LE_LLC_PACKETS_TX    (TOTAL_NUM_ELEMENTS_llcTxQ - 0)
typedef struct //__attribute__((packed))
{
    llcpdu_PAYLOAD_TypeDef      llcpdu_payload;         // 0~251 octets   address of 2nd variable __packed llcpdu_payload must be 4 multiple
    uint32_t                    ram_start_address ;
    uint8_t                     length;
    uint16_t                    connection_handle;
    uint8_t                     tttstate ;          // 0x00:free, 0x01:booked, 0x02:loaded

}      llc_buffer_tx2air_TypeDef ;
extern llc_buffer_tx2air_TypeDef LLcBUFFtx[TOTAL_NUM_LE_LLC_PACKETS_TX];


#define TOTAL_NUM_LE_LLC_PACKETS_RX       (TOTAL_NUM_ELEMENTS_llcRxQ - 0)
typedef struct //__attribute__((packed))
{
    llcpdu_PAYLOAD_TypeDef      llcpdu_payload;         // 0~251 octets   address of 2nd variable __packed llcpdu_payload must be 4 multiple
    uint32_t                    received_AESCCM_MIC_F_C0h;
    uint16_t                    connection_handle;
    uint8_t                     length;
    uint8_t                     rrrstate ;          // 0x00:free, 0x01:booked

}      llc_buffer_rx_TypeDef ;
extern llc_buffer_rx_TypeDef LLcBUFFrx[TOTAL_NUM_LE_LLC_PACKETS_RX];


/*--------------------------------------------------------------------------------*/
typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x00 LL_00_CONNECTION_UPDATE_IND
    uint8_t                     winSize;                //ctrData[0]     unit 1.25 ms
    uint16_t                    winOffset;              //ctrData[1~2]   unit 1.25 ms
    uint16_t                    interval;               //ctrData[3~4]   unit 1.25 ms
    uint16_t                    latency;                //ctrData[5~6]
    uint16_t                    timeout;                //ctrData[7~8]   unit 10 ms
    uint16_t                    instant;                //ctrData[9~10]
} llcpdu_00_connection_update_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x01 LL_01_CHANNEL_MAP_IND
    uint8_t                     chM[5];                 //ctrData[0~4]
    uint16_t                    instant;                //ctrData[5~6]
} llcpdu_01_channel_map_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x02 LL_02_TERMINATE_IND
    uint8_t                     errorCode;              //ctrData[0]
} llcpdu_02_terminate_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x03 LL_03_ENC_REQ
    uint8_t                     rand[8];                //ctrData[0~7]
    uint8_t                     ediv[2];                //ctrData[8~9]  encrypted_diversifier
    uint8_t                     SKDm[8];                //ctrData[10~17]
    uint8_t                     IVm[4];                 //ctrData[18~21]
} llcpdu_03_enc_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x04 LL_04_ENC_RSP
    uint8_t                     SKDs[8];                //ctrData[0~7]
    uint8_t                     IVs[4];                 //ctrData[8~11]
} llcpdu_04_enc_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x05 LL_05_START_ENC_REQ
} llcpdu_05_start_enc_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x06 LL_06_START_ENC_RSP
} llcpdu_06_start_enc_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x07 LL_07_UNKNOWN_RSP
    uint8_t                     unknownType;            //ctrData[0]
} llcpdu_07_unknown_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x08 LL_08_FEATURE_REQ
    uint8_t                     featureSet[8];          //ctrData[0~7]
} llcpdu_08_feature_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x09 LL_09_FEATURE_RSP
    uint8_t                     featureSet[8];          //ctrData[0~7]
} llcpdu_09_feature_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0A LL_0A_PAUSE_ENC_REQ
} llcpdu_0a_pause_enc_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0B LL_0B_PAUSE_ENC_RSP
} llcpdu_0b_pause_enc_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0C LL_0C_VERSION_IND
    uint8_t                     llVersNr;               //ctrData[0] Link Layer Version
                                                        //              0x06: Bluetooth spec 4.0
                                                        //              0x07: Bluetooth spec 4.1
                                                        //              0x08: Bluetooth spec 4.2
                                                        //              0x09: Bluetooth spec 5.0
                                                        //              0x0A: Bluetooth spec 5.1
                                                        //              0x0B: Bluetooth spec 5.2
                                                        //              0x0C: Bluetooth spec 5.3
    uint16_t                    compId;                 //ctrData[1~2] Company identifiers
                                                        //              0x0046	MediaTek, Inc
    uint16_t                    llSubVersNr;            //ctrData[3~4]
} llcpdu_0c_version_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0D LL_0D_REJECT_IND
    uint8_t                     errorCode;              //ctrData[0]
} llcpdu_0d_reject_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0E LL_0E_SLAVE_FEATURE_REQ
    uint8_t                     featureSet[8];          //ctrData[0~7]
} llcpdu_0e_slave_feature_req_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x0F LL_0F_CONNECTION_PARAM_REQ
    uint16_t                    minInterval;            //ctrData[0~1]
    uint16_t                    maxInterval;            //ctrData[2~3]
    uint16_t                    latency;                //ctrData[4~5]
    uint16_t                    timeout;                //ctrData[6~7]
    uint8_t                     preferredPeriodicity;   //ctrData[8]
    uint16_t                    referenceConnEventCount;//ctrData[9~10]
    uint16_t                    offset0;                //ctrData[11~12] unit 1.25 ms
    uint16_t                    offset1;                //ctrData[13~14] unit 1.25 ms
    uint16_t                    offset2;                //ctrData[15~16] unit 1.25 ms
    uint16_t                    offset3;                //ctrData[17~18] unit 1.25 ms
    uint16_t                    offset4;                //ctrData[19~20] unit 1.25 ms
    uint16_t                    offset5;                //ctrData[21~22] unit 1.25 ms
} llcpdu_0f_connection_param_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x10 LL_10_CONNECTION_PARAM_RSP
    uint16_t                    minInterval;            //ctrData[0~1]
    uint16_t                    maxInterval;            //ctrData[2~3]
    uint16_t                    latency;                //ctrData[4~5]
    uint16_t                    timeout;                //ctrData[6~7]
    uint8_t                     preferredPeriodicity;   //ctrData[8]
    uint16_t                    referenceConnEventCount;//ctrData[9~10]
    uint16_t                    offset0;                //ctrData[11~12] unit 1.25 ms
    uint16_t                    offset1;                //ctrData[13~14] unit 1.25 ms
    uint16_t                    offset2;                //ctrData[15~16] unit 1.25 ms
    uint16_t                    offset3;                //ctrData[17~18] unit 1.25 ms
    uint16_t                    offset4;                //ctrData[19~20] unit 1.25 ms
    uint16_t                    offset5;                //ctrData[21~22] unit 1.25 ms
} llcpdu_10_connection_param_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x11 LL_11_REJECT_EXT_IND
    uint8_t                     rejectOpcode;           //ctrData[0]
    uint8_t                     errorCode;              //ctrData[1]
} llcpdu_11_reject_ext_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x12 LL_12_PING_REQ
} llcpdu_12_ping_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x13 LL_13_PING_RSP
} llcpdu_13_ping_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x14 LL_14_LENGTH_REQ              // spec 4.2
    uint16_t                    maxRxOctets;            //ctrData[0~1]  sender's connMaxRxOctets
    uint16_t                    maxRxTime;              //ctrData[2~3]  sender's connMaxRxTime
    uint16_t                    maxTxOctets;            //ctrData[4~5]  sender's connMaxTxOctets
    uint16_t                    maxTxTime;              //ctrData[6~7]  sender's connMaxTxTime
} llcpdu_14_length_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x15 LL_15_LENGTH_RSP              // spec 4.2
    uint16_t                    maxRxOctets;            //ctrData[0~1]  sender's connMaxRxOctets
    uint16_t                    maxRxTime;              //ctrData[2~3]  sender's connMaxRxTime
    uint16_t                    maxTxOctets;            //ctrData[4~5]  sender's connMaxTxOctets
    uint16_t                    maxTxTime;              //ctrData[6~7]  sender's connMaxTxTime
} llcpdu_15_length_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x16 LL_16_PHY_REQ                 // spec 5.0
    uint8_t                     tx_phys;                //ctrData[0]
    uint8_t                     rx_phys;                //ctrData[1]
} llcpdu_16_phy_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x17 LL_17_PHY_RSP                 // spec 5.0
    uint8_t                     tx_phys;                //ctrData[0]
    uint8_t                     rx_phys;                //ctrData[1]
} llcpdu_17_phy_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x18 LL_18_PHY_UPDATE_IND          // spec 5.0
    uint8_t                     m_to_s_phy;             //ctrData[0]
    uint8_t                     s_to_m_phy;             //ctrData[1]
    uint16_t                    instant;                //ctrData[2~3]
} llcpdu_18_phy_update_ind_TypeDef ;

typedef union
{
    struct {
        uint8_t le_1M_phy   : 1;
        uint8_t le_2M_phy   : 1;
        uint8_t le_Coded_phy: 1;
        uint8_t ___rfu______: 5;
    } field;
    uint8_t reg;

} PHYS_field_bit_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x19 LL_19_MIN_USED_CHANNELS_IND   // spec 5.0
    PHYS_field_bit_TypeDef      phys;                   //ctrData[0]
    uint8_t                     minUsedChannels;        //ctrData[1]
} llcpdu_19_min_used_channels_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x1A LL_1A_CTE_REQ                 // spec 5.1
    uint8_t                     cteTypeReq:2;           //ctrData[0]
    uint8_t                     rfu_______:1;           //ctrData[0]
    uint8_t                     minCteLenReq:5;         //ctrData[0]
} llcpdu_1a_cte_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x1B LL_1B_CTE_RSP                 // spec 5.1
} llcpdu_1b_cte_rsp_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x1C LL_1C_PERIODIC_SYNC_IND       // spec 5.1
    uint8_t                     ID[2];                  //ctrData[0~1]
    uint8_t                     syncInfo[18];           //ctrData[2~19]
    uint8_t                     connEventCount[2];      //ctrData[20~21]
    uint8_t                     sid:4;                  //ctrData[22]
    uint8_t                     AType:1;                //ctrData[22]
    uint8_t                     sca:3;                  //ctrData[22]
    uint8_t                     phy;                    //ctrData[23]
    uint8_t                     advA[6];                //ctrData[24~29]
    uint8_t                     syncConnEventCounter[2];//ctrData[30~31]
} llcpdu_1c_periodic_sync_ind_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x1D LL_1D_CLOCK_ACCURACY_REQ       // spec 5.1
    uint8_t                     sca;                    //ctrData[0]
} llcpdu_1d_clock_accuracy_req_TypeDef ;

typedef struct __attribute__((packed))
{
    uint8_t                     opcode ;                //opcode        0x1E LL_1E_CLOCK_ACCURACY_RSP       // spec 5.1
    uint8_t                     sca;                    //ctrData[0]
} llcpdu_1e_clock_accuracy_rsp_TypeDef ;


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/
void process_llcRxQ(LEACL_TypeDef *);
void llcBuffer_tx2air_init(void);
void llcBuffer_rx_init(void);

bool TxLLcPDU_00_connection_update_ind(LEACL_TypeDef *);
bool TxLLcPDU_01_channel_map_ind      (LEACL_TypeDef *);
bool TxLLcPDU_02_terminate_ind        (LEACL_TypeDef *, uint8_t reason);
bool TxLLcPDU_03_enc_req              (LEACL_TypeDef *, bool high_priority);
bool TxLLcPDU_04_enc_rsp              (LEACL_TypeDef *);
bool TxLLcPDU_05_start_enc_req        (LEACL_TypeDef *);
bool TxLLcPDU_06_start_enc_rsp        (LEACL_TypeDef *);
bool TxLLcPDU_07_unknown_rsp          (LEACL_TypeDef *, uint8_t unknownType);
bool TxLLcPDU_08_feature_req          (LEACL_TypeDef *);
bool TxLLcPDU_09_feature_rsp          (LEACL_TypeDef *);
bool TxLLcPDU_0a_pause_enc_req        (LEACL_TypeDef *);
bool TxLLcPDU_0b_pause_enc_rsp        (LEACL_TypeDef *);
bool TxLLcPDU_0c_version_ind          (LEACL_TypeDef *);
bool TxLLcPDU_0d_reject_ind           (LEACL_TypeDef *, uint8_t reason);
bool TxLLcPDU_0E_slave_feature_req    (LEACL_TypeDef *);
bool TxLLcPDU_0f_connection_param_req (LEACL_TypeDef *);
bool TxLLcPDU_10_connection_param_rsp (LEACL_TypeDef *);
bool TxLLcPDU_11_reject_ext_ind       (LEACL_TypeDef *, uint8_t rejectOpcode, uint8_t reason);
bool TxLLcPDU_12_ping_req             (LEACL_TypeDef *);
bool TxLLcPDU_13_ping_rsp             (LEACL_TypeDef *);
bool TxLLcPDU_14_length_req           (LEACL_TypeDef *);
bool TxLLcPDU_15_length_rsp           (LEACL_TypeDef *);

bool TxLLcPDU_11RejectExtInd_or_0dRejectInd(LEACL_TypeDef *, uint8_t rejectOp, uint8_t reason);

void calc_usedChannel_byRemapIndex(LLCHANNELMAP_TypeDef *);
void calc_pacl_encrypt_SK(LEACL_TypeDef *);

static inline void llcBuffer_tx2air_free(llc_buffer_tx2air_TypeDef * pBuff)
{
    pBuff->tttstate = 0x00;  // 0x00:free, 0x01:booked
}
static inline void llcBuffer_rx_free(llc_buffer_rx_TypeDef * pBuff)
{
    pBuff->rrrstate = 0x00;  // 0x00:free, 0x01:booked
}
llc_buffer_tx2air_TypeDef * llcBuffer_tx2air_alloc(void);
                   uint32_t llcBuffer_tx2air_availableNum(void);

                   bool irq1isr_llcBuffer_rx_isAvailable(void);
llc_buffer_rx_TypeDef * irq1isr_llcBuffer_rx_alloc(void);
llc_buffer_rx_TypeDef * irq1isr_read_rxfifo_to_llcRxQ(LEACL_TypeDef *);


#endif /* HT_LLC_H */
