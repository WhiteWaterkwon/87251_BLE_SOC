/*********************************************************************************************************//**
 * @file    leacl.h
 * @version $Rev:: 100          $
 * @date    $Date:: 2022-02-22 #$
 * @brief   The header file of the LEACL functions.
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
#ifndef __LEACL_H
#define __LEACL_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include "ringbuf.h"        //llcTxQ_TypeDef, llcRxQ_TypeDef, lldataTxQ_TypeDef, lldataRxQ_TypeDef
#include "htqueue.h"        //dllist_node_TypeDef

/* Exported types ------------------------------------------------------------------------------------------*/
typedef struct
{
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t interval;//
    uint16_t latency;
    uint16_t timeout;
    uint8_t  preferredPeriodicity;
    uint16_t referenceConnEventCount;
    uint16_t offset0;
    uint16_t offset1;
    uint16_t offset2;
    uint16_t offset3;
    uint16_t offset4;
    uint16_t offset5;
    struct {
    uint16_t interval_min;
    uint16_t interval_max;
    uint16_t max_slave_latency;
    uint16_t supervision_timeout;
    uint16_t min_CE_length;
    uint16_t max_CE_length;
    } host;

} LLCONNECTIONPARAMS_TEMPLATE_TypeDef;

typedef struct
{
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;

} LLCONNECTIONPARAMS_TypeDef;

typedef struct
{
    uint8_t  winSize;
    uint16_t winOffset;
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;
    uint16_t instant;                       // instant @ LL_00_CONNECTION_UPDATE_IND
    uint16_t iMaster_deltaT_minus_winOffset;
    uint8_t  iSlave_updateState;
    uint8_t  iSlave_ce1stRx_lost_rxto_consecutive_times_before1stRoger;

} LLCONNECTIONUPDATE_TypeDef;

typedef struct
{
    uint8_t  chM[5];
    uint16_t instant;                       // instant @ LL_01_CHANNEL_MAP_IND
    uint8_t  numUsedChannels;
    uint8_t  usedChIndex_byRemapIndex[37];  // usedChannel_byRemapIndex

} LLCHANNELMAP_TypeDef;

typedef struct
{
    uint8_t  SKDm[8];
    uint8_t  SKDs[8];
    uint8_t  IVm[4];
    uint8_t  IVs[4];
    uint8_t  rand[8];           // random_number           enable_encryption
    uint8_t  ediv[2];           // encrypted_diversifier   enable_encryption
    uint8_t  LTK[16];           // Long Term Key
    uint8_t  SK[16];            // Session Key

} LLENCRYPTION_TypeDef;

typedef struct
{
    uint8_t  versNr;
    uint16_t compId;
    uint16_t subVersNr;

} REMOTE_VERSION_INFO_TypeDef;

typedef struct
{
    uint8_t  featureSet[8];                 //

} REMOTE_FEATURES_TypeDef;

typedef union
{
    struct {
        uint16_t transmitLLID       : 2;
        uint16_t nextExpectedSeqNum : 1;
        uint16_t transmitSeqNum     : 1;
        uint16_t md     : 1;
        uint16_t rfu    : 3;
        uint16_t transmitLength : 8;
    } field;
    uint16_t reg;

} TRANSMIT_LLDATA_PDU_HDR_TypeDef;

typedef union
{
    struct {
        uint16_t receivedLLID   : 2;
        uint16_t nesn           : 1;
        uint16_t receivedSeqNum : 1;
        uint16_t md     : 1;
        uint16_t rfu    : 3;
        uint16_t receivedLength : 8;
    } field;
    uint16_t reg;

} RECEIVED_LLDATA_PDU_HDR_TypeDef;

typedef union
{
    struct {
        uint64_t packetCounter : 39;
        uint64_t directionBit  :  1; //0:Slave to Master, 1:Master to Slave
        uint64_t ________dummy : 24;
    } field;
    uint64_t reg;

} CCM_NONCE_PACKET_COUNTER_TypeDef;

typedef union
{
    struct {
        uint32_t cntlo : 31;    //F0h ~ F3h: NUM_CNT_LO
        uint32_t ____z :  1;
    } field;
    uint32_t reg;

} BLETIMER_CNT_LO_625US_TypeDef;

typedef union
{
    struct {
        uint16_t phase : 10;    //E2h ~ E3h: NUM_FINE
        uint16_t ____z :  6;
    } field;
    uint16_t reg;

} BLETIMER_PHASE_1US_TypeDef;

typedef struct
{
    BLETIMER_CNT_LO_625US_TypeDef  cntLoIn625us;
    BLETIMER_PHASE_1US_TypeDef     phaseIn1us;

} BLETIMER_TypeDef;

typedef union
{
    struct {
        uint32_t alreadySentLLVersionInd : 1;
        uint32_t ever_pause_encryption : 1; //debug
        uint32_t ____z :  30;
    } field;
    uint32_t reg;

} FLAG_0_CHANGED_AT_MAIN_TypeDef;

typedef union
{
    struct {
        uint32_t Tx_aesccm_enabled : 1;
        uint32_t Rx_aesccm_enabled : 1;
        uint32_t stop_tx_dataPhyChannelPdu : 1; //Encryption Start procedure
        uint32_t current_llcTxQ_type : 1;       //0:llcTxQ, 1:llcTxQ_high
        uint32_t roger_connectind : 1;
        uint32_t sent_connectind : 1;
        uint32_t conn_setup_success_syncword_match : 1;
        uint32_t conn_setup_success_roger_nesn_1 : 1;
        uint32_t rxLLTerminateInd_txAck : 2;    //0x00:none
                                                //0x01:sending     ACK for rx LL_02_TERMINATE_IND
                                                //0x02:finish send ACK for rx LL_02_TERMINATE_IND
        uint32_t txLLTerminateInd_w4ack : 2;    //0x00:none
                                                //0x01:sending LL_02_TERMINATE_IND  , waiting ACKnowledgment...
                                                //0x02:sending LL_02_TERMINATE_IND  ,   roger ACKnowledgment
        uint32_t ____z :  20;
    } field;
    uint32_t reg;

} FLAG_1_CHANGED_AT_IRQ1_TypeDef;

//////////////////////////////////////////
#define FLAG_PEER_ALREADY_RESPOND_AT_LEAST_ONE_PDU (1UL << 29) //llcInitiateProcedure.state bit-29
#define FLAG_INITIATED_BY_HOST                     (1UL << 30) //llcInitiateProcedure.state bit-30  0:autonomously by Link Layer, 1:by Host
#define FLAG_INVOLVE_INSTANT                       (1UL << 31) //llcInitiateProcedure.state bit-31
typedef union
{
    struct {
        uint32_t llcOP : 8;
        uint32_t __rfu : 21;
        uint32_t flag_peer_already_respond_atLeastOnePdu: 1; //#define FLAG_PEER_ALREADY_RESPOND_AT_LEAST_ONE_PDU  (1UL << 29)
        uint32_t flag_initiated_by_Host : 1;                 //#define FLAG_INITIATED_BY_HOST                      (1UL << 30)
        uint32_t flag_involve_instant   : 1;                 //#define FLAG_INVOLVE_INSTANT                        (1UL << 31)
    } field;
    uint32_t reg;

} LLC_INIT_PROC_STATE_t;
typedef struct
{
    LLC_INIT_PROC_STATE_t state;
    uint32_t              timeout_offset_ticks;

} LLC_INITIATE_PROCEDURE_TypeDef;

#define LLC_INITIATE_PROCEDURE_start( acl, st32 )               { acl->llcInitiateProcedure.state.reg = st32; \
                                                                  acl->llcInitiateProcedure.timeout_offset_ticks = tmr625usGetCurrentTicks(); }
#define LLC_INITIATE_PROCEDURE_end_complete( acl )              { acl->llcInitiateProcedure.state.reg = 0; }
#define LLC_INITIATE_PROCEDURE_is_active( acl )                 ( acl->llcInitiateProcedure.state.reg != 0 )
                                                   //llcOP LL_00_CONNECTION_UPDATE_IND == 0x00 but with flag_involve_instant == 1
#define LLC_INITIATE_PROCEDURE_transition_to_llcOP(acl,op8)     { acl->llcInitiateProcedure.state.field.llcOP = op8; }
#define LLC_INITIATE_PROCEDURE_is_llcOP( acl, op8 )             ( acl->llcInitiateProcedure.state.field.llcOP == op8 )
#define LLC_INITIATE_PROCEDURE_set_peerRespondAtLeastOnePdu(acl){ acl->llcInitiateProcedure.state.field.flag_peer_already_respond_atLeastOnePdu = 1; }
#define LLC_INITIATE_PROCEDURE_is_peerRespondAtLeastOnePdu(acl) ( acl->llcInitiateProcedure.state.field.flag_peer_already_respond_atLeastOnePdu )
#define LLC_INITIATE_PROCEDURE_is_initiated_by_host( acl )      ( acl->llcInitiateProcedure.state.field.flag_initiated_by_Host )
#define LLC_INITIATE_PROCEDURE_is_involve_instant( acl )        ( acl->llcInitiateProcedure.state.field.flag_involve_instant )
#define LLC_INITIATE_PROCEDURE_is_expired( acl, twenty )          tmr625usIsExpired( acl->llcInitiateProcedure.timeout_offset_ticks, twenty )

typedef union
{
    struct {
        uint32_t llcOP : 8 ;
        uint32_t __rfu : 22;
        uint32_t flag_initiated_by_Host : 1;          //#define FLAG_INITIATED_BY_HOST          (1UL << 30)
        uint32_t flag_involve_instant   : 1;          //#define FLAG_INVOLVE_INSTANT            (1UL << 31)
    } field;
    uint32_t reg;

} LLC_RESP_PROC_STATE_t;
typedef struct
{
    LLC_RESP_PROC_STATE_t state;
    uint32_t              timeout_offset_ticks;

} LLC_RESPOND_PROCEDURE_TypeDef;

#define LLC_RESPOND_PROCEDURE_start( acl, st32 )            { acl->llcRespondProcedure.state.reg = st32; \
                                                              acl->llcRespondProcedure.timeout_offset_ticks = tmr625usGetCurrentTicks(); }
#define LLC_RESPOND_PROCEDURE_end_complete( acl )           { acl->llcRespondProcedure.state.reg = 0; }
#define LLC_RESPOND_PROCEDURE_is_active( acl )              ( acl->llcRespondProcedure.state.reg != 0 )
#define LLC_RESPOND_PROCEDURE_transition_to_llcOP(acl,op8)  { acl->llcRespondProcedure.state.field.llcOP = op8; }
#define LLC_RESPOND_PROCEDURE_is_llcOP( acl, op8 )          ( acl->llcRespondProcedure.state.field.llcOP == op8 )
#define LLC_RESPOND_PROCEDURE_is_involve_instant( acl )     ( acl->llcRespondProcedure.state.field.flag_involve_instant )
#define LLC_RESPOND_PROCEDURE_is_expired( acl, twenty )       tmr625usIsExpired( acl->llcRespondProcedure.timeout_offset_ticks, twenty )

//.........................................................
#define LLC_INITIATE_PROCEDURE_is_not_allowed_to_initiate(acl) (    LLC_INITIATE_PROCEDURE_is_active(acl) \
                                                                 || LLC_RESPOND_PROCEDURE_is_involve_instant(acl) \
                                                               )

//////////////////////////////////////////
typedef struct                          // LE asynchronous connection (LE ACL)
{
    dllist_node_TypeDef node;//<<<<<<<<<<< must be 1st variable
    
    uint16_t aclConnHandle:12;          // ACL connection handle[11:0]
    uint16_t zzzzzzzzzzz:4;
    uint8_t  role;                      // 0x00:master, 0x01:slave
    uint8_t  roundtrip_times_in_a_ce;
    
    uint8_t  remoteDeviceAddr[6];       // connection
    uint8_t  remote_said_unknownType[6];//1-bit indicate 1-opcode, maximum LL_CHANNEL_STATUS_IND 0x29 need 6 octets
    REMOTE_FEATURES_TypeDef     *pRemoteFeature;
    REMOTE_VERSION_INFO_TypeDef *pRemoteVersInfo;
    
    TRANSMIT_LLDATA_PDU_HDR_TypeDef transmitHdr ; // @ Connection Event
    RECEIVED_LLDATA_PDU_HDR_TypeDef receivedHdr ; // @ Connection Event
    
    uint32_t AA;
    uint8_t  crcInit[3];
    uint8_t  hopIncrement:5;            // in the range of 5 to 16
    uint8_t  sca:3;                     // 3 bits
    uint16_t advEventCount;		// 16-bit, shall be set to zero on the first connection event
                                        //         shall wrap from 0xFFFF to 0x0000
    uint16_t periodicEventCount;	//                                    
    uint16_t connEventCount;            // 16-bit, shall be set to zero on the first connection event
                                        //         shall wrap from 0xFFFF to 0x0000
    CCM_NONCE_PACKET_COUNTER_TypeDef TxPacketCounter; // 39-bit
    CCM_NONCE_PACKET_COUNTER_TypeDef RxPacketCounter; // 39-bit

    uint8_t  unmappedChIndex;           // spec unmappedChannel     is the unmapped channel index for the current connection event
    uint8_t  lastUnmappedChIndex;       // spec lastUnmappedChannel shall be 0 for the first connection event of a connection
    uint8_t  channel;
    uint8_t  channelIndex;
    uint8_t                              currParam_winSize;
    uint16_t                             currParam_winOffset;
    LLCONNECTIONPARAMS_TypeDef           currParam;
    LLCONNECTIONUPDATE_TypeDef          *pNewUpdate;
    LLCONNECTIONPARAMS_TEMPLATE_TypeDef *pTemplateConnParam;
    LLCHANNELMAP_TypeDef                 currChM;
    LLCHANNELMAP_TypeDef                *pNewChM;
    
    LLENCRYPTION_TypeDef                 encrypt;
    
    uint16_t connMaxRxOctets;           // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t connMaxRxTime;             // Range 0x0148-0x4290    (328)~(17040)
    uint16_t connMaxTxOctets;           // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t connMaxTxTime;             // Range 0x0148-0x4290    (328)~(17040)
    uint16_t connRemoteMaxRxOctets;     //
    uint16_t connRemoteMaxRxTime;       //
    uint16_t connRemoteMaxTxOctets;     //
    uint16_t connRemoteMaxTxTime;       //
    uint16_t connEffectiveMaxRxOctets;  // the lesser of connMaxRxOctets and connRemoteMaxTxOctets
    uint16_t connEffectiveMaxRxTime;    //
    uint16_t connEffectiveMaxTxOctets;  // the lesser of connMaxTxOctets and connRemoteMaxRxOctets
    uint16_t connEffectiveMaxTxTime;    //
    
    uint8_t  invalid_crc_consecutive_times_within_ce;
    uint8_t  debug_consecutive_times_rxto;           //debug
    uint8_t  iSlave_ce1stRx_lost_rxto_consecutive_times;     // used at window widening

    BLETIMER_TypeDef ceAnchor_connectInd_get_F4h_F8h;   // reference for CE anchor
    BLETIMER_TypeDef ceAnchor_1stconnM2S_get_F4h_F8h;   // reference for CE anchor
    BLETIMER_TypeDef masAnchor;
    
    LLC_INITIATE_PROCEDURE_TypeDef llcInitiateProcedure;
    LLC_RESPOND_PROCEDURE_TypeDef  llcRespondProcedure;
    llcTxQ_TypeDef     llcTxQ_high;     // ring buffer queue
    llcTxQ_TypeDef     llcTxQ;          // ring buffer queue
    llcRxQ_TypeDef     llcRxQ;          // ring buffer queue
    lldataTxQ_TypeDef  lldataTxQ;       // ring buffer queue, q received hci packets, then send to air
    lldataRxQ_TypeDef  lldataRxQ;       // ring buffer queue, q received air packets, then send to host
    uint32_t HcAclDataBUFF2air_finished_count[2];
    uint32_t txw4ack ;                  //0x00:none
                                        //0x01:sending empty pdu: length=0, LLID=0b01:L2CAP Continuation
                                        //0x02:sending L2CAP, waiting ACK...
                                        //0x03:sending LLC  , waiting ACK...
    FLAG_0_CHANGED_AT_MAIN_TypeDef     flag0;
    FLAG_1_CHANGED_AT_IRQ1_TypeDef     flag1;
    uint16_t ADInfo_DID ;
    uint8_t  dtm_rx_aod_mode;
    uint8_t  dtm_tx_aoa_mode;
    uint8_t  cte_type;
    uint8_t  cte_length;
    uint8_t  cte_sample_count;
    uint8_t  cte_count;
    uint8_t  ext_adv_enable;		//bit[0] ext_adv_enable; bit[1] periodic_adv_enable; bit[2] connectionless_cte_enable
    
    uint8_t  mappedChannelIndex[4];
    BLETIMER_TypeDef ExtAdvEventAnchor;
    BLETIMER_TypeDef AuxAdvEventAnchor;
    BLETIMER_TypeDef PeriodicAdvEventAnchor;
} LEACL_TypeDef;

#define LEACL_conn_handle( pacl )                      ( pacl->aclConnHandle )
#define LEACL_is_role_master( pacl )                   ( pacl->role == 0x00 )  // 0x00:master, 0x01:slave
#define LEACL_is_role_slave( pacl )                    ( pacl->role == 0x01 )  // 0x00:master, 0x01:slave
#define LEACL_is_role_dtm_tx( pacl )                   ( pacl->role == 0x03 )  // 0x00:master, 0x01:slave  0x03:dtmTx
#define LEACL_is_role_dtm_rx( pacl )                   ( pacl->role == 0x04 )  // 0x00:master, 0x01:slave  0x04:dtmrx
#define LEACL_is_role_ext_adv_tx( pacl )               ( pacl->role == 0x05 )  // 0x00:master, 0x01:slave  0x05:ext_adv_tx

#define LEACL_is_remote_said_unknownType( pacl, op )   ((pacl->remote_said_unknownType[op >> 3] & (1U<<(op & 0x07))) != 0x00 )
static inline void LEACL_set_remote_said_unknownType(LEACL_TypeDef *pacl, uint8_t op)
{
    if (op < (6<<3)) { //maximum LL_CHANNEL_STATUS_IND 0x29 need 6 octets   uint8_t remote_said_unknownType[6];
        pacl->remote_said_unknownType[op >> 3] |= (1U<<(op & 0x07));
    }
}

#define REMOTE_VERS_INFO_is_valid(pacl)                                      ((pacl->pRemoteVersInfo != 0))
#define REMOTE_VERS_INFO_get_versNr(pacl)                                    ( REMOTE_VERS_INFO_is_valid(pacl) ? (pacl->pRemoteVersInfo)->versNr : 0x06 )
#define REMOTE_VERS_INFO_get_compId(pacl)                                    ( REMOTE_VERS_INFO_is_valid(pacl) ? (pacl->pRemoteVersInfo)->compId : 0x0000 )
#define REMOTE_VERS_INFO_get_subVersNr(pacl)                                 ( REMOTE_VERS_INFO_is_valid(pacl) ? (pacl->pRemoteVersInfo)->subVersNr : 0x0000 )

#define REMOTE_FEATURE_is_valid(pacl)                                                ((pacl->pRemoteFeature != 0))
#define REMOTE_FEATURE_get_featureSet(pacl, idx)                                     ( REMOTE_FEATURE_is_valid(pacl) ?    (pacl->pRemoteFeature)->featureSet[idx] : 0x00 )
#define REMOTE_FEATURE_is_supported_LE_encryption(pacl)                              ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x01) )  //bit 0  LE Encryption
#define REMOTE_FEATURE_is_supported_conn_params_request_procedure(pacl)              ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x02) )  //bit 1  Connection Parameters Request procedure  LL_0F_CONNECTION_PARAM_REQ,RSP
#define REMOTE_FEATURE_is_supported_extended_reject_ind(pacl)                        ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x04) )  //bit 2  Extended Reject Indication               LL_11_REJECT_EXT_IND
#define REMOTE_FEATURE_is_supported_slave_initiated_features_exchange(pacl)          ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x08) )  //bit 3  Peripheral-initiated Features Exchange   LL_0E_SLAVE_FEATURE_REQ
#define REMOTE_FEATURE_is_supported_LE_ping(pacl)                                    ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x10) )  //bit 4  LE Ping                                  LL_12_PING_REQ,RSP
#define REMOTE_FEATURE_is_supported_LE_data_packet_length_extension(pacl)            ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x20) )  //bit 5  LE Data Packet Length Extension          LL_14_LENGTH_REQ,RSP
#define REMOTE_FEATURE_is_supported_LE_privacy(pacl)                                 ( REMOTE_FEATURE_is_valid(pacl) &&  ((pacl->pRemoteFeature)->featureSet[0] & 0x40) )  //bit 6  LE Privacy

#define REMOTE_FEATURE_is_valid_UNsupported_LE_encryption(pacl)                      ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x01)==0x00) )  //bit 0  LE Encryption
#define REMOTE_FEATURE_is_valid_UNsupported_conn_params_request_procedure(pacl)      ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x02)==0x00) )  //bit 1  Connection Parameters Request procedure  LL_0F_CONNECTION_PARAM_REQ,RSP
#define REMOTE_FEATURE_is_valid_UNsupported_extended_reject_ind(pacl)                ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x04)==0x00) )  //bit 2  Extended Reject Indication               LL_11_REJECT_EXT_IND
#define REMOTE_FEATURE_is_valid_UNsupported_slave_initiated_features_exchange(pacl)  ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x08)==0x00) )  //bit 3  Peripheral-initiated Features Exchange   LL_0E_SLAVE_FEATURE_REQ
#define REMOTE_FEATURE_is_valid_UNsupported_LE_ping(pacl)                            ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x10)==0x00) )  //bit 4  LE Ping                                  LL_12_PING_REQ,RSP
#define REMOTE_FEATURE_is_valid_UNsupported_LE_data_packet_length_extension(pacl)    ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x20)==0x00) )  //bit 5  LE Data Packet Length Extension          LL_14_LENGTH_REQ,RSP
#define REMOTE_FEATURE_is_valid_UNsupported_LE_privacy(pacl)                         ( REMOTE_FEATURE_is_valid(pacl) && (((pacl->pRemoteFeature)->featureSet[0] & 0x40)==0x00) )  //bit 6  LE Privacy


/* Exported constants --------------------------------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------------------------------------*/
extern dllist_TypeDef           leACLlist;

/* Exported functions --------------------------------------------------------------------------------------*/
LEACL_TypeDef * leacl_alloc          (void);
           void leacl_delete         (LEACL_TypeDef *);
LEACL_TypeDef * leacl_with_peerAddress      (uint8_t *pPeerAddress);
LEACL_TypeDef * leacl_with_connHandle       (uint16_t handle);
LEACL_TypeDef * ur0isr_leacl_with_connHandle(uint16_t handle);


#endif /* __LEACL_H ------------------------------------------------------------------------------------*/
