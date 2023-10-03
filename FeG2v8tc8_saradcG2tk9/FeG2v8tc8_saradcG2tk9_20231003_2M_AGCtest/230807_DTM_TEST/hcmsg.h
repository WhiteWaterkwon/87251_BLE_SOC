/*********************************************************************************************************//**
 * @file    hcmsg.h
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
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
 * <h2><center>Copyright (C) 2021 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef HT_HCMSG_H
#define HT_HCMSG_H

/* Includes ------------------------------------------------------------------------------------------------*/

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"


/* Exported constants --------------------------------------------------------------------------------------*/


/* Exported types ------------------------------------------------------------------------------------------*/


/////////////////////////////////////////////////////



/* Exported macro ------------------------------------------------------------------------------------------*/
typedef enum {
    HCMSG_SEND_HCIEVT = 0x0001,
    HCMSG_DELETE_LEACL_FREE_MEMORY,
    HCMSG_CHECK_CONN_EFFECTIVE_MAX_TRX_OCTETS_TIME_CHANGE,
} msgcodeHCMSG_t;

typedef struct //__attribute__((packed))
{
    htQueue_node_TypeDef node;
    uint32_t          msgcode;
    uint32_t          LengthExtramsgparam;  //length of extra msgparam[] appended after msgparam+4
    uint8_t           msgparam[4];

} hcmsgQ_node_TypeDef ;


typedef enum {
    LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE = 0x0001,         //send LL_00_CONNECTION_UPDATE_IND
    LLCMSG_INITIATE_LLC_CHANNEL_MAP_UPDATE_PROCEDURE,           //send LL_01_CHANNEL_MAP_IND
    LLCMSG_INITIATE_LLC_ACL_TERMINATION_PROCEDURE,              //send LL_02_TERMINATE_IND
    LLCMSG_INITIATE_LLC_START_ENCRYPTION_PROCEDURE,             //send LL_03_ENC_REQ
    LLCMSG_INITIATE_LLC_PAUSE_ENCRYPTION_PROCEDURE,             //send LL_0A_PAUSE_ENC_REQ
    LLCMSG_INITIATE_LLC_FEATURE_EXCHANGE_PROCEDURE,             //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
    LLCMSG_INITIATE_LLC_VERSION_EXCHANGE_PROCEDURE,             //send LL_0C_VERSION_IND, wait LL_0C_VERSION_IND
    LLCMSG_INITIATE_LLC_CONN_PARAMS_REQUEST_PROCEDURE,          //send LL_0F_CONNECTION_PARAM_REQ
    LLCMSG_INITIATE_LLC_LE_PING_PROCEDURE,                      //send LL_12_PING_REQ, wait LL_13_PING_RSP
    LLCMSG_INITIATE_LLC_DATA_LENGTH_UPDATE_PROCEDURE,           //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP
    LLCMSG_INITIATE_LLC_PHY_UPDATE_PROCEDURE,                   //                                          spec 5.0
    LLCMSG_INITIATE_LLC_MIN_NUMBER_OF_USED_CHANNELS_PROCEDURE,  //send LL_19_MIN_USED_CHANNELS_IND          spec 5.0
} msgcodeLLCMSG_t;

typedef struct //__attribute__((packed))
{
    htQueue_node_TypeDef node;
    uint32_t          msgcode;
    uint32_t          LengthExtramsgparam;  //length of extra msgparam[] appended after msgparam+4
    uint8_t           msgparam[4];

} llcmsgQ_node_TypeDef ;

/* Exported functions --------------------------------------------------------------------------------------*/
bool hcmsgQ_push( msgcodeHCMSG_t msgCode, 
                  uint32_t msgparam3210,         //msgparam[0],[1],[2],[3]
                  uint32_t LengthExtramsgparam,  //length of extra msgparam[] appended after msgparam+4
                  uint8_t *pExtramsgparam        //          extra msgparam[] appended after msgparam+4
                );
bool llcmsgQ_push(msgcodeLLCMSG_t msgCode, 
                  uint32_t msgparam3210,         //msgparam[0],[1],[2],[3]
                  uint32_t LengthExtramsgparam,  //length of extra msgparam[] appended after msgparam+4
                  uint8_t *pExtramsgparam        //          extra msgparam[] appended after msgparam+4
                );
void process_hcmsgQ(void);

bool proc_hcmsg_send_hcievent                                   (hcmsgQ_node_TypeDef *);
bool proc_hcmsg_delete_leacl_free_memory                        (hcmsgQ_node_TypeDef *);
bool proc_hcmsg_check_conn_effective_max_trx_octets_time_change (hcmsgQ_node_TypeDef *);

bool proc_llcmsg_initiate_llc_conn_update_procedure             (llcmsgQ_node_TypeDef *); //send LL_00_CONNECTION_UPDATE_IND
bool proc_llcmsg_initiate_llc_channel_map_update_procedure      (llcmsgQ_node_TypeDef *); //send LL_01_CHANNEL_MAP_IND
bool proc_llcmsg_initiate_llc_acl_termination_procedure         (llcmsgQ_node_TypeDef *); //send LL_02_TERMINATE_IND
bool proc_llcmsg_initiate_llc_encryption_start_procedure        (llcmsgQ_node_TypeDef *); //send LL_03_ENC_REQ
bool proc_llcmsg_initiate_llc_encryption_pause_procedure        (llcmsgQ_node_TypeDef *); //send LL_0A_PAUSE_ENC_REQ
bool proc_llcmsg_initiate_llc_feature_exchange_procedure        (llcmsgQ_node_TypeDef *); //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
bool proc_llcmsg_initiate_llc_version_exchange_procedure        (llcmsgQ_node_TypeDef *); //send LL_0C_VERSION_IND, wait LL_0C_VERSION_IND
bool proc_llcmsg_initiate_llc_conn_params_request_procedure     (llcmsgQ_node_TypeDef *); //send LL_0F_CONNECTION_PARAM_REQ
bool proc_llcmsg_initiate_llc_le_ping_procedure                 (llcmsgQ_node_TypeDef *); //send LL_12_PING_REQ, wait LL_LPING_RSP
bool proc_llcmsg_initiate_llc_data_length_update_procedure      (llcmsgQ_node_TypeDef *); //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP

#endif /* HT_HCMSG_H */
