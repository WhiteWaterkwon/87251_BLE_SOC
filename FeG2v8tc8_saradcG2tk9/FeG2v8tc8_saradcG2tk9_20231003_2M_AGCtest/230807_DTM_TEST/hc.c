/*********************************************************************************************************//**
 * @file    hc.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
 * @brief   This file provides all BC5602B HOST functions.
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
#include <stdio.h>                           //printf  debug
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcmp, memcpy

#include "bc5602b_host.h"   //ht_memory_copy    ht_write_hword
#include "pdma.h"                            //pdma_ram_bleRx_read_payload()
#include "ble_soc.h"        //RF_WT08        debug_RFLA_pulse_to_trigger_LA
#include "usart.h"
#include "leconfig.h"
#include "leacl.h"          //LEACL_TypeDef
#include "llc.h"
#include "lldata.h"         //hc_leacl_data_buffer_toair_TypeDef
#include "aes128.h"                          //aes128_encrypt___lsbyte_to_msbyte  #if AESCCM_LL_SIGNALING == 1
#include "hcmsg.h"
#include "hc.h"

/* Private define ------------------------------------------------------------------------------------------*/
static inline void debug_RFLA_pulse_to_trigger_LA(void)
{
    RF_WT08(0x100, 0x1);
}

hcievt_0e_cmd_complete_leTestEnd_TypeDef leTestEnd_evt =
    {
        .event_code_0e      = 0x0E,  //HCI_CMD_COMPLETE
        .param_Total_Length = sizeof(leTestEnd_evt)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_201f    = 0x201f,//(OGF_LE<<10)|OCF_LE_TEST_END,//opcode 0x201f
        .status             = ERR_00_SUCCESS,//status succeed
        .num_packets        = 0x0000 //Number_Of_Packets:      Number_Of_Packets for a transmitter test shall be reported as 0x0000
    };
debug_hcievt_0e_command_complete_leTestEnd_debug_TypeDef debug_leTestEnd_evt =
    {
        .event_code_0e      = 0x0E,  //HCI_CMD_COMPLETE
        .param_Total_Length = sizeof(leTestEnd_evt)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_201f    = 0x201f,//(OGF_LE<<10)|OCF_LE_TEST_END,//opcode 0x201f
        .status             = ERR_00_SUCCESS,//status succeed
        .num_packets        = 0x0000, //Number_Of_Packets:      Number_Of_Packets for a transmitter test shall be reported as 0x0000        
    	.num_crcf	    = 0x0000,
    	.num_synclost	    = 0x0000
    };     
    
/* Private types -------------------------------------------------------------------------------------------*/
typedef bool (*rxhcicmdHandle_func_pointer_TypeDef)(HCI_01_COMMAND_PACKET_TypeDef *);

typedef struct //__attribute__((packed))
{
    uint8_t                     octet[255+3];   // max hc_cmd param len 255
    uint8_t                     hcmdstate ;     // 0x00:free, 0x01:booked

} hc_command_buffer_TypeDef ;
hc_command_buffer_TypeDef HcCommandBUFF[1];


/* Private variables ---------------------------------------------------------------------------------------*/
uint8_t phyTx_pdudata ;
uint8_t debug_wantedpower = 0x08;
const uint8_t ChannelIndex_table[40]={37,0,1,2,3,4,5,6,7,8,9,10,38,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,39};

const uint8_t pn9_seq[128]= { // pn9 cycle 511 bits, here repeated 2 times, in case large packet length
    0xFF, 0xC1, 0xFB, 0xE8, 0x4C, 0x90, 0x72, 0x8B, 0xE7, 0xB3, 0x51, 0x89, 0x63, 0xAB, 0x23, 0x23, 
    0x02, 0x84, 0x18, 0x72, 0xAA, 0x61, 0x2F, 0x3B, 0x51, 0xA8, 0xE5, 0x37, 0x49, 0xFB, 0xC9, 0xCA, 
    0x0C, 0x18, 0x53, 0x2C, 0xFD, 0x45, 0xE3, 0x9A, 0xE6, 0xF1, 0x5D, 0xB0, 0xB6, 0x1B, 0xB4, 0xBE, 
    0x2A, 0x50, 0xEA, 0xE9, 0x0E, 0x9C, 0x4B, 0x5E, 0x57, 0x24, 0xCC, 0xA1, 0xB7, 0x59, 0xB8, 0x87, 
    0xFF, 0xE0, 0x7D, 0x74, 0x26, 0x48, 0xB9, 0xC5, 0xF3, 0xD9, 0xA8, 0xC4, 0xB1, 0xD5, 0x91, 0x11, 
    0x01, 0x42, 0x0C, 0x39, 0xD5, 0xB0, 0x97, 0x9D, 0x28, 0xD4, 0xF2, 0x9B, 0xA4, 0xFD, 0x64, 0x65, 
    0x06, 0x8C, 0x29, 0x96, 0xFE, 0xA2, 0x71, 0x4D, 0xF3, 0xF8, 0x2E, 0x58, 0xDB, 0x0D, 0x5A, 0x5F, 
    0x15, 0x28, 0xF5, 0x74, 0x07, 0xCE, 0x25, 0xAF, 0x2B, 0x12, 0xE6, 0xD0, 0xDB, 0x2C, 0xDC, 0xC3
};
const uint8_t pn15_seq[128]= { // pn15 cycle 32767 bits, we only list the first 128*8 bits
    0xFF, 0x7F, 0x00, 0x20, 0x00, 0x18, 0x00, 0x0A, 0x80, 0x07, 0x20, 0x02, 0x98, 0x01, 0xAA, 0x80, 
    0x7F, 0x20, 0x20, 0x18, 0x18, 0x0A, 0x8A, 0x87, 0x27, 0x22, 0x9A, 0x99, 0xAB, 0x2A, 0xFF, 0x5F, 
    0x00, 0x38, 0x00, 0x12, 0x80, 0x0D, 0xA0, 0x05, 0xB8, 0x03, 0x32, 0x81, 0xD5, 0xA0, 0x5F, 0x38, 
    0x38, 0x12, 0x92, 0x8D, 0xAD, 0xA5, 0xBD, 0xBB, 0x31, 0xB3, 0x54, 0x75, 0xFF, 0x67, 0x00, 0x2A, 
    0x80, 0x1F, 0x20, 0x08, 0x18, 0x06, 0x8A, 0x82, 0xE7, 0x21, 0x8A, 0x98, 0x67, 0x2A, 0xAA, 0x9F, 
    0x3F, 0x28, 0x10, 0x1E, 0x8C, 0x08, 0x65, 0xC6, 0xAB, 0x12, 0xFF, 0x4D, 0x80, 0x35, 0xA0, 0x17, 
    0x38, 0x0E, 0x92, 0x84, 0x6D, 0xA3, 0x6D, 0xB9, 0xED, 0xB2, 0xCD, 0xB5, 0x95, 0xB7, 0x2F, 0x36, 
    0x9C, 0x16, 0xE9, 0xCE, 0xCE, 0xD4, 0x54, 0x5F, 0x7F, 0x78, 0x20, 0x22, 0x98, 0x19, 0xAA, 0x8A
};

/* Global functions ----------------------------------------------------------------------------------------*/
void init_0rf_2in1_5602B_set_rfch(uint8_t ch); //5602B RF

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
bool send_04_hciEventPacket(HCI_04_EVENT_PACKET_TypeDef *pEvent)
{
  /*
    {
        uint16_t i;
            uart_putchar(0x04);//                       // 0x04 HCI_PTYPE_04_HCI_EVENT
            uart_putchar(pEvent->event_code);           // Event Code
            uart_putchar(pEvent->param_Total_Length);   // Parameter Total Length
        for(i=0; i<pEvent->param_Total_Length; i++)
        {
            uart_putchar(pEvent->params[i]);            // Event Parameter[]
        }
        return (1);
    }
  */
    {
        uint16_t length;
        length = pEvent->param_Total_Length + 2; // ->event_code   ->param_Total_Length
        return hcmsgQ_push( HCMSG_SEND_HCIEVT, 
                            0x0000, //param3210
                            length, 
                            (uint8_t *)pEvent
                          );
    }
  //
}

bool proc_hcmsg_send_hcievent(hcmsgQ_node_TypeDef *qNode)
{
//  #if _UART_PUT_DEBUG_ == 1
    /*
    {
        uint16_t i;
            uart_puts("hci event, ");
        for(i=0; i<qNode->LengthExtramsgparam; i++)
        {
            uart_putu8(qNode->msgparam[4+i]);         // 4+i: uint8_t msgparam[4]
            uart_puts(" ");
        }
            uart_putchar_n('\n');
    }
    */
//  #else
        uint16_t i;
            uart_putchar(0x04);//                       // 0x04 HCI_PTYPE_04_HCI_EVENT
        for(i=0; i<qNode->LengthExtramsgparam; i++)
        {
            uart_putchar(qNode->msgparam[4+i]);         // 4+i: uint8_t msgparam[4]
        }
//  #endif
	debug_RFLA_pulse_to_trigger_LA();
        return 1;
}

bool send_04_hciEvent_05_disconnection_complete(uint16_t connHandle, uint8_t reason)
{
    hcievt_05_disconnection_complete_TypeDef evt05 =
    {
        .event_code_05      = 0x05,//HCIEVT_05_DISCONN_COMPLETE
        .param_Total_Length = sizeof(evt05)-2,
        .status             = 0x00,//ERR_00_SUCCESS
      //.conn_handle        = connHandle,
      //.reason             = reason
    };
        ht_write_hword( (uint8_t *)&(evt05.conn_handle), connHandle );
        ht_write_byte ( (uint8_t *)&(evt05.reason),      reason );
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt05);
}

bool send_04_hciEvent_08_encryption_change(uint16_t connHandle, uint8_t status, uint8_t encryption_enabled)
{
    hcievt_08_encryption_change_TypeDef evt08 =
    {
        .event_code_08      = 0x08,//HCIEVT_08_ENCRYPTION_CHANGE
        .param_Total_Length = sizeof(evt08)-2,
      //.status             = status,//ERR_00_SUCCESS
      //.conn_handle        = connHandle,
      //.encryption_enabled = encryption_enabled
    };
        ht_write_byte ( (uint8_t *)&(evt08.status),             status );
        ht_write_hword( (uint8_t *)&(evt08.conn_handle),        connHandle );
        ht_write_byte ( (uint8_t *)&(evt08.encryption_enabled), encryption_enabled );
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt08);
}

bool send_04_hciEvent_13_number_of_completed_packets(uint16_t connHandle, uint16_t num_completed_packets)
{
    hcievt_13_number_of_completed_packets_TypeDef evt13 =
    {
        .event_code_13      = 0x13,//HCIEVT_13_NUMBER_OF_COMPLETED_PACKETS
        .param_Total_Length = sizeof(evt13)-2,
        .num_handles_01     = 0x01,
      //.conn_handle        = connHandle,
      //.num_completed_pkts = num_completed_packets,
    };
        ht_write_hword( (uint8_t *)&(evt13.conn_handle),        connHandle );
        ht_write_hword( (uint8_t *)&(evt13.num_completed_pkts), num_completed_packets );
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt13);
}

bool send_04_hciEvent_30_encryption_key_refresh_complete(uint16_t connHandle, uint8_t status)
{
    hcievt_30_encryption_key_refresh_complete_TypeDef evt30 =
    {
        .event_code_30      = 0x30,//HCIEVT_30_ENCRYPTION_KEY_REFRESH_COMPLETE
        .param_Total_Length = sizeof(evt30)-2,
      //.status             = status,//ERR_00_SUCCESS
      //.conn_handle        = connHandle,
    };
        ht_write_byte ( (uint8_t *)&(evt30.status),             status );
        ht_write_hword( (uint8_t *)&(evt30.conn_handle),        connHandle );
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt30);
}

bool send_04_hciEvent_57_authenticated_payload_timeout_expired(uint16_t connHandle)
{
    hcievt_57_authenticated_payload_timeout_expired_TypeDef evt57 =
    {
        .event_code_57      = 0x57,//HCIEVT_57_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED
        .param_Total_Length = sizeof(evt57)-2,
      //.conn_handle        = connHandle,
    };
        ht_write_hword( (uint8_t *)&(evt57.conn_handle),        connHandle );
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt57);
}

bool send_04_hciEvent_0c_read_remote_version_info_complete(LEACL_TypeDef *pacl, uint8_t reason)
{
    hcievt_0C_read_remote_version_info_complete_TypeDef evt0c =
    {
        .event_code_0C      = 0x0C,//HCIEVT_0C_READ_REMOTE_VER_INFO_COMPLETE
        .param_Total_Length = sizeof(evt0c)-2,
      //.status             = reason,
      //.conn_handle        = LEACL_conn_handle(pacl),
      //.version            =(pacl->pRemoteVersInfo)->versNr,
      //.manufacturer_name  =(pacl->pRemoteVersInfo)->compId,
      //.subversion         =(pacl->pRemoteVersInfo)->subVersNr
    };
        ht_write_byte ( (uint8_t *)&(evt0c.status),            reason );
        ht_write_hword( (uint8_t *)&(evt0c.conn_handle),       LEACL_conn_handle(pacl) );
    if (pacl->pRemoteVersInfo) {
        ht_write_byte ( (uint8_t *)&(evt0c.version),           (pacl->pRemoteVersInfo)->versNr );
        ht_write_hword( (uint8_t *)&(evt0c.manufacturer_name), (pacl->pRemoteVersInfo)->compId );
        ht_write_hword( (uint8_t *)&(evt0c.subversion),        (pacl->pRemoteVersInfo)->subVersNr );
    }
    if( leconfig_eventMask.event_mask[1]&0x08 ) { //bit-11 Read Remote Version Information Complete Event
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0c);
    }
    else {
        //be masked
        return 1;
    }
}

static bool send_04_hciEvent_0e_command_complete(uint16_t cmd_opcode, uint8_t status)
{
    hcievt_0e_cmd_complete_TypeDef evt0e =
    {
        .event_code_0e      = 0x0E,//HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e)-2,
        .num_hci_cmd_pkts   = 0x01,//allowed cmd packets
      //.cmd_opcode         = cmd_opcode,
      //.status             = status
    };
        ht_write_hword( (uint8_t *)&(evt0e.cmd_opcode), cmd_opcode );
        ht_write_byte ( (uint8_t *)&(evt0e.status),     status );
        //cannot be masked, event always occurs
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e);
}

static bool send_04_hciEvent_0f_command_status(uint16_t cmd_opcode, uint8_t status)
{
    hcievt_0f_cmd_status_TypeDef evt0f =
    {
        .event_code_0f      = 0x0F,//HCIEVT_0F_CMD_STATUS
        .param_Total_Length = sizeof(evt0f)-2,
      //.status             = status,
        .num_hci_cmd_pkts   = 0x01,//allowed cmd packets
      //.cmd_opcode         = cmd_opcode
    };
        ht_write_byte ( (uint8_t *)&(evt0f.status),     status );
        ht_write_hword( (uint8_t *)&(evt0f.cmd_opcode), cmd_opcode );
        //cannot be masked, event always occurs
        return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0f);
}

bool send_04_hciEvent_3e_01_LE_connection_complete(LEACL_TypeDef *pacl)
{
    hcievt_3e_01_LE_connection_complete_TypeDef evt3e01 =
    {
        .event_code_3e         = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length    = sizeof(evt3e01)-2,
        .subevent_code_01      = 0x01,//SUBEVT_01_LE_CONN_COMPLETE
        .status                = 0x00,//ERR_00_SUCCESS  0x00:successfully completed,   0x01~0xFF:error codes
      //.conn_handle           = LEACL_conn_handle(pacl),
      //.role                  = pacl->role,
        .peer_addr_type        = 0x00,
      //.peer_address
      //.conn_interval         = pacl->currParam.interval,
      //.slave_latency         = pacl->currParam.latency,
      //.supervision_timeout   = pacl->currParam.timeout,
      //.master_clock_accuracy = pacl->sca
    };
        ht_write_hword( (uint8_t *)&(evt3e01.conn_handle),           LEACL_conn_handle(pacl) );
        ht_write_byte ( (uint8_t *)&(evt3e01.role),                  ht_read_byte ((uint8_t *)&pacl->role               ) );
                      ht_memory_copy(evt3e01.peer_address+0,         pacl->remoteDeviceAddr+0, 6);
        ht_write_hword( (uint8_t *)&(evt3e01.conn_interval),         ht_read_hword((uint8_t *)&pacl->currParam.interval ) );
        ht_write_hword( (uint8_t *)&(evt3e01.slave_latency),         ht_read_hword((uint8_t *)&pacl->currParam.latency  ) );
        ht_write_hword( (uint8_t *)&(evt3e01.supervision_timeout),   ht_read_hword((uint8_t *)&pacl->currParam.timeout  ) );
        ht_write_byte ( (uint8_t *)&(evt3e01.master_clock_accuracy), pacl->sca );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e01);
}

bool send_04_hciEvent_3e_03_LE_conn_update_complete(LEACL_TypeDef *pacl, uint8_t reason)
{
    hcievt_3e_03_LE_conn_update_complete_TypeDef evt3e03 =
    {
        .event_code_3e       = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length  = sizeof(evt3e03)-2,
        .subevent_code_03    = 0x03,//SUBEVT_03_LE_CONN_UPDATE_COMPLETE
        .status              = reason,//0x00:successfully completed,   0x01~0xFF:error codes
      //.conn_handle         = LEACL_conn_handle(pacl),
      //.conn_interval       = pacl->currParam.interval,
      //.slave_latency       = pacl->currParam.latency,
      //.supervision_timeout = pacl->currParam.timeout
    };
        ht_write_hword( (uint8_t *)&(evt3e03.conn_handle),         LEACL_conn_handle(pacl) );
        ht_write_hword( (uint8_t *)&(evt3e03.conn_interval),       pacl->currParam.interval );
        ht_write_hword( (uint8_t *)&(evt3e03.slave_latency),       pacl->currParam.latency );
        ht_write_hword( (uint8_t *)&(evt3e03.supervision_timeout), pacl->currParam.timeout );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e03);
}

bool send_04_hciEvent_3e_04_LE_read_remote_feature_complete(LEACL_TypeDef *pacl, uint8_t reason)
{
    uint8_t i;
    hcievt_3e_04_LE_read_remote_feature_complete_TypeDef evt3e04 =
    {
        .event_code_3e      = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length = sizeof(evt3e04)-2,
        .subevent_code_04   = 0x04,//SUBEVT_04_LE_READ_REMOTE_FEATURES
        .status             = reason,//0x00:successfully completed,   0x01~0xFF:error codes
      //.conn_handle        = LEACL_conn_handle(pacl)
      //.LE_features+0
    };
        ht_write_hword( (uint8_t *)&(evt3e04.conn_handle), LEACL_conn_handle(pacl) );
    for (i=0; i<8; i++) {
        evt3e04.LE_features[i] = REMOTE_FEATURE_get_featureSet(pacl, i);
    }
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e04);
}

bool send_04_hciEvent_3e_05_LE_LTK_request(LEACL_TypeDef *pacl)
{
    hcievt_3e_05_LE_LTK_request_TypeDef evt3e05 =
    {
        .event_code_3e      = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length = sizeof(evt3e05)-2,
        .subevent_code_05   = 0x05,//SUBEVT_05_LE_LONG_TERM_KEY_REQUEST
      //.conn_handle        = LEACL_conn_handle(pacl)
      //.random_number+0
      //.encrypted_diversifier+0
    };
        ht_write_hword( (uint8_t *)&(evt3e05.conn_handle), LEACL_conn_handle(pacl) );
        ht_memory_copy( evt3e05.random_number+0,         pacl->encrypt.rand+0, 8 ); //Random_Number:         Size: 8 octets
        ht_memory_copy( evt3e05.encrypted_diversifier+0, pacl->encrypt.ediv+0, 2 ); //Encrypted_Diversifier: Size: 2 octets
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e05);
}

bool send_04_hciEvent_3e_06_LE_remote_conn_param_request(LEACL_TypeDef *pacl)
{
    hcievt_3e_06_LE_remote_conn_param_request_TypeDef evt3e06 =
    {
        .event_code_3e      = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length = sizeof(evt3e06)-2,
        .subevent_code_06   = 0x06,//SUBEVT_06_LE_REMOTE_CONN_PARAM_REQUEST
      //.conn_handle        = LEACL_conn_handle(pacl),
      //.interval_min       = pacl->pTemplateConnParam->minInterval,
      //.interval_max       = pacl->pTemplateConnParam->maxInterval,
      //.latency            = pacl->pTemplateConnParam->latency,
      //.timeout            = pacl->pTemplateConnParam->timeout
    };
        ht_write_hword( (uint8_t *)&(evt3e06.conn_handle ), LEACL_conn_handle(pacl) );
    if (pacl->pTemplateConnParam != 0) {
      /*
                                     evt3e06.interval_min = pacl->pTemplateConnParam->minInterval;
                                     evt3e06.interval_max = pacl->pTemplateConnParam->maxInterval;
                                     evt3e06.latency      = pacl->pTemplateConnParam->latency;
                                     evt3e06.timeout      = pacl->pTemplateConnParam->timeout;
      */
        ht_write_hword( (uint8_t *)&(evt3e06.interval_min), pacl->pTemplateConnParam->minInterval );
        ht_write_hword( (uint8_t *)&(evt3e06.interval_max), pacl->pTemplateConnParam->maxInterval );
        ht_write_hword( (uint8_t *)&(evt3e06.latency     ), pacl->pTemplateConnParam->latency );
        ht_write_hword( (uint8_t *)&(evt3e06.timeout     ), pacl->pTemplateConnParam->timeout );
      //
    }
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e06);
}

bool send_04_hciEvent_3e_07_LE_data_length_change(LEACL_TypeDef *pacl,
                                                  uint16_t connEffectiveMaxTxOctets,
                                                  uint16_t connEffectiveMaxTxTime,
                                                  uint16_t connEffectiveMaxRxOctets,
                                                  uint16_t connEffectiveMaxRxTime
                                                 )
    //C8 Mandatory if LE Feature (LE Data Packet Length Extension) is supported, otherwise optional
{
    hcievt_3e_07_LE_data_length_change_TypeDef evt3e07 =
    {
        .event_code_3e            = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length       = sizeof(evt3e07)-2,
        .subevent_code_07         = 0x07,//SUBEVT_07_LE_DATA_LENGTH_CHANGE
      //.conn_handle              = LEACL_conn_handle(pacl),
      //.connEffectiveMaxTxOctets = connEffectiveMaxTxOctets,
      //.connEffectiveMaxTxTime   = connEffectiveMaxTxTime,
      //.connEffectiveMaxRxOctets = connEffectiveMaxRxOctets,
      //.connEffectiveMaxRxTime   = connEffectiveMaxRxTime
    };
        ht_write_hword( (uint8_t *)&(evt3e07.conn_handle             ), LEACL_conn_handle(pacl)            );
        ht_write_hword( (uint8_t *)&(evt3e07.connEffectiveMaxTxOctets), connEffectiveMaxTxOctets );
        ht_write_hword( (uint8_t *)&(evt3e07.connEffectiveMaxTxTime  ), connEffectiveMaxTxTime   );
        ht_write_hword( (uint8_t *)&(evt3e07.connEffectiveMaxRxOctets), connEffectiveMaxRxOctets );
        ht_write_hword( (uint8_t *)&(evt3e07.connEffectiveMaxRxTime  ), connEffectiveMaxRxTime   );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e07);
}

bool send_04_hciEvent_3e_13_LE_scan_request_received(void)
{
    hcievt_3e_13_LE_scan_request_received_TypeDef evt3e13 =
    {
        .event_code_3e      = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length = sizeof(evt3e13)-2,
        .subevent_code_13   = 0x13,//SUBEVT_13_LE_SCAN_REQUEST_RECEIVED
      //.adv_handle         =         // todo
      //.scanner_addr_type  =         // todo
      //.scanner_address[]            // todo
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3e13);
}

bool send_04_hciEvent_3e_15_LE_connectionless_iq_report(uint8_t channel_index, uint8_t cte_type,uint8_t sample_count,unsigned char *iq_sample_value)
{
    int i;
    hcievt_3e_15_LE_connectionless_iq_report_TypeDef evt =
    {
        .event_code_3e      = 0x3E,//HCI_LE_META_EVENT_3E
        .param_Total_Length = 13+(sample_count*2),
        .subevent_code_15   = 0x15,//HCI_LE_CONNECTIONLESS_IQ_REPORT
      	.sync_handle        = 0x0FFF,       
      	.channel_index	    = channel_index,
      	.rssi		    = 0x0080, //todo
        .rssi_antenna_id    = 0x01,
        .cte_type           = cte_type,
        .slot_durations     = cte_type,
        .packet_status      = 0x00,
        .periodic_event_counter = 0x0000,
        .sample_count	    =	 sample_count
    };
    
    for(i=0;i<(sample_count*2);i++)
    {
    	evt.iq_sample[i]= iq_sample_value[i];
    }
	
	/*
     for(i=0;i<(sample_count);i++)
    {
    	evt.iq_sample[i]= 0xC5;
    }
     for(i=sample_count;i<(sample_count*2);i++)
    {
    	evt.iq_sample[i]= 0x00;
    }*/
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt);
}

bool send_04_hciEvent_3e_ff_LE_kidd_debug(uint8_t length, uint8_t *pParam)
{
    uint8_t *dst;
    
    hcievt_3e_ff_LE_kidd_debug_TypeDef evt3eff =
    {
        .event_code_3e      = 0x3E,//HCIEVT_3E_LE_META_EVENT
        .param_Total_Length = 1+6+length, //sizeof(evt3eff)-2,
        .subevent_code_ff   = 0xFF,//SUBEVT_FF_LE_KIDD_DEBUG
      //.param              =      //
    };
    
          evt3eff.param[0] = 0xFF;
          evt3eff.param[1] = 0xFF;
          evt3eff.param[2] = 0xFF;
          evt3eff.param[3] = 0xFF;
          evt3eff.param[4] = 0xFF;
          evt3eff.param[5] = 0xFF;
    dst = evt3eff.param+6;
    while (length) {
        ht_write_byte( dst, *pParam);
        dst ++; pParam ++;
        length --;
    }
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt3eff);
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
bool hcmd_0406_disconnect(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_0406_disconnect_TypeDef *pCmd0406;
  //pCmd0406 = (hcicmd_0406_disconnect_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
  //uint8_t  reason;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
  //reason = ht_read_byte ( pCmd->params+2 ); //           Reason: Size: 1 octet
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x0406, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    else
    {
            //
            done = TxLLcPDU_02_terminate_ind(pacl, 0x13); //ERR_13_OTHER_END_TERMINATED_CONN_USER_END
            /*
            done = llcmsgQ_push( LLCMSG_INITIATE_LLC_ACL_TERMINATION_PROCEDURE, 
                                LEACL_conn_handle(pacl) | (((uint32_t)0x13)<<16), //param[2] ERR_13_OTHER_END_TERMINATED_CONN_USER_END
                                0,//length
                                (uint8_t *)0
                               );
            */
        if (done) {
               send_04_hciEvent_0f_command_status(0x0406, 0x00); //ERR_00_SUCCESS
        }
        return (done);
    }
}

bool proc_hcmsg_delete_leacl_free_memory(hcmsgQ_node_TypeDef *qNode)
{
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword(qNode->msgparam+0); //param[0]~[1]
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return 1;
    }
    else
    {
        leacl_delete(pacl);
        return 1;
    }
}

bool hcmd_041d_read_remote_version_information(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_041d_read_remote_version_info_TypeDef *pCmd041d;
  //pCmd041d = (hcicmd_041d_read_remote_version_info_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
            return send_04_hciEvent_0f_command_status(0x041d, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    else
    {
            done = send_04_hciEvent_0f_command_status(0x041d, 0x00); //ERR_00_SUCCESS
        if (done) {
            if (REMOTE_VERS_INFO_is_valid(pacl)) {
                send_04_hciEvent_0c_read_remote_version_info_complete(pacl, 0x00); //ERR_00_SUCCESS
            }
            else {
                llcmsgQ_push( LLCMSG_INITIATE_LLC_VERSION_EXCHANGE_PROCEDURE, 
                             LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                             0,//length
                             (uint8_t *)0
                           );
            }
        }
            return (done);
    }
}

bool hcmd_0c03_reset(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    return send_04_hciEvent_0e_command_complete(0x0c03, 0x00); //ERR_00_SUCCESS
}

bool hcmd_0c7b_read_authenticated_payload_timeout(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_0c7b_read_authenticated_payload_timeout_TypeDef *pCmd0c7b = (hcicmd_0c7b_read_authenticated_payload_timeout_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x0c7b, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    hcievt_0e_cmd_complete_leReadAuthenticatedPayloadTO_TypeDef evt0e0c7b =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e0c7b)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_0c7b    = 0x0c7b,//(OGF_03_HC_N_BASEBAND<<10)|OCF_R_AUTHENTICATED_PAYLOAD_TIMEOUT,//opcode 0x0c7b
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = LEACL_conn_handle(pacl),
      //.authenticatedPayloadTO = leconfig_authenticatedPayloadTimeout.authenticatedPayloadTO,
    };
        ht_write_hword( (uint8_t *)&(evt0e0c7b.conn_handle             ), handle );
        ht_write_hword( (uint8_t *)&(evt0e0c7b.authenticatedPayloadTO  ), leconfig_authenticatedPayloadTimeout.authenticatedPayloadTO );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e0c7b);
}

bool hcmd_0c7c_write_authenticated_payload_timeout(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_0c7c_write_authenticated_payload_timeout_TypeDef *pCmd0c7c = (hcicmd_0c7c_write_authenticated_payload_timeout_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x0c7c, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    hcievt_0e_cmd_complete_leWriteAuthenticatedPayloadTO_TypeDef evt0e0c7c =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e0c7c)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_0c7c    = 0x0c7c,//(OGF_03_HC_N_BASEBAND<<10)|OCF_W_AUTHENTICATED_PAYLOAD_TIMEOUT,//opcode 0x0c7c
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = LEACL_conn_handle(pacl),
    };
        ht_write_hword( (uint8_t *)&(evt0e0c7c.conn_handle  ), handle );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e0c7c);
}

bool hcmd_1001_read_local_version_info(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_readLocalVersion_TypeDef evt0e1001 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e1001)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_1001    = 0x1001,//(OGF_04_INFO_PARAMS<<10)|OCF_R_LOC_VERSION_INFO,//opcode 0x1001
        .status             = 0x00,  //ERR_00_SUCCESS
        .hci_version    = 0x08,  //HCI version, 0x06: Bluetooth spec 4.0
                                 //             0x07: Bluetooth spec 4.1
                                 //             0x08: Bluetooth spec 4.2
                                 //             0x09: Bluetooth spec 5.0
                                 //             0x0A: Bluetooth spec 5.1
                                 //             0x0B: Bluetooth spec 5.2
                                 //             0x0C: Bluetooth spec 5.3
        .hci_subversion = 0x0C5C,//HCI revision
      //.lmp_version    = leconfig_versionInfo.versNr,
                                 //LMP version, 0x06: Bluetooth spec 4.0
                                 //             0x07: Bluetooth spec 4.1
                                 //             0x08: Bluetooth spec 4.2
                                 //             0x09: Bluetooth spec 5.0
                                 //             0x0A: Bluetooth spec 5.1
                                 //             0x0B: Bluetooth spec 5.2
                                 //             0x0C: Bluetooth spec 5.3
      //.company_identifier= leconfig_versionInfo.compId,//manufacturer name  0x000A:Qualcomm, 0x000F:Broadcom
      //.lmp_subversion    = leconfig_versionInfo.subVersNr //LMP subver
    };
        ht_write_byte ( (uint8_t *)&(evt0e1001.lmp_version       ), leconfig_versionInfo.versNr    );
        ht_write_hword( (uint8_t *)&(evt0e1001.company_identifier), leconfig_versionInfo.compId    );
        ht_write_hword( (uint8_t *)&(evt0e1001.lmp_subversion    ), leconfig_versionInfo.subVersNr );
  /*
    {   uint8_t octet[4];
        octet[0] = 0x00;
        octet[1] = 0x00;
        octet[2] = ((LEACL_TypeDef *)(leACLlist.front))->llcInitiateProcedure.state.reg;
        octet[3] = ((LEACL_TypeDef *)(leACLlist.front))->llcRespondProcedure.state.reg;
        send_04_hciEvent_3e_ff_LE_kidd_debug(4, octet+0);
    }
  */
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e1001);
}

bool hcmd_1002_read_local_supported_commands(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_readLocalSupportedCommands_TypeDef evt0e1002 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e1002)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_1002    = 0x1002,//(OGF_04_INFO_PARAMS<<10)|OCF_R_LOC_SUPPORTED_COMMANDS,//opcode 0x1002
        .status             = 0x00,  //ERR_00_SUCCESS
    //  .supported_commands[0]=  0x00,        //mask octet 0
    //  .supported_commands[1]=  0x00,        //mask octet 1
    //  .supported_commands[2]=  0x00,        //mask octet 2
    //  .supported_commands[3]=  0x00,        //mask octet 3
    //  .supported_commands[4]=  0x00,        //mask octet 4
        .supported_commands[5]=  0xC0,        //mask octet 5
                              //              0 Switch Role
                              //              1 Read Link Policy Settings
                              //              2 Write Link Policy Settings
                              //              3 Read Default Link Policy Settings
                              //              4 Write Default Link Policy Settings
                              //              5 Flow Specification
                              //              6 Set Event Mask                           <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              7 Reset                                    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
    //  .supported_commands[6]=  0x00,        //mask octet 6
    //  .supported_commands[7]=  0x00,        //mask octet 7
    //  .supported_commands[8]=  0x00,        //mask octet 8
    //  .supported_commands[9]=  0x00,        //mask octet 9
    //  .supported_commands[10]= 0x00,        //mask octet 10
    //  .supported_commands[11]= 0x00,        //mask octet 11
    //  .supported_commands[12]= 0x00,        //mask octet 12
    //  .supported_commands[13]= 0x00,        //mask octet 13
        .supported_commands[14]= 0x38,        //mask octet 14
                              //              0 Reserved
                              //              1 Reserved
                              //              2 Reserved
                              //              3 Read Local Version Information           <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              4 Read Local Supported Commands            <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              5 Read Local Supported Features            <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              6 Read Local Extended Features
                              //              7 Read Buffer Size
        .supported_commands[15]= 0x02,        //mask octet 15
                              //              0 Read Country Code [Deprecated]
                              //              1 Read BD ADDR                             <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              2 Read Failed Contact Counter 
                              //              3 Reset Failed Contact Counter 
                              //              4 Read Link Quality 
                              //              5 Read RSSI
                              //              6 Read AFH Channel Map
                              //              7 Read Clock 
    //  .supported_commands[16]= 0x00,        //mask octet 16
    //  .supported_commands[17]= 0x00,        //mask octet 17
    //  .supported_commands[18]= 0x00,        //mask octet 18
    //  .supported_commands[19]= 0x00,        //mask octet 19
    //  .supported_commands[20]= 0x00,        //mask octet 20
    //  .supported_commands[21]= 0x00,        //mask octet 21
    //  .supported_commands[22]= 0x00,        //mask octet 22
    //  .supported_commands[23]= 0x00,        //mask octet 23
    //  .supported_commands[24]= 0x00,        //mask octet 24
        .supported_commands[25]= 0xF7,        //mask octet 25
                              //              0 LE Set Event Mask                        <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              1 LE Read Buffer Size                      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              2 LE Read Local Supported Features         <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              3 Reserved
                              //              4 LE Set Random Address                    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              5 LE Set Advertising Parameters            <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              6 LE Read Advertising Channel TX Power     <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              7 LE Set Advertising Data                  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
        .supported_commands[26]= 0xC2,        //mask octet 26
                              //              0 LE Set Scan Response Data
                              //              1 LE Set Advertise Enable                  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              2 LE Set Scan Parameters
                              //              3 LE Set Scan Enable
                              //              4 LE Create Connection
                              //              5 LE Create Connection Cancel
                              //              6 LE Read White List Size                  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              7 LE Clear White List                      <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
        .supported_commands[27]= 0x03,        //mask octet 27
                              //              0 LE Add Device To White List              <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              1 LE Remove Device From White List         <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              2 LE Connection Update
                              //              3 LE Set Host Channel Classification
                              //              4 LE Read Channel Map
                              //              5 LE Read Remote Used Features
                              //              6 LE Encrypt
                              //              7 LE Rand
        .supported_commands[28]= 0x68,        //mask octet 28
                              //              0 LE Start Encryption
                              //              1 LE Long Term Key Request Reply
                              //              2 LE Long Term Key Request Negative Reply 
                              //              3 LE Read Supported States                 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              4 LE Receiver Test v1
                              //              5 LE Transmitter Test v1                   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              6 LE Test End                              <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< supported
                              //              7 Reserved
    //  .supported_commands[29]= 0x00,        //mask octet 29
                              //              0 Reserved for future use
                              //              1 Reserved for future use
                              //              2 Reserved for future use
                              //              3 HCI_Enhanced_Setup_Synchronous_Connection
                              //              4 HCI_Enhanced_Accept_Synchronous_Connection
                              //              5 HCI_Read_Local_Supported_Codecs
                              //              6 HCI_Set_MWS_Channel_Parameters 
                              //              7 HCI_Set_External_Frame_Configuration
    //  .supported_commands[30]= 0x00,        //mask octet 30
                              //              0 HCI_Set_MWS_Signaling 
                              //              1 HCI_Set_MWS_Transport_Layer 
                              //              2 HCI_Set_MWS_Scan_Frequency_Table 
                              //              3 HCI_Get_MWS_Transport_Layer_Configuration 
                              //              4 HCI_Set_MWS_PATTERN_Configuration 
                              //              5 HCI_Set_Triggered_Clock_Capture
                              //              6 HCI_Truncated_Page
                              //              7 HCI_Truncated_Page_Cancel
    //  .supported_commands[31]= 0x00,        //mask octet 31
                              //              0 HCI_Set_Connectionless_Peripheral_Broadcast
                              //              1 HCI_Set_Connectionless_Peripheral_Broadcast_Receive
                              //              2 HCI_Start_Synchronization_Train
                              //              3 HCI_Receive_Synchronization_Train
                              //              4 HCI_Set_Reserved_LT_ADDR
                              //              5 HCI_Delete_Reserved_LT_ADDR
                              //              6 HCI_Set_Connectionless_Peripheral_Broadcast_Data
                              //              7 HCI_Read_Synchronization_Train_Parameters
        .supported_commands[32]= 0x30,        //mask octet 32
                              //              0 HCI_Write_Synchronization_Train_Parameters
                              //              1 HCI_Remote_OOB_Extended_Data_Request_Reply
                              //              2 HCI_Read_Secure_Connections_Host_Support
                              //              3 HCI_Write_Secure_Connections_Host_Support
                              //              4 HCI_Read_Authenticated_Payload_Timeout
                              //              5 HCI_Write_Authenticated_Payload_Timeout
                              //              6 HCI_Read_Local_OOB_Extended_Data
                              //              7 HCI_Write_Secure_Connections_Test_Mode
    //  .supported_commands[33]= 0x00,        //mask octet 33
                              //              0 HCI_Read_Extended_Page_Timeout
                              //              1 HCI_Write_Extended_Page_Timeout
                              //              2 HCI_Read_Extended_Inquiry_Length
                              //              3 HCI_Write_Extended_Inquiry_Length
                              //              4 HCI_LE_Remote_Connection_Parameter_Request_Reply 
                              //              5 HCI_LE_Remote_Connection_Parameter_Request_Negative_Reply
                              //              6 HCI_LE_Set_Data_Length
                              //              7 HCI_LE_Read_Suggested_Default_Data_Length
    //  .supported_commands[34]= 0x00,        //mask octet 34
                              //              0 HCI_LE_Write_Suggested_Default_Data_Length
                              //              1 HCI_LE_Read_Local_P-256_Public_Key
                              //              2 HCI_LE_Generate_DHKey [v1] 
                              //              3 HCI_LE_Add_Device_To_Resolving_List 
                              //              4 HCI_LE_Remove_Device_From_Resolving_List 
                              //              5 HCI_LE_Clear_Resolving_List 
                              //              6 HCI_LE_Read_Resolving_List_Size 
                              //              7 HCI_LE_Read_Peer_Resolvable_Address
    //  .supported_commands[35]= 0x00,        //mask octet 35
                              //              0 HCI_LE_Read_Local_Resolvable_Address 
                              //              1 HCI_LE_Set_Address_Resolution_Enable 
                              //              2 HCI_LE_Set_Resolvable_Private_Address_Timeout 
                              //              3 HCI_LE_Read_Maximum_Data_Length 
                              //              4 HCI_LE_Read_PHY
                              //              5 HCI_LE_Set_Default_PHY
                              //              6 HCI_LE_Set_PHY
                              //              7 HCI_LE_Receiver_Test [v2] 
    //  .supported_commands[36]= 0x00,        //mask octet 36
                              //              0 HCI_LE_Transmitter_Test [v2] 
                              //              1 HCI_LE_Set_Advertising_Set_Random_Address
                              //              2 HCI_LE_Set_Extended_Advertising_Parameters
                              //              3 HCI_LE_Set_Extended_Advertising_Data
                              //              4 HCI_LE_Set_Extended_Scan_Response_Data
                              //              5 HCI_LE_Set_Extended_Advertising_Enable
                              //              6 HCI_LE_Read_Maximum_Advertising_Data_Length
                              //              7 HCI_LE_Read_Number_of_Supported_Advertising_Sets 
    //  .supported_commands[37]= 0x00,        //mask octet 37
                              //              0 HCI_LE_Remove_Advertising_Set 
                              //              1 HCI_LE_Clear_Advertising_Sets 
                              //              2 HCI_LE_Set_Periodic_Advertising_Parameters
                              //              3 HCI_LE_Set_Periodic_Advertising_Data
                              //              4 HCI_LE_Set_Periodic_Advertising_Enable 
                              //              5 HCI_LE_Set_Extended_Scan_Parameters
                              //              6 HCI_LE_Set_Extended_Scan_Enable
                              //              7 HCI_LE_Extended_Create_Connection
    //  .supported_commands[38]= 0x00,        //mask octet 38
                              //              0 HCI_LE_Periodic_Advertising_Create_Sync
                              //              1 HCI_LE_Periodic_Advertising_Create_Sync_Cancel
                              //              2 HCI_LE_Periodic_Advertising_Terminate_Sync
                              //              3 HCI_LE_Add_Device_To_Periodic_Advertiser_List
                              //              4 HCI_LE_Remove_Device_From_Periodic_Advertiser_List
                              //              5 HCI_LE_Clear_Periodic_Advertiser_List
                              //              6 HCI_LE_Read_Periodic_Advertiser_List_Size
                              //              7 HCI_LE_Read_Transmit_Power
    //  .supported_commands[39]= 0x00,        //mask octet 39
                              //              0 HCI_LE_Read_RF_Path_Compensation
                              //              1 HCI_LE_Write_RF_Path_Compensation
                              //              2 HCI_LE_Set_Privacy_Mode
                              //              3 HCI_LE_Receiver_Test [v3] 
                              //              4 HCI_LE_Transmitter_Test [v3] 
                              //              5 HCI_LE_Set_Connectionless_CTE_Transmit_Parameters 
                              //              6 HCI_LE_Set_Connectionless_CTE_Transmit_Enable 
                              //              7 HCI_LE_Set_Connectionless_IQ_Sampling_Enable
    //  .supported_commands[40]= 0x00,        //mask octet 40
                              //              0 HCI_LE_Set_Connection_CTE_Receive_Parameters
                              //              1 HCI_LE_Set_Connection_CTE_Transmit_Parameters
                              //              2 HCI_LE_Connection_CTE_Request_Enable
                              //              3 HCI_LE_Connection_CTE_Response_Enable
                              //              4 HCI_LE_Read_Antenna_Information 
                              //              5 HCI_LE_Set_Periodic_Advertising_Receive_Enable
                              //              6 HCI_LE_Periodic_Advertising_Sync_Transfer
                              //              7 HCI_LE_Periodic_Advertising_Set_Info_Transfer
    //  .supported_commands[41]= 0x00,        //mask octet 41
                              //              0 HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters
                              //              1 HCI_LE_Set_Default_Periodic_Advertising_Sync_Transfer_Parameters
                              //              2 HCI_LE_Generate_DHKey [v2]
                              //              3 HCI_Read_Local_Simple_Pairing_Options 
                              //              4 HCI_LE_Modify_Sleep_Clock_Accuracy 
                              //              5 HCI_LE_Read_Buffer_Size [v2]
                              //              6 HCI_LE_Read_ISO_TX_Sync
                              //              7 HCI_LE_Set_CIG_Parameters
    //  .supported_commands[42]= 0x00,        //mask octet 42
                              //              0 HCI_LE_Set_CIG_Parameters_Test
                              //              1 HCI_LE_Create_CIS
                              //              2 HCI_LE_Remove_CIG
                              //              3 HCI_LE_Accept_CIS_Request
                              //              4 HCI_LE_Reject_CIS_Request
                              //              5 HCI_LE_Create_BIG
                              //              6 HCI_LE_Create_BIG_Test
                              //              7 HCI_LE_Terminate_BIG
    //  .supported_commands[43]= 0x00,        //mask octet 43
                              //              0 HCI_LE_BIG_Create_Sync
                              //              1 HCI_LE_BIG_Terminate_Sync
                              //              2 HCI_LE_Request_Peer_SCA
                              //              3 HCI_LE_Setup_ISO_Data_Path
                              //              4 HCI_LE_Remove_ISO_Data_Path
                              //              5 HCI_LE_ISO_Transmit_Test
                              //              6 HCI_LE_ISO_Receive_Test
                              //              7 HCI_LE_ISO_Read_Test_Counters
    //  .supported_commands[44]= 0x00,        //mask octet 44
                              //              0 HCI_LE_ISO_Test_End
                              //              1 HCI_LE_Set_Host_Feature
                              //              2 HCI_LE_Read_ISO_Link_Quality
                              //              3 HCI_LE_Enhanced_Read_Transmit_Power_Level
                              //              4 HCI_LE_Read_Remote_Transmit_Power_Level
                              //              5 HCI_LE_Set_Path_Loss_Reporting_Parameters
                              //              6 HCI_LE_Set_Path_Loss_Reporting_Enable
                              //              7 HCI_LE_Set_Transmit_Power_Reporting_Enable
    //  .supported_commands[45]= 0x00,        //mask octet 45
                              //              0 HCI_LE_Transmitter_Test [v4]
                              //              1 HCI_Set_Ecosystem_Base_Interval
                              //              2 HCI_Read_Local_Supported_Codecs [v2]
                              //              3 HCI_Read_Local_Supported_Codec_Capabilities
                              //              4 HCI_Read_Local_Supported_Controller_Delay
                              //              5 HCI_Configure_Data_Path
                              //              6 HCI_LE_Set_Data_Related_Address_Changes
                              //              7 HCI_Set_Min_Encryption_Key_Size
    //  .supported_commands[46]= 0x00,        //mask octet 46
                              //              0 HCI_LE_Set_Default_Subrate command
                              //              1 HCI_LE_Subrate_Request command
                              //              2 Reserved for future use
                              //              3 Reserved for future use
                              //              4 Reserved for future use
                              //              5 Reserved for future use
                              //              6 Reserved for future use
                              //              7 Reserved for future use
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e1002);
}

bool hcmd_1003_read_local_supported_features(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_readLocalSupportedFeatures_TypeDef evt0e1003 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e1003)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_1003    = 0x1003,//(OGF_04_INFO_PARAMS<<10)|OCF_R_LOC_SUPPORTED_FEATURES,//opcode 0x1003
        .status             = 0x00,  //ERR_00_SUCCESS
      /*
        .lmp_features[0]    = leconfig_lmpFeatures.lmp_features[0],//[0]
        .lmp_features[1]    = leconfig_lmpFeatures.lmp_features[1],//[1]
        .lmp_features[2]    = leconfig_lmpFeatures.lmp_features[2],//[2]
        .lmp_features[3]    = leconfig_lmpFeatures.lmp_features[3],//[3]
        .lmp_features[4]    = leconfig_lmpFeatures.lmp_features[4],//[4]
                                   //bit 32=0:EV4 eSCO not supported
                                   //bit 33=0:EV5 eSCO not supported
                                   //bit 34=0:reserved
                                   //bit 35  :"AFH capable slave"
                                   //bit 36  :"AFH classification slave"
                                   //bit 37=1:BR/EDR Not Supported
                                   //bit 38=1:LE Supported (Controller)
                                   //bit 39  :3-slot EDR ACL packets
        .lmp_features[5]    = leconfig_lmpFeatures.lmp_features[5],//[5]
        .lmp_features[6]    = leconfig_lmpFeatures.lmp_features[6],//[6]
        .lmp_features[7]    = leconfig_lmpFeatures.lmp_features[7] //[7]
      */
    };
        ht_memory_copy( evt0e1003.lmp_features+0, leconfig_lmpFeatures.lmp_features+0, 8); // 8 octets
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e1003);
}

bool hcmd_1009_read_bd_addr(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_readBdAddr_TypeDef evt0e1009 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e1009)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_1009    = 0x1009,//(OGF_04_INFO_PARAMS<<10)|OCF_R_BD_ADDR,//opcode 0x1009
        .status             = 0x00,  //ERR_00_SUCCESS
      /*
        .bd_addr[0]         = leconfig_bdaddr.le_public_AdvA[0],//on an LE Controller, this command shall read the Public Device Address
        .bd_addr[1]         = leconfig_bdaddr.le_public_AdvA[1],//on an LE Controller, this command shall read the Public Device Address
        .bd_addr[2]         = leconfig_bdaddr.le_public_AdvA[2],//on an LE Controller, this command shall read the Public Device Address
        .bd_addr[3]         = leconfig_bdaddr.le_public_AdvA[3],//on an LE Controller, this command shall read the Public Device Address
        .bd_addr[4]         = leconfig_bdaddr.le_public_AdvA[4],//on an LE Controller, this command shall read the Public Device Address
        .bd_addr[5]         = leconfig_bdaddr.le_public_AdvA[5] //on an LE Controller, this command shall read the Public Device Address
      */
    };
        ht_memory_copy( evt0e1009.bd_addr+0, leconfig_bdaddr.le_public_AdvA+0, 6); // 6 octets
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e1009);
}

bool hcmd_0c01_set_event_mask(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_0c01_set_event_mask_TypeDef *pCmd0c01 = (hcicmd_0c01_set_event_mask_TypeDef *)pCmd;
        ht_memory_copy( leconfig_eventMask.event_mask+0, pCmd0c01->event_mask+0, 8); // 8 octets
    return send_04_hciEvent_0e_command_complete(0x0c01, 0x00); //ERR_00_SUCCESS
}

bool hcmd_0c63_set_event_mask_page_2(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_0c63_set_event_mask_page_2_TypeDef *pCmd0c63 = (hcicmd_0c63_set_event_mask_page_2_TypeDef *)pCmd;
        ht_memory_copy( leconfig_eventMaskPage2.event_mask+0, pCmd0c63->event_mask+0, 8); // 8 octets
    return send_04_hciEvent_0e_command_complete(0x0c63, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2001_le_set_event_mask(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2001_le_set_event_mask_TypeDef *pCmd2001 = (hcicmd_2001_le_set_event_mask_TypeDef *)pCmd;
        ht_memory_copy( leconfig_leEventMask.le_event_mask+0, pCmd2001->le_event_mask+0, 8 );
    return send_04_hciEvent_0e_command_complete(0x2001, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2002_le_read_buffer_size_v1(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_leReadBufferSizeV1_TypeDef evt0e2002 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2002)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2002    = 0x2002,//(OGF_08_LE<<10)|OCF_002_LE_R_BUFFER_SIZE_V1,//opcode 0x2002
        .status             = 0x00,  //ERR_00_SUCCESS
        .le_acl_data_packet_length     = LENGTH_LE_ACL_DATA_PACKET_TOAIR,
        .total_num_le_acl_data_packets = TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2002);
}

bool hcmd_2003_le_read_local_supported_features(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_leReadLocalSupportedFeatures_TypeDef evt0e2003 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2003)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2003    = 0x2003,//(OGF_08_LE<<10)|OCF_003_LE_R_LOC_SUPPORTED_FEATURES,//opcode 0x2003
        .status             = 0x00,  //ERR_00_SUCCESS
        .le_features[0]= LOCAL_FEATURE_get_featureSet(0),//[0]
                              //spec 4.0   0 LE Encryption
                              //spec 4.1   1 Conn Params Request Procedure
                              //spec 4.1   2 Extended Reject Indication
                              //spec 4.1   3 Slave-initiated Features Exchange
                              //spec 4.1   4 LE Ping
                              //spec 4.2   5 LE Data Packet Length Extension
                              //spec 4.2   6 LL Privacy
                              //spec 4.2   7 Extended Scanner Filter Policies
        .le_features[1]= LOCAL_FEATURE_get_featureSet(1),//[1]
                              //spec 5.0   8  LE 2M PHY
                              //spec 5.0   9  Stable Modulation Index - Transmitter
                              //spec 5.0   10 Stable Modulation Index - Receiver
                              //spec 5.0   11 LE Coded PHY
                              //spec 5.0   12 LE Extended Advertising
                              //spec 5.0   13 LE Periodic Advertising
                              //spec 5.0   14 Channel Selection Algorithm #2
                              //spec 5.0   15 LE Power Class 1 
        .le_features[2]= LOCAL_FEATURE_get_featureSet(2),//[2]
                              //spec 5.0   16 Minimum Number of Used Channels Procedure
                              //spec 5.1   17 Connection CTE Request
                              //spec 5.1   18 Connection CTE Response
                              //spec 5.1   19 Connectionless CTE Transmitter
                              //spec 5.1   20 Connectionless CTE Receiver
                              //spec 5.1   21 Antenna Switching During CTE Transmission (AoD)
                              //spec 5.1   22 Antenna Switching During CTE Reception (AoA)
                              //spec 5.1   23 Receiving Constant Tone Extensions
        .le_features[3]= LOCAL_FEATURE_get_featureSet(3),//[3]
                              //spec 5.1   24 Periodic Advertising Sync Transfer - Sender
                              //spec 5.1   25 Periodic Advertising Sync Transfer - Recipient
                              //spec 5.1   26 Sleep Clock Accuracy Updates
                              //spec 5.1   27 Remote Public Key Validation
                              //spec 5.2   28 Connected Isochronous Stream - Central
                              //spec 5.2   29 Connected Isochronous Stream - Peripheral
                              //spec 5.2   30 Isochronous Broadcaster
                              //spec 5.2   31 Synchronized Receiver
        .le_features[4]= LOCAL_FEATURE_get_featureSet(4),//[4]RFU
                              //spec 5.2   32 Connected Isochronous Stream (Host Support)
                              //spec 5.2   33 LE Power Control Request (Bits 33 and 34 shall always have the same value)
                              //spec 5.2   34 LE Power Control Request (Bits 33 and 34 shall always have the same value)
                              //spec 5.2   35 LE Path Loss Monitoring
                              //spec 5.3   36 Periodic Advertising ADI support
                              //spec 5.3   37 Connection Subrating
                              //spec 5.3   38 Connection Subrating (Host Support)
                              //spec 5.3   39 Channel Classification
        .le_features[5]= LOCAL_FEATURE_get_featureSet(5),//[5]RFU
        .le_features[6]= LOCAL_FEATURE_get_featureSet(6),//[6]RFU
        .le_features[7]= LOCAL_FEATURE_get_featureSet(7) //[7]RFU
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2003);
}

bool hcmd_2005_le_set_random_address(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2005_LE_set_random_address_TypeDef *pCmd2005 = (hcicmd_2005_LE_set_random_address_TypeDef *)pCmd;
        ht_memory_copy( leconfig_randomAddress.random_address+0, pCmd2005->random_address+0, 6 );
    return send_04_hciEvent_0e_command_complete(0x2005, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2006_le_set_adv_parameters(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2006_LE_set_adv_parameters_TypeDef *pCmd2006 = (hcicmd_2006_LE_set_adv_parameters_TypeDef *)pCmd;
  //
    ht_write_hword( (uint8_t *)&(leconfig_AdvParam.adv_interval_min ), ht_read_hword((uint8_t *)&(pCmd2006->adv_interval_min )) );
    ht_write_hword( (uint8_t *)&(leconfig_AdvParam.adv_interval_max ), ht_read_hword((uint8_t *)&(pCmd2006->adv_interval_max )) );
    ht_write_byte ( (uint8_t *)&(leconfig_AdvParam.adv_type         ), ht_read_byte ((uint8_t *)&(pCmd2006->adv_type         )) );
    ht_write_byte ( (uint8_t *)&(leconfig_AdvParam.own_address_type ), ht_read_byte ((uint8_t *)&(pCmd2006->own_address_type )) );
    ht_write_byte ( (uint8_t *)&(leconfig_AdvParam.peer_address_type), ht_read_byte ((uint8_t *)&(pCmd2006->peer_address_type)) );
    ht_memory_copy(leconfig_AdvParam.peer_address, pCmd2006->peer_address, 6);
    ht_write_byte ( (uint8_t *)&(leconfig_AdvParam.adv_channel_map  ), ht_read_byte ((uint8_t *)&(pCmd2006->adv_channel_map  )) );
    ht_write_byte ( (uint8_t *)&(leconfig_AdvParam.adv_filter_policy), ht_read_byte ((uint8_t *)&(pCmd2006->adv_filter_policy)) );
  /*
    leconfig_AdvParam.adv_interval_min  = pCmd2006->adv_interval_min;
    leconfig_AdvParam.adv_interval_max  = pCmd2006->adv_interval_max;
    leconfig_AdvParam.adv_type          = pCmd2006->adv_type;
    leconfig_AdvParam.own_address_type  = pCmd2006->own_address_type;
    leconfig_AdvParam.peer_address_type = pCmd2006->peer_address_type;
    ht_memory_copy(leconfig_AdvParam.peer_address, pCmd2006->peer_address, 6);
    leconfig_AdvParam.adv_channel_map   = pCmd2006->adv_channel_map&0x07; //[2]Channel 39 shall be used, [1]Channel 38 shall be used, [0]Channel 37 shall be used
    leconfig_AdvParam.adv_filter_policy = pCmd2006->adv_filter_policy;
  */

    return send_04_hciEvent_0e_command_complete(0x2006, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2007_le_read_adv_channel_tx_power(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_leReadAdvPhyChannelTxPower_TypeDef evt0e2007 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2007)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2007    = 0x2007,//(OGF_08_LE<<10)|OCF_007_LE_R_ADV_CHANNEL_TX_PWR,//opcode 0x2007
        .status             = 0x00,  //ERR_00_SUCCESS
    //  .tx_power_level     = 0x08 //Transmit_Power_Level,  Range: -20 <= N <= 10,  Units: dBm          7262: 8dBm
        .tx_power_level     = 0x05 //Transmit_Power_Level,  Range: -20 <= N <= 10,  Units: dBm          7262: 8dBm
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2007);
}

bool hcmd_2008_le_set_adv_data(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2008_LE_set_adv_data_TypeDef *pCmd2008 = (hcicmd_2008_LE_set_adv_data_TypeDef *)pCmd;
    
        ht_memory_set ((uint8_t *)(&leconfig_AdvData), 0x00, sizeof(leconfig_AdvData));//clear
        ht_write_byte ( (uint8_t *)&(leconfig_AdvData.length), ht_read_byte ((uint8_t *)&(pCmd2008->adv_data_length)) );
        ht_memory_copy( leconfig_AdvData.advData+0, pCmd2008->adv_data+0,    pCmd2008->adv_data_length);
    return send_04_hciEvent_0e_command_complete(0x2008, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2009_le_set_scanresp_data(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2009_LE_set_scanresp_data_TypeDef *pCmd2009 = (hcicmd_2009_LE_set_scanresp_data_TypeDef *)pCmd;
    
        ht_memory_set((uint8_t *)(&leconfig_ScanRspData), 0x00, sizeof(leconfig_ScanRspData));//clear
        ht_write_byte ( (uint8_t *)&(leconfig_ScanRspData.length), ht_read_byte ((uint8_t *)&(pCmd2009->scanresp_data_length)) );
        ht_memory_copy( leconfig_ScanRspData.scanRspData+0, pCmd2009->scanresp_data+0, pCmd2009->scanresp_data_length);
    return send_04_hciEvent_0e_command_complete(0x2009, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200a_le_set_adv_enable(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_200a_LE_set_adv_enable_TypeDef *pCmd200a = (hcicmd_200a_LE_set_adv_enable_TypeDef *)pCmd;
    if     (pCmd200a->adv_enable == 0x00) // 0x00:Advertising is disabled
    {
    }
    else if(pCmd200a->adv_enable == 0x01) // 0x01:Advertising is enabled
    {
        { //debug
            LEACL_TypeDef *pacl;
            pacl = leacl_alloc();
            pacl->role = 0x01;                 // 0x00:master, 0x01:slave
debug_hcfsm_state_ST_0();
        }
    }
    else
    {
    }
    return send_04_hciEvent_0e_command_complete(0x200a, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200b_le_set_scan_parameters(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_200b_LE_set_scan_parameters_TypeDef *pCmd200b = (hcicmd_200b_LE_set_scan_parameters_TypeDef *)pCmd;
  //
    ht_write_byte ( (uint8_t *)&(leconfig_ScanParam.le_scan_type          ), ht_read_byte ((uint8_t *)&(pCmd200b->le_scan_type          )) );
    ht_write_hword( (uint8_t *)&(leconfig_ScanParam.le_scan_interval      ), ht_read_hword((uint8_t *)&(pCmd200b->le_scan_interval      )) );
    ht_write_hword( (uint8_t *)&(leconfig_ScanParam.le_scan_window        ), ht_read_hword((uint8_t *)&(pCmd200b->le_scan_window        )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ScanParam.own_address_type      ), ht_read_byte ((uint8_t *)&(pCmd200b->own_address_type      )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ScanParam.scanning_filter_policy), ht_read_byte ((uint8_t *)&(pCmd200b->scanning_filter_policy)) );
  /*
    leconfig_ScanParam.le_scan_type           = pCmd200b->le_scan_type;
    leconfig_ScanParam.le_scan_interval       = pCmd200b->le_scan_interval;
    leconfig_ScanParam.le_scan_window         = pCmd200b->le_scan_window;
    leconfig_ScanParam.own_address_type       = pCmd200b->own_address_type;
    leconfig_ScanParam.scanning_filter_policy = pCmd200b->scanning_filter_policy;
  */
    return send_04_hciEvent_0e_command_complete(0x200b, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200c_le_set_scan_enable(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_200c_LE_set_scan_enable_TypeDef *pCmd200c = (hcicmd_200c_LE_set_scan_enable_TypeDef *)pCmd;
    if     (pCmd200c->scan_enable==0x00) // 0x00:Scanning disabled
    {
    }
    else if(pCmd200c->scan_enable==0x01) // 0x01:Scanning enabled
    {
      {
        LEACL_TypeDef *pacl;
        pacl = leacl_alloc();
        pacl->role = 0x00;                 // 0x00:master, 0x01:slave
	debug_hcfsm_state_ST_0();
      }
    }
    else {
    }
    return send_04_hciEvent_0e_command_complete(0x200c, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200d_le_create_connection(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_200d_LE_create_connection_TypeDef *pCmd200d = (hcicmd_200d_LE_create_connection_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
        pacl = leacl_with_peerAddress( pCmd200d->peer_address+0 );
    if (pacl != 0)
    {
        return send_04_hciEvent_0f_command_status(0x200d, 0x0B); //ERR_0B_ACL_CONN_ALREADY_EXISTS
    }
    
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.le_scan_interval       ), ht_read_hword((uint8_t *)&(pCmd200d->le_scan_interval       )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.le_scan_window         ), ht_read_hword((uint8_t *)&(pCmd200d->le_scan_window         )) );
    ht_write_byte ( (uint8_t *)&(leconfig_CreateConnection.initiator_filter_policy), ht_read_byte ((uint8_t *)&(pCmd200d->initiator_filter_policy)) );
    ht_write_byte ( (uint8_t *)&(leconfig_CreateConnection.peer_address_type      ), ht_read_byte ((uint8_t *)&(pCmd200d->peer_address_type      )) );
    ht_memory_copy( leconfig_CreateConnection.peer_address+0, pCmd200d->peer_address+0, 6); //params[6] ~ [11]  6 octets
    ht_write_byte ( (uint8_t *)&(leconfig_CreateConnection.own_address_type       ), ht_read_byte ((uint8_t *)&(pCmd200d->own_address_type       )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.conn_interval_min      ), ht_read_hword((uint8_t *)&(pCmd200d->conn_interval_min      )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.conn_interval_max      ), ht_read_hword((uint8_t *)&(pCmd200d->conn_interval_max      )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.slave_latency          ), ht_read_hword((uint8_t *)&(pCmd200d->slave_latency          )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.supervision_timeout    ), ht_read_hword((uint8_t *)&(pCmd200d->supervision_timeout    )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.min_CE_length          ), ht_read_hword((uint8_t *)&(pCmd200d->min_CE_length          )) );
    ht_write_hword( (uint8_t *)&(leconfig_CreateConnection.max_CE_length          ), ht_read_hword((uint8_t *)&(pCmd200d->max_CE_length          )) );
    {
        pacl = leacl_alloc();
        pacl->role = 0x00;                 // 0x00:master, 0x01:slave
debug_hcfsm_state_ST_0();
    }
    return send_04_hciEvent_0f_command_status(0x200d, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200e_le_create_connection_cancel(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    return send_04_hciEvent_0e_command_complete(0x200e, 0x00); //ERR_00_SUCCESS
}

bool hcmd_200f_le_read_white_list_size(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_leReadWhiteListSize_TypeDef evt0e200f =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e200f)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_200f    = 0x200f,//(OGF_08_LE<<10)|OCF_00F_LE_R_WHITE_LIST_SIZE,//opcode 0x200f
        .status             = 0x00,  //ERR_00_SUCCESS
        .white_list_size    = LE_WHITE_LIST_SIZE //white list size
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e200f);
}

bool hcmd_2010_le_clear_white_list(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    return send_04_hciEvent_0e_command_complete(0x2010, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2011_le_add_device_to_white_list(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2011_LE_add_device_to_whitelist_TypeDef *pCmd2011 = (hcicmd_2011_LE_add_device_to_whitelist_TypeDef *)pCmd;
    if( leconfig_whiteList[0].address_type == pCmd2011->addr_type ) {
        ht_memory_copy( leconfig_whiteList[0].address, pCmd2011->address, 6);
    }
    return send_04_hciEvent_0e_command_complete(0x2011, 0x00); //ERR_00_SUCCESS
}

bool hcmd_2012_le_remove_device_from_white_list(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2012_LE_remove_device_from_whitelist_TypeDef *pCmd2012 = (hcicmd_2012_LE_remove_device_from_whitelist_TypeDef *)pCmd;
    if( leconfig_whiteList[0].address_type == pCmd2012->addr_type ) {
        ht_memory_copy( leconfig_whiteList[0].address, pCmd2012->address, 6);
    }
    return send_04_hciEvent_0e_command_complete(0x2012, 0x00); //ERR_00_SUCCESS
}

/**
    //LL/CON/MAS/BV-24-C [Initiating Conn Param Request - Accept]
    //LL/CON/SLA/BV-24-C [Initiating Conn Param Request - Accept]
    //LL/CON/MAS/BV-25-C [Initiating Conn Param Request - Reject]
    //LL/CON/SLA/BV-25-C [Initiating Conn Param Request - Reject]
    //LL/CON/MAS/BV-26-C [Initiating Conn Param Request - same procedure collision]
    //LL/CON/SLA/BV-26-C [Initiating Conn Param Request - same procedure collision]
    //LL/CON/MAS/BV-27-C [Initiating Conn Param Request - different procedure collision - channel map update]
    //LL/CON/SLA/BV-27-C [Initiating Conn Param Request - different procedure collision - channel map update]
    //LL/CON/MAS/BV-28-C [Initiating Conn Param Request - different procedure collision - encryption]
    //LL/CON/SLA/BV-28-C [Initiating Conn Param Request - different procedure collision - encryption]
    //LL/CON/MAS/BV-81-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
    //LL/CON/SLA/BV-85-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
    //LL/CON/MAS/BV-82-C [Initiating Conn Param Request - Unsupported With    Feature Exchange]
    //LL/CON/SLA/BV-86-C [Initiating Conn Param Request - Unsupported With    Feature Exchange]
    //LL/CON/MAS/BI-05-C [Initiating Conn Param Request - Timeout]
    //LL/CON/SLA/BI-07-C [Initiating Conn Param Request - Timeout]
    
    //LL/CON/MAS/BV-29-C [Initiating Conn Param Request - remote legacy host]
    
    //LL/CON/MAS/BV-30-C [ Accepting Conn Param Request - no Preferred_Periodicity]
    //LL/CON/SLA/BV-29-C [ Accepting Conn Param Request - no Preferred_Periodicity]
    //LL/CON/MAS/BV-31-C [ Accepting Conn Param Request - preferred anchor points only]
    //LL/CON/SLA/BV-30-C [ Accepting Conn Param Request - preferred anchor points only]
    //LL/CON/MAS/BV-32-C [ Accepting Conn Param Request -    Preferred_Periodicity]
    //LL/CON/SLA/BV-31-C [ Accepting Conn Param Request -    Preferred_Periodicity]
    //LL/CON/MAS/BV-33-C [ Accepting Conn Param Request -    Preferred_Periodicity and preferred anchor points]
    //LL/CON/SLA/BV-32-C [ Accepting Conn Param Request -    Preferred_Periodicity and preferred anchor points]
    //LL/CON/MAS/BV-34-C [ Accepting Conn Param Request - event masked]
    //LL/CON/SLA/BV-33-C [ Accepting Conn Param Request - event masked]
    //LL/CON/MAS/BV-35-C [ Accepting Conn Param Request - Host rejects]
    //LL/CON/SLA/BV-34-C [ Accepting Conn Param Request - Host rejects]
    //LL/CON/MAS/BI-06-C [ Accepting Conn Param Request - illegal parameters]
    //LL/CON/SLA/BI-08-C [ Accepting Conn Param Request - illegal parameters]
    
    //LL/CON/MAS/BV-48-C [Handling Protocol Collision - Different Procedure - Conn Params]
    //LL/CON/SLA/BV-47-C [Handling Protocol Collision - Different Procedure - Conn Params]
 **/
bool hcmd_2013_le_connection_update(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    bool done;
    hcicmd_2013_LE_connection_update_TypeDef *pCmd2013 = (hcicmd_2013_LE_connection_update_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x2013, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    else
    {
        done = send_04_hciEvent_0f_command_status(0x2013, 0x00); //ERR_00_SUCCESS
    }
    /////////
    //save
    if (done)
    {
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
      /*
        pacl->pTemplateConnParam->host.interval_min        = pCmd2013->conn_interval_min;
        pacl->pTemplateConnParam->host.interval_max        = pCmd2013->conn_interval_max;
        pacl->pTemplateConnParam->host.max_slave_latency   = pCmd2013->max_slave_latency;
        pacl->pTemplateConnParam->host.supervision_timeout = pCmd2013->supervision_timeout;
        pacl->pTemplateConnParam->host.min_CE_length       = pCmd2013->min_CE_length;
        pacl->pTemplateConnParam->host.max_CE_length       = pCmd2013->max_CE_length;
      */
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.interval_min       ), ht_read_hword((uint8_t *)&(pCmd2013->conn_interval_min  )) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.interval_max       ), ht_read_hword((uint8_t *)&(pCmd2013->conn_interval_max  )) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.max_slave_latency  ), ht_read_hword((uint8_t *)&(pCmd2013->max_slave_latency  )) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.supervision_timeout), ht_read_hword((uint8_t *)&(pCmd2013->supervision_timeout)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.min_CE_length      ), ht_read_hword((uint8_t *)&(pCmd2013->min_CE_length      )) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.max_CE_length      ), ht_read_hword((uint8_t *)&(pCmd2013->max_CE_length      )) );
      //
        pacl->pTemplateConnParam->minInterval = pacl->pTemplateConnParam->host.interval_min;
        pacl->pTemplateConnParam->maxInterval = pacl->pTemplateConnParam->host.interval_max;
    }
    }//end done
    /////////
    if (done)
    {
        if (LEACL_is_role_master(pacl))
        {
            //Master
            if ((0 == LOCAL_FEATURE_is_supported_conn_params_request_procedure()) ||
                LEACL_is_remote_said_unknownType(pacl, LL_0F_CONNECTION_PARAM_REQ) ||      //LL/CON/MAS/BV-81-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
                REMOTE_FEATURE_is_valid_UNsupported_conn_params_request_procedure(pacl) )  //LL/CON/MAS/BV-82-C [Initiating Conn Param Request - Unsupported With    Feature Exchange]
            {
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,         //send LL_00_CONNECTION_UPDATE_IND
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16),     //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
            }
            else {
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_PARAMS_REQUEST_PROCEDURE, //send LL_0F_CONNECTION_PARAM_REQ
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16),     //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
            }
        }
        else
        {
            //Slave shall NOT send LL_00_CONNECTION_UPDATE_IND
            if ((0 == LOCAL_FEATURE_is_supported_conn_params_request_procedure()))
            {
                    send_04_hciEvent_3e_03_LE_conn_update_complete(pacl, 0x11); //ERR_11_UNSUPPORTED_FEATURE_OR_PARAM_VALUE
            }
            else
            if (LEACL_is_remote_said_unknownType(pacl, LL_0F_CONNECTION_PARAM_REQ) ||      //LL/CON/SLA/BV-85-C [Initiating Conn Param Request - Unsupported Without Feature Exchange]
                REMOTE_FEATURE_is_valid_UNsupported_conn_params_request_procedure(pacl) )  //LL/CON/SLA/BV-86-C [Initiating Conn Param Request - Unsupported With    Feature Exchange]
            {
                    send_04_hciEvent_3e_03_LE_conn_update_complete(pacl, 0x1A); //ERR_1A_UNSUPPORTED_REMOTE_FEATURE
            }
            else {
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_PARAMS_REQUEST_PROCEDURE, //send LL_0F_CONNECTION_PARAM_REQ
                                 LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16),     //param[2] 0x00:autonomously by LL, 0x01:by Host
                                 0,//length
                                 (uint8_t *)0
                               );
            }
        }
    }
    return (done);
}

bool hcmd_2014_le_set_host_channel_classification(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    bool done;
  //hcicmd_2014_LE_set_host_channel_classification_TypeDef *pCmd2014 = (hcicmd_2014_LE_set_host_channel_classification_TypeDef *)pCmd;
  //for(uint8_t i=0;i<5;i++) {
  //    leconfig_hostChannelClass.channel_map[i] = pCmd2014->channel_map[i];
  //}
    
    // This classification persists until overwritten with a subsequent HCI_LE_Set_Host_Channel_Classification command 
    // or until the Controller is reset using the HCI_Reset command
        ht_memory_copy( leconfig_hostChannelClass.channel_map, pCmd->params, 5);//params[0]~[4]  channel_map[0]~[4] 5 octets
    //hci event
        done = send_04_hciEvent_0e_command_complete(0x2014, 0x00); //ERR_00_SUCCESS
    if (done)
    {
        //Master can update the channel map by sending  LL_01_CHANNEL_MAP_IND
        dllist_node_TypeDef *pNode;
        LEACL_TypeDef *pacl;
               pNode = leACLlist.front ;
        while (pNode != NULL)
        {
                pacl = (LEACL_TypeDef *)(pNode) ;
            if (LEACL_is_role_master(pacl)) {
                                      //      Master can update the channel map by sending  LL_01_CHANNEL_MAP_IND
                    llcmsgQ_push( LLCMSG_INITIATE_LLC_CHANNEL_MAP_UPDATE_PROCEDURE, //send LL_01_CHANNEL_MAP_IND
                                 LEACL_conn_handle(pacl), 
                                 0,//length
                                 (uint8_t *)0
                               );
            }
            pNode = pNode->next;
        }
    }
    return (done);
}

bool hcmd_2015_le_read_channel_map(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_2015_LE_read_channel_map_TypeDef *pCmd2015 = (hcicmd_2015_LE_read_channel_map_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x2015, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    hcievt_0e_cmd_complete_leReadChannelMap_TypeDef evt0e2015 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2015)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2015    = 0x2015,//(OGF_08_LE<<10)|OCF_015_LE_R_CHANNEL_MAP,//opcode 0x2015
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = LEACL_conn_handle(pacl),
      //.channel_map[0]     = pacl->currChM.chM[0],
      //.channel_map[1]     = pacl->currChM.chM[1],
      //.channel_map[2]     = pacl->currChM.chM[2],
      //.channel_map[3]     = pacl->currChM.chM[3],
      //.channel_map[4]     = pacl->currChM.chM[4]
    };
        ht_write_hword( (uint8_t *)&(evt0e2015.conn_handle  ), handle );
        ht_memory_copy( evt0e2015.channel_map+0, pacl->currChM.chM+0, 5);
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2015);
}

bool hcmd_2016_le_read_remote_features(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_2016_LE_read_remote_features_TypeDef *pCmd2016 = (hcicmd_2016_LE_read_remote_features_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
            return send_04_hciEvent_0f_command_status(0x2016, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    else
    {
            done = send_04_hciEvent_0f_command_status(0x2016, 0x00); //ERR_00_SUCCESS
        if (done) {
          /*
            if (REMOTE_FEATURE_is_valid(pacl)) {
                //LL/CON/MAS/BV-13-C [Feature Setup Request]
                //IUT may send LL_FEATURE_REQ autonomously at any time during connection lifetime and return cached feature set in response to Host if executed prior to HCI command being received
                send_04_hciEvent_3e_04_LE_read_remote_feature_complete(pacl, 0x00); //ERR_00_SUCCESS
            }
            else
          */
            if (LEACL_is_role_slave(pacl) && 
                (0 == LOCAL_FEATURE_is_supported_slave_initiated_features_exchange()) )
            {
                send_04_hciEvent_3e_04_LE_read_remote_feature_complete(pacl, 0x11); //ERR_11_UNSUPPORTED_FEATURE_OR_PARAM_VALUE
            }
            else
            if (LEACL_is_role_slave(pacl) && 
                LEACL_is_remote_said_unknownType(pacl, LL_0E_SLAVE_FEATURE_REQ) )
            {
                send_04_hciEvent_3e_04_LE_read_remote_feature_complete(pacl, 0x1A); //ERR_1A_UNSUPPORTED_REMOTE_FEATURE
            }
            else {
                llcmsgQ_push( LLCMSG_INITIATE_LLC_FEATURE_EXCHANGE_PROCEDURE, 
                             LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                             0,//length
                             (uint8_t *)0
                           );
            }
        }
            return (done);
    }
}

bool hcmd_2017_le_encrypt(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2017_LE_encrypt_TypeDef *pCmd2017 = (hcicmd_2017_LE_encrypt_TypeDef *)pCmd;
    bool done;
        uint8_t key[16], state[16];
        ht_memory_copy( key  +0, pCmd2017->key           +0, 16 );
        ht_memory_copy( state+0, pCmd2017->plaintext_data+0, 16 );
      //
        aes128_Encrypt_use_32f87251_engine(state, key);
      /*
        aes128_encrypt___lsbyte_to_msbyte(state, key);
      */
    ///////////////////////////////
    hcievt_0e_cmd_complete_leEncrypt_TypeDef evt0e2017 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2017)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2017    = 0x2017,//(OGF_08_LE<<10)|OCF_017_LE_ENCRYPT,//opcode 0x2017
        .status             = 0x00,  //ERR_00_SUCCESS
      //.encrypted_data[]   = 
    };
        ht_memory_copy( evt0e2017.encrypted_data+0, state+0, 16 );
            done = send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2017);
    return (done);
}

bool hcmd_2019_le_enable_encryption(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2019_LE_enable_encryption_TypeDef *pCmd2019 = (hcicmd_2019_LE_enable_encryption_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
            return send_04_hciEvent_0f_command_status(0x2019, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
    else
    {
            done = send_04_hciEvent_0f_command_status(0x2019, 0x00); //ERR_00_SUCCESS
        if (done) {
                ht_memory_copy( pacl->encrypt.rand, pCmd2019->random_number,          8 );
                ht_memory_copy( pacl->encrypt.ediv, pCmd2019->encrypted_diversifier,  2 );
                ht_memory_copy( pacl->encrypt.LTK,  pCmd2019->long_term_key,         16 );
            if (pacl->flag1.field.Tx_aesccm_enabled == 0 &&
                pacl->flag1.field.Rx_aesccm_enabled == 0 )
            {
                llcmsgQ_push(LLCMSG_INITIATE_LLC_START_ENCRYPTION_PROCEDURE, 
                             LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                             0,//length
                             (uint8_t *)0
                           );
            }
            else
            {
                llcmsgQ_push( LLCMSG_INITIATE_LLC_PAUSE_ENCRYPTION_PROCEDURE, 
                             LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                             0,//length
                             (uint8_t *)0
                           );
            }
        }
            return (done);
    }
}

bool hcmd_201a_le_LTK_request_reply(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_201a_LE_LTK_request_reply_TypeDef *pCmd201a = (hcicmd_201a_LE_LTK_request_reply_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0e_command_complete(0x201a, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
            ht_memory_copy( pacl->encrypt.LTK, pCmd201a->long_term_key, 16 );
            calc_pacl_encrypt_SK(pacl);
            
          //done = 1;// The Lower Tester  //LL/SEC/MAS/BI-03-C [Master Encryption Setup: Missing Request]      ERR_22_LL_RESPONSE_TIMEOUT
            done = TxLLcPDU_05_start_enc_req(pacl);
        if (done) {
                //Slave  Rx LL_03_ENC_REQ               start RespondProcedure
                //       Tx LL_04_ENC_RSP
                //       tx request event to host, wait reply $$$
                //       Tx LL_05_START_ENC_REQ
                //       Rx LL_06_START_ENC_RSP         complete RespondProcedure
                //       Tx LL_06_START_ENC_RSP
            send_04_hciEvent_0e_command_complete(0x201a, 0x00); //ERR_00_SUCCESS
        }
    return (done);
}

bool hcmd_201b_le_LTK_request_negative_reply(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
  //hcicmd_201b_LE_LTK_request_negative_reply_TypeDef *pCmd201b = (hcicmd_201b_LE_LTK_request_negative_reply_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0e_command_complete(0x201b, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
      //LL/SEC/SLA/BV-04-C [Slave  Sending   LL_REJECT_IND]       ERR_06_PIN_OR_KEY_MISSING
      //LL/SEC/SLA/BV-11-C [Slave  Sending   LL_REJECT_EXT_IND]   ERR_06_PIN_OR_KEY_MISSING
      //LL/SEC/MAS/BV-03-C [Master Receiving LL_REJECT_IND]       ERR_1A_UNSUPPORTED_REMOTE_FEATURE
      //LL/SEC/MAS/BV-11-C [Master Receiving LL_REJECT_EXT_IND]   ERR_06_PIN_OR_KEY_MISSING
                   send_04_hciEvent_0e_command_complete(0x201b, 0x00); //ERR_00_SUCCESS
            done = TxLLcPDU_11RejectExtInd_or_0dRejectInd(pacl, LL_03_ENC_REQ, //rejectOpcode
                                                                0x06           //errorCode   ERR_06_PIN_OR_KEY_MISSING
                                                         );
        if (done) {
                //Slave  Rx LL_03_ENC_REQ               start RespondProcedure
                //       Tx LL_04_ENC_RSP
                //       tx request event to host, wait reply $$$
                //       Tx LL_REJECT_EXT_IND           complete RespondProcedure
                LLC_RESPOND_PROCEDURE_end_complete(pacl);   //complete RespondProcedure
        }
    return (done);
}

bool hcmd_201c_le_read_supported_states(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcievt_0e_cmd_complete_leReadLocalSupportedStates_TypeDef evt0e201c =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e201c)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_201c    = 0x201c,//(OGF_08_LE<<10)|OCF_01C_LE_R_SUPPORTED_STATES,//opcode 0x201c
        .status             = 0x00,  //ERR_00_SUCCESS
        .le_states[0]       = 0x01,//LE_States[0]  value 0x0000000000000001: only "Non-connectable Advertising State"
        .le_states[1]       = 0x00,//LE_States[1]
        .le_states[2]       = 0x00,//LE_States[2]
        .le_states[3]       = 0x00,//LE_States[3]
        .le_states[4]       = 0x00,//LE_States[4]
        .le_states[5]       = 0x00,//LE_States[5]
        .le_states[6]       = 0x00,//LE_States[6]
        .le_states[7]       = 0x00 //LE_States[7]
    };
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e201c);
}

bool hcmd_201d_le_receiver_test_v1(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    uint8_t phyRx_rx_channel;
    
    hcicmd_201d_LE_receiver_test_v1_TypeDef *pCmd201d = (hcicmd_201d_LE_receiver_test_v1_TypeDef *)pCmd;
    phyRx_rx_channel =(pCmd201d->rx_channel+1)*2;
    init_0rf_1adc_dtm_mode(0x01);   
    init_0rf_2in1_5602B_set_rfch(phyRx_rx_channel);
    init_2fpga_directed_test_mode(0,phyRx_rx_channel,0x01,0x00,0x00,0xFF);    		
    leTestEnd_evt.num_packets   =0;
    debug_wantedpower = debug_wantedpower + 2;
	#if Debug_COUNT_CRCF == 1	
        debug_leTestEnd_evt.num_packets        = 0x0000; //Number_Of_Packets:      Number_Of_Packets for a transmitter test shall be reported as 0x0000  
        debug_leTestEnd_evt.num_crcf	       = 0x0000;
        debug_leTestEnd_evt.num_synclost       = 0x0000;  
	#endif         
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x04;                 // 0x00:master, 0x01:slave 0x04:dtm_rx
    pacl->channel = phyRx_rx_channel;
    pacl->debug_power = debug_wantedpower;
    debug_hcfsm_state_ST_0();    
    return send_04_hciEvent_0e_command_complete(0x201d, 0x00); //ERR_00_SUCCESS
}

bool hcmd_201e_le_transmitter_test_v1(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    int i;
    hcicmd_201e_LE_transmitter_test_v1_TypeDef *pCmd201e;
    pCmd201e = (hcicmd_201e_LE_transmitter_test_v1_TypeDef *)pCmd;
    
    leconfig_txTest.tx_channel       =(pCmd201e->tx_channel+1)*2; // N = (freq - 2402) / 2 ,   Range: 0x00--0x27. Frequency Range : 2402 MHz to 2480 MHz
    leconfig_txTest.test_data_length = pCmd201e->test_data_length;
    leconfig_txTest.packet_payload   = pCmd201e->packet_payload;
                                     //              0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
                                     //              0x01:Repeated "11110000"(in transmission order) sequence
                                     //              0x02:Repeated "10101010"(in transmission order) sequence
                                     //              0x03:PRBS15 sequence
                                     //              0x04:Repeated "11111111"(in transmission order) sequence
                                     //              0x05:Repeated "00000000"(in transmission order) sequence
                                     //              0x06:Repeated "00001111"(in transmission order) sequence
                                     //              0x07:Repeated "01010101"(in transmission order) sequence
                                     //            others:RFU
    if     ( pCmd201e->packet_payload==0x00) {                  ;} //0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
    else if( pCmd201e->packet_payload==0x01) {phyTx_pdudata=0x0F;} //0x01:Repeated "11110000"(in transmission order) sequence
    else if( pCmd201e->packet_payload==0x02) {phyTx_pdudata=0x55;} //0x02:Repeated "10101010"(in transmission order) sequence
    else if( pCmd201e->packet_payload==0x03) {                  ;} //0x03:PRBS15 sequence
    else if( pCmd201e->packet_payload==0x04) {phyTx_pdudata=0xFF;} //0x04:Repeated "11111111"(in transmission order) sequence
    else if( pCmd201e->packet_payload==0x05) {phyTx_pdudata=0x00;} //0x05:Repeated "00000000"(in transmission order) sequence
    else if( pCmd201e->packet_payload==0x06) {phyTx_pdudata=0xF0;} //0x06:Repeated "00001111"(in transmission order) sequence
    else if( pCmd201e->packet_payload==0x07) {phyTx_pdudata=0xAA;} //0x07:Repeated "01010101"(in transmission order) sequence
    if	   ( pCmd201e->packet_payload==0x00){
   		for(i=0; i<pCmd201e->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn9_seq[i];
    		}
    }
    else if( pCmd201e->packet_payload==0x03){
    		for(i=0; i<pCmd201e->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn15_seq[i];
    		}
    }		
    else 	{ 
    	        for(i=0; i<pCmd201e->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= phyTx_pdudata;
    		}	
    }
    
    init_0rf_2in1_5602B_set_rfch(leconfig_txTest.tx_channel);    	
    init_2fpga_directed_test_mode(1,leconfig_txTest.tx_channel,0x01,0x00,0x00,0xFF);     
    
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x03;                 // 0x00:master, 0x01:slave  0x03:DTM_TX 0x04:DTM_RX

    debug_hcfsm_state_ST_0(); 
        
    return send_04_hciEvent_0e_command_complete(0x201e, ERR_00_SUCCESS);

}

bool hcmd_201f_le_test_end(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    debug_hcfsm_state_ST_Sean_IDLE();
    
    
    #if Debug_COUNT_CRCF == 0 
     return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&leTestEnd_evt);
    #elif Debug_COUNT_CRCF == 1 
     debug_leTestEnd_evt.num_synclost = 0x03E8 - (debug_leTestEnd_evt.num_packets + debug_leTestEnd_evt.num_crcf);
     uart_putchar(0x04);
     uart_putchar(0x0E);
     uart_putchar(0x06);
     uart_putchar(0x01);
     uart_putchar(0x1F);
     uart_putchar(0x20);
     uart_putchar(0x00);
     uart_putchar(debug_leTestEnd_evt.num_packets);
     uart_putchar(debug_leTestEnd_evt.num_packets>>8);
     uart_putchar(debug_leTestEnd_evt.num_crcf);
     uart_putchar(debug_leTestEnd_evt.num_crcf>>8);
     uart_putchar(debug_leTestEnd_evt.num_synclost);
     uart_putchar(debug_leTestEnd_evt.num_synclost>>8);    
     return 1;       
   #endif  
}

bool hcmd_2020_le_remote_conn_param_req_reply(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    bool done;
    hcicmd_2020_LE_remote_conn_param_req_reply_TypeDef *pCmd2020 = (hcicmd_2020_LE_remote_conn_param_req_reply_TypeDef *)pCmd;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x2020, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }

    hcievt_0e_cmd_complete_leRemoteConnParamReqReply_TypeDef evt0e2020 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2020)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2020    = 0x2020,//(OGF_08_LE<<10)|OCF_020_LE_REMOTE_CONN_PARAM_REQ_REPLY,//opcode 0x2020
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = handle //
    };
        ht_write_hword( (uint8_t *)&(evt0e2020.conn_handle  ), handle );
        done = send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2020);
    /////////
    //save
  //if (done)
    {
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
      /*
        pacl->pTemplateConnParam->host.interval_min        = pCmd2020->conn_interval_min;
        pacl->pTemplateConnParam->host.interval_max        = pCmd2020->conn_interval_max;
        pacl->pTemplateConnParam->host.max_slave_latency   = pCmd2020->max_slave_latency;
        pacl->pTemplateConnParam->host.supervision_timeout = pCmd2020->supervision_timeout;
        pacl->pTemplateConnParam->host.min_CE_length       = pCmd2020->min_CE_length;
        pacl->pTemplateConnParam->host.max_CE_length       = pCmd2020->max_CE_length;
      */
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.interval_min       ), ht_read_hword((uint8_t *)&(pCmd2020->conn_interval_min)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.interval_max       ), ht_read_hword((uint8_t *)&(pCmd2020->conn_interval_max)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.max_slave_latency  ), ht_read_hword((uint8_t *)&(pCmd2020->max_slave_latency)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.supervision_timeout), ht_read_hword((uint8_t *)&(pCmd2020->supervision_timeout)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.min_CE_length      ), ht_read_hword((uint8_t *)&(pCmd2020->min_CE_length)) );
        ht_write_hword( (uint8_t *)&(pacl->pTemplateConnParam->host.max_CE_length      ), ht_read_hword((uint8_t *)&(pCmd2020->max_CE_length)) );
      //
    }
    }//end done
    /////////
  //if (done)
    {
        if (LEACL_is_role_master(pacl))
        {
            //Master shall not send the LL_10_CONNECTION_PARAM_RSP
                ////not use push msg to send LL_00_CONNECTION_UPDATE_IND, because push q may queue after other msgs which initiate llc procedure
                ////           directly send LL_00_CONNECTION_UPDATE_IND
                ////done = llcmsgQ_push( LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE,       //send LL_00_CONNECTION_UPDATE_IND
                ////                    LEACL_conn_handle(pacl) | (((uint32_t)0x00)<<16),   //param[2] 0x00:autonomously by LL, 0x01:by Host
                ////                    0,//length
                ////                    (uint8_t *)0
                ////                  );
                    done = TxLLcPDU_00_connection_update_ind(pacl);
                if (done) {
                    //Master Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure
                    //             tx request event to host, wait reply
                    //             Tx LL_00_CONNECTION_UPDATE_IND    start InitiateProcedure__________________Conn Update procedure
                    LLC_RESPOND_PROCEDURE_end_complete(pacl);   //complete RespondProcedure
                    LLC_INITIATE_PROCEDURE_start(pacl, LL_00_CONNECTION_UPDATE_IND | FLAG_INVOLVE_INSTANT); //Conn Update procedure
                }
        }
        else
        {
            //Slave shall NOT send LL_00_CONNECTION_UPDATE_IND
                    done = TxLLcPDU_10_connection_param_rsp(pacl);
                if (done) {
                    //Slave  Rx LL_0F_CONNECTION_PARAM_REQ     start RespondProcedure
                    //       tx request event to host, wait reply
                    //       Tx LL_10_CONNECTION_PARAM_RSP  complete RespondProcedure
                    LLC_RESPOND_PROCEDURE_end_complete(pacl);   //complete RespondProcedure
                }
        }
    }
    return (done);
}

bool hcmd_2021_le_remote_conn_param_req_negative_reply(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    bool done;
  //hcicmd_2021_LE_remote_conn_param_req_negative_reply_TypeDef *pCmd2021 = (hcicmd_2021_LE_remote_conn_param_req_negative_reply_TypeDef *)pCmd;
  //The Host shall only use the Error Code 0x3B (Unacceptable Connection Parameters) in order to reject the request
    LEACL_TypeDef *pacl;
    uint16_t handle;
  //uint8_t  reason;
    handle = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
  //reason = ht_read_byte ( pCmd->params+2 ); //           Reason: Size: 1 octet
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        return send_04_hciEvent_0f_command_status(0x2021, 0x02); //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }

    hcievt_0e_cmd_complete_leRemoteConnParamReqNegativeReply_TypeDef evt0e2021 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2021)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2021    = 0x2021,//(OGF_08_LE<<10)|OCF_021_LE_REMOTE_CONN_PARAM_REQ_NEG_REPLY,//opcode 0x2021
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = handle //
    };
        ht_write_hword( (uint8_t *)&(evt0e2021.conn_handle  ), handle );
        done = send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2021);
  //if (done)
    {
        if (LEACL_is_role_master(pacl))
        {
            //Master shall not send the LL_10_CONNECTION_PARAM_RSP
            done = TxLLcPDU_11_reject_ext_ind(pacl, LL_0F_CONNECTION_PARAM_REQ, //rejectOpcode
                                                    0x3B                        //errorCode   ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                             );
        }
        else
        {
            //Slave shall NOT send LL_00_CONNECTION_UPDATE_IND
            done = TxLLcPDU_11_reject_ext_ind(pacl, LL_0F_CONNECTION_PARAM_REQ, //rejectOpcode
                                                    0x3B                        //errorCode   ERR_3B_UNACCEPTABLE_CONN_PARAMS
                                             );
        }
    }
    return (done);
}

/**
    //LL/CON/MAS/BV-73-C [Master Data Length Update - Responding to Data Length Update Procedure; LE 1M PHY]
    //LL/CON/SLA/BV-77-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE 1M PHY]
    //LL/CON/MAS/BV-74-C [Master Data Length Update - Initiating Data Length Update Procedure; LE 1M PHY]
    //LL/CON/SLA/BV-78-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE 1M PHY]
    //LL/CON/MAS/BV-75-C [Master Data Length Update -  Slave does not support; LE 1M PHY]
    //LL/CON/SLA/BV-79-C [ Slave Data Length Update - Master does not support; LE 1M PHY]
    //LL/CON/MAS/BV-76-C [Master Data Length Update - Responding to Data Length Update Procedure; LE 2M PHY]
    //LL/CON/SLA/BV-80-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE 2M PHY]
    //LL/CON/MAS/BV-77-C [Master Data Length Update - Initiating Data Length Update Procedure; LE 2M PHY]
    //LL/CON/SLA/BV-81-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE 2M PHY]
    //LL/CON/MAS/BV-78-C [Master Data Length Update - Responding to Data Length Update Procedure; LE Coded PHY]
    //LL/CON/SLA/BV-82-C [ Slave Data Length Update - Responding to Data Length Update Procedure; LE Coded PHY]
    //LL/CON/MAS/BV-79-C [Master Data Length Update - Initiating Data Length Update Procedure; LE Coded PHY]
    //LL/CON/SLA/BV-83-C [ Slave Data Length Update - Initiating Data Length Update Procedure; LE Coded PHY]
    //LL/CON/MAS/BV-80-C [Master Data Length Update -  Slave does not support; LE Coded PHY]
    //LL/CON/SLA/BV-84-C [ Slave Data Length Update - Master does not support; LE Coded PHY]
    //4.3.6.32 Data Length Update - Preserve Parameters After a PHY Change
    //                              LL/CON/SLA/BV-129-C Slave LE 2M PHY 
    //                              LL/CON/SLA/BV-130-C Slave LE Coded PHY 
    //                              LL/CON/MAS/BV-126-C Master LE 2M PHY 
    //                              LL/CON/MAS/BV-127-C Master LE Coded PHY
    //4.3.6.33 Data Length Update - Retransmission During an Update
    //                              LL/CON/SLA/BV-131-C Slave
    //                              LL/CON/MAS/BV-128-C Master
    //4.3.6.34 Data Length Update - Handling Invalid Data Length Responses
    //                              LL/CON/SLA/BI-10-C Slave LE 1M PHY
    //                              LL/CON/SLA/BI-11-C Slave LE 2M PHY
    //                              LL/CON/SLA/BI-12-C Slave LE Coded PHY
    //                              LL/CON/MAS/BI-07-C Master LE 1M PHY
    //                              LL/CON/MAS/BI-08-C Master LE 2M PHY
    //                              LL/CON/MAS/BI-09-C Master LE Coded PHY
    //4.3.6.35 Data Length Update - Peer Does Not Support LE Coded PHY
    //                              LL/CON/SLA/BV-132-C
    //                              LL/CON/MAS/BV-129-C
 **/
bool hcmd_2022_le_set_data_length(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
    //C8 Mandatory if LE Feature (LE Data Packet Length Extension) is supported, otherwise optional
    //allows the Host to suggest connMaxTxOctets and connMaxTxTime  to be used for LL Data PDUs on a given connection
{
  //hcicmd_2022_LE_set_data_length_TypeDef *pCmd2022 = (hcicmd_2022_LE_set_data_length_TypeDef *)pCmd;
    bool done;
    LEACL_TypeDef *pacl;
    uint16_t handle;
    uint16_t tx_octets;
    uint16_t tx_time;
    handle    = ht_read_hword( pCmd->params+0 ); //Connection_Handle: Size: 2 octets (12 bits meaningful)
    tx_octets = ht_read_hword( pCmd->params+2 ); //Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    tx_time   = ht_read_hword( pCmd->params+4 ); //Range 0x0148-0x4290
    hcievt_0e_cmd_complete_leSetDataLength_TypeDef evt0e2022 =
    {
        .event_code_0e      = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length = sizeof(evt0e2022)-2,
        .num_hci_cmd_pkts   = 0x01,  //allowed cmd packets
        .cmd_opcode_2022    = 0x2022,//(OGF_08_LE<<10)|OCF_022_LE_SET_DATA_LEN,//opcode 0x2022
        .status             = 0x00,  //ERR_00_SUCCESS
      //.conn_handle        = handle         //Return parameters   Connection_Handle: Size: 2 octets 
    };
        ht_write_hword( (uint8_t *)&(evt0e2022.conn_handle  ), handle );
        pacl = leacl_with_connHandle( handle );
    if (pacl == 0)
    {
        evt0e2022.status    = 0x02; //ERR_02_UNKNOWN_CONN_IDENTIFIER, the connection does not exist
    }
        done = send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2022);
    if (done) {
        if (pacl != 0) {
        if (pacl->connMaxTxOctets != tx_octets ||
            pacl->connMaxTxTime   != tx_time )
        {
            //Host suggest different "connMaxTxOctets and connMaxTxTime"
            pacl->connMaxTxOctets = tx_octets;
            pacl->connMaxTxTime   = tx_time;
            llcmsgQ_push( LLCMSG_INITIATE_LLC_DATA_LENGTH_UPDATE_PROCEDURE, //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP
                         LEACL_conn_handle(pacl) | (((uint32_t)0x01)<<16), //param[2] 0x00:autonomously by LL, 0x01:by Host
                         0,//length
                         (uint8_t *)0
                       );
        }
        }
    }
    return (done);
}

bool hcmd_2023_le_read_suggested_default_data_length(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
    //C8 Mandatory if LE Feature (LE Data Packet Length Extension) is supported, otherwise optional
{
    hcievt_0e_cmd_complete_leReadSuggestedDefaultDataLength_TypeDef evt0e2023 =
    {
        .event_code_0e          = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length     = sizeof(evt0e2023)-2,
        .num_hci_cmd_pkts       = 0x01,  //allowed cmd packets
        .cmd_opcode_2023        = 0x2023,//(OGF_08_LE<<10)|OCF_023_LE_R_SUGGEST_DEFAULT_DATA_LEN,//opcode 0x2023
        .status                 = 0x00,  //ERR_00_SUCCESS
      //.connInitialMaxTxOctets = leconfig_defaultDataLength.connInitialMaxTxOctets,
      //.connInitialMaxTxTime   = leconfig_defaultDataLength.connInitialMaxTxTime
    };
        ht_write_hword( (uint8_t *)&(evt0e2023.connInitialMaxTxOctets  ), leconfig_defaultDataLength.connInitialMaxTxOctets );
        ht_write_hword( (uint8_t *)&(evt0e2023.connInitialMaxTxTime    ), leconfig_defaultDataLength.connInitialMaxTxTime   );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2023);
}

bool hcmd_2024_le_write_suggested_default_data_length(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
    //C8 Mandatory if LE Feature (LE Data Packet Length Extension) is supported, otherwise optional
{
    hcicmd_2024_LE_write_suggested_default_data_length_TypeDef *pCmd2024 = (hcicmd_2024_LE_write_suggested_default_data_length_TypeDef *)pCmd;
        leconfig_defaultDataLength.connInitialMaxTxOctets = pCmd2024->connInitialMaxTxOctets;
        leconfig_defaultDataLength.connInitialMaxTxTime   = pCmd2024->connInitialMaxTxTime  ;
    return send_04_hciEvent_0e_command_complete(0x2024, 0x00); //ERR_00_SUCCESS  //Return parameters   ...
}

bool hcmd_202f_le_read_maximum_data_length(HCI_01_COMMAND_PACKET_TypeDef *pCmd) //IUT supported
    //C8 Mandatory if LE Feature (LE Data Packet Length Extension) is supported, otherwise optional
    //allows the Host to read the Controller's supportedMaxTxOctets and supportedMaxTxTime, supportedMaxRxOctets, and supportedMaxRxTime
{
    hcievt_0e_cmd_complete_leReadMaximumDataLength_TypeDef evt0e202f =
    {
        .event_code_0e         = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length    = sizeof(evt0e202f)-2,
        .num_hci_cmd_pkts      = 0x01,  //allowed cmd packets
        .cmd_opcode_202f       = 0x202f,//(OGF_08_LE<<10)|OCF_02F_LE_R_MAXIMUM_DATA_LEN,//opcode 0x202f
        .status                = 0x00,  //ERR_00_SUCCESS
      //.supportedMaxTxOctets  = leconfig_maximumDataLength.supportedMaxTxOctets,
      //.supportedMaxTxTime    = leconfig_maximumDataLength.supportedMaxTxTime,
      //.supportedMaxRxOctets  = leconfig_maximumDataLength.supportedMaxRxOctets,
      //.supportedMaxRxTime    = leconfig_maximumDataLength.supportedMaxRxTime,
    };
        ht_write_hword( (uint8_t *)&(evt0e202f.supportedMaxTxOctets  ), leconfig_maximumDataLength.supportedMaxTxOctets );
        ht_write_hword( (uint8_t *)&(evt0e202f.supportedMaxTxTime    ), leconfig_maximumDataLength.supportedMaxTxTime   );
        ht_write_hword( (uint8_t *)&(evt0e202f.supportedMaxRxOctets  ), leconfig_maximumDataLength.supportedMaxRxOctets );
        ht_write_hword( (uint8_t *)&(evt0e202f.supportedMaxRxTime    ), leconfig_maximumDataLength.supportedMaxRxTime   );
    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e202f);
}

bool hcmd_2033_le_receiver_test_v2(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    uint8_t phyRx_rx_channel;	
	
    hcicmd_2033_LE_receiver_test_v2_TypeDef *pCmd2033;
    pCmd2033 = (hcicmd_2033_LE_receiver_test_v2_TypeDef *)pCmd;
    
    phyRx_rx_channel =(pCmd2033->rx_channel+1)*2;
    init_0rf_1adc_dtm_mode(pCmd2033->phy);
    init_0rf_2in1_5602B_set_rfch(phyRx_rx_channel);
    init_2fpga_directed_test_mode(0,phyRx_rx_channel,pCmd2033->phy,pCmd2033->modulation_index,0x00,0xFF);    	
    //init_2fpga_directed_test_mode(0,phyRx_rx_channel,0x02,0x00,0x00,0xFF);    	
    leTestEnd_evt.num_packets   =0; 
    debug_wantedpower = debug_wantedpower + 2;   
    
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x04;                 // 0x00:master, 0x01:slave 0x04:dtm_rx
    pacl->channel = phyRx_rx_channel;
    pacl->debug_power = debug_wantedpower;	
    debug_hcfsm_state_ST_0(); 
    
    return send_04_hciEvent_0e_command_complete(0x2033, ERR_00_SUCCESS);

}

bool hcmd_2034_le_transmitter_test_v2(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    int i;
    hcicmd_2034_LE_transmitter_test_v2_TypeDef *pCmd2034;
    pCmd2034 = (hcicmd_2034_LE_transmitter_test_v2_TypeDef *)pCmd;
    
    leconfig_txTest.tx_channel       =(pCmd2034->tx_channel+1)*2; // N = (freq - 2402) / 2 ,   Range: 0x00--0x27. Frequency Range : 2402 MHz to 2480 MHz
    leconfig_txTest.test_data_length = pCmd2034->test_data_length;
    leconfig_txTest.packet_payload   = pCmd2034->packet_payload;
                                     //              0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
                                     //              0x01:Repeated "11110000"(in transmission order) sequence
                                     //              0x02:Repeated "10101010"(in transmission order) sequence
                                     //              0x03:PRBS15 sequence
                                     //              0x04:Repeated "11111111"(in transmission order) sequence
                                     //              0x05:Repeated "00000000"(in transmission order) sequence
                                     //              0x06:Repeated "00001111"(in transmission order) sequence
                                     //              0x07:Repeated "01010101"(in transmission order) sequence
                                     //            others:RFU
    if     ( pCmd2034->packet_payload==0x00) {                  ;} //0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
    else if( pCmd2034->packet_payload==0x01) {phyTx_pdudata=0x0F;} //0x01:Repeated "11110000"(in transmission order) sequence
    else if( pCmd2034->packet_payload==0x02) {phyTx_pdudata=0x55;} //0x02:Repeated "10101010"(in transmission order) sequence
    else if( pCmd2034->packet_payload==0x03) {                  ;} //0x03:PRBS15 sequence
    else if( pCmd2034->packet_payload==0x04) {phyTx_pdudata=0xFF;} //0x04:Repeated "11111111"(in transmission order) sequence
    else if( pCmd2034->packet_payload==0x05) {phyTx_pdudata=0x00;} //0x05:Repeated "00000000"(in transmission order) sequence
    else if( pCmd2034->packet_payload==0x06) {phyTx_pdudata=0xF0;} //0x06:Repeated "00001111"(in transmission order) sequence
    else if( pCmd2034->packet_payload==0x07) {phyTx_pdudata=0xAA;} //0x07:Repeated "01010101"(in transmission order) sequence
    if	   ( pCmd2034->packet_payload==0x00){
   		for(i=0; i<pCmd2034->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn9_seq[i];
    		}
    }
    else if( pCmd2034->packet_payload==0x03){
    		for(i=0; i<pCmd2034->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn15_seq[i];
    		}
    }		
    else 	{ 
    	        for(i=0; i<pCmd2034->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= phyTx_pdudata;
    		}	
    }	
    init_0rf_2in1_5602B_set_rfch(leconfig_txTest.tx_channel);    	
    init_2fpga_directed_test_mode(1,leconfig_txTest.tx_channel,pCmd2034->phy,0x00,0x00,0xFF);      
    
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x03;                 // 0x00:master, 0x01:slave 0x03:DTM_TX 0x04:DTM_RX
    debug_hcfsm_state_ST_0();     
    
    return send_04_hciEvent_0e_command_complete(0x2034, ERR_00_SUCCESS);
	

}

bool hcmd_2036_le_set_extended_advertising_parameters(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2036_LE_set_ext_adv_parameters_TypeDef *pCmd2036 = (hcicmd_2036_LE_set_ext_adv_parameters_TypeDef *)pCmd;
    
     /*
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.adv_handle       ),        	 ht_read_byte ((uint8_t *)&(pCmd2036->adv_handle              )) );
    ht_write_hword( (uint8_t *)&(leconfig_ExtAdvParam.adv_event_properties ),            ht_read_hword((uint8_t *)&(pCmd2036->adv_event_properties    )) );
    leconfig_ExtAdvParam.primary_adv_interval_min = pCmd2036->primary_adv_interval_min[2];
    leconfig_ExtAdvParam.primary_adv_interval_min = (leconfig_ExtAdvParam.primary_adv_interval_min<<8)| pCmd2036->primary_adv_interval_min[1];
    leconfig_ExtAdvParam.primary_adv_interval_min = (leconfig_ExtAdvParam.primary_adv_interval_min<<8)| pCmd2036->primary_adv_interval_min[0];  
    leconfig_ExtAdvParam.primary_adv_interval_max = pCmd2036->primary_adv_interval_max[2];
    leconfig_ExtAdvParam.primary_adv_interval_max = (leconfig_ExtAdvParam.primary_adv_interval_max<<8)| pCmd2036->primary_adv_interval_max[1];
    leconfig_ExtAdvParam.primary_adv_interval_max = (leconfig_ExtAdvParam.primary_adv_interval_max<<8)| pCmd2036->primary_adv_interval_max[0];  
    */      
    //ht_memory_copy(leconfig_ExtAdvParam.primary_adv_interval_max, pCmd2036->primary_adv_interval_max, 3);    
    /*
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.primary_adv_channel_map       ),   ht_read_byte ((uint8_t *)&(pCmd2036->primary_adv_channel_map )) ); 
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.own_address_type       ),        	 ht_read_byte ((uint8_t *)&(pCmd2036->own_address_type        )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.peer_address_type       ),         ht_read_byte ((uint8_t *)&(pCmd2036->peer_address_type       )) );
    ht_memory_copy(leconfig_ExtAdvParam.peer_address, pCmd2036->peer_address, 6);
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.adv_filter_policy       ),         ht_read_byte ((uint8_t *)&(pCmd2036->adv_filter_policy       )) ); 
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.adv_tx_power       ),              ht_read_byte ((uint8_t *)&(pCmd2036->adv_tx_power            )) ); 
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.primary_adv_phy       ),           ht_read_byte ((uint8_t *)&(pCmd2036->primary_adv_phy         )) ); 
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.secondary_adv_max_skip       ),    ht_read_byte ((uint8_t *)&(pCmd2036->secondary_adv_max_skip  )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.secondary_adv_max_phy       ),     ht_read_byte ((uint8_t *)&(pCmd2036->secondary_adv_max_phy   )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.adv_sid       ),                   ht_read_byte ((uint8_t *)&(pCmd2036->adv_sid                 )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ExtAdvParam.scan_req_notification_enable ),    ht_read_byte ((uint8_t *)&(pCmd2036->scan_req_notification_enable)) );
   */            
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x05;                 // 0x00:master, 0x01:slave  0x05:ext_adv_tx
    pacl->aclConnHandle = pCmd2036->adv_handle; // 
    debug_hcfsm_state_ST_0();        
   
    hcievt_0e_cmd_complete_leSetExtAdvParameters_TypeDef evt0e2036 =
    {
        .event_code_0e         = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length    = sizeof(evt0e2036)-2,
        .num_hci_cmd_pkts      = 0x01,  //allowed cmd packets
        .cmd_opcode_2036       = 0x2036,//(OGF_08_LE<<10)|OCF_02F_LE_R_MAXIMUM_DATA_LEN,//opcode 0x2058
        .status                = 0x00,  //ERR_00_SUCCESS
	.selectedTxPower       = 0x05,	//Txpower
    };

    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2036);
}

bool hcmd_2039_le_set_extended_advertising_enable(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{	
    uint16_t handle;	    
    hcicmd_2039_LE_set_ext_adv_enable_TypeDef *pCmd2039 = (hcicmd_2039_LE_set_ext_adv_enable_TypeDef *)pCmd;
   
    handle = pCmd2039->adv_handle  ;
    LEACL_TypeDef *pacl;    
    pacl = leacl_with_connHandle( handle ); 
    pacl->ext_adv_enable = 0x01;
    debug_hcfsm_state_ST_0(); 
             

    
    return send_04_hciEvent_0e_command_complete(0x2039, ERR_00_SUCCESS);   
}

bool hcmd_203e_le_set_periodic_advertising_parameters(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    uint16_t handle;	
    hcicmd_203E_LE_set_periodic_adv_parameters_TypeDef *pCmd203e = (hcicmd_203E_LE_set_periodic_adv_parameters_TypeDef *)pCmd;

    ht_write_byte ( (uint8_t *)&(leconfig_PeriodicAdvParam.adv_handle       ),         	  ht_read_byte ((uint8_t *)&(pCmd203e->adv_handle                 )) );    
    ht_write_hword( (uint8_t *)&(leconfig_PeriodicAdvParam.periodic_adv_interval_min ),   ht_read_hword((uint8_t *)&(pCmd203e->periodic_adv_interval_min  )) );    
    ht_write_hword( (uint8_t *)&(leconfig_PeriodicAdvParam.periodic_adv_interval_max ),   ht_read_hword((uint8_t *)&(pCmd203e->periodic_adv_interval_max  )) );  
    ht_write_hword( (uint8_t *)&(leconfig_PeriodicAdvParam.periodic_adv_properties ),     ht_read_hword((uint8_t *)&(pCmd203e->periodic_adv_properties    )) ); 
    
    handle = pCmd203e->adv_handle  ;
    LEACL_TypeDef *pacl;    
    pacl = leacl_with_connHandle( handle ); 
	
    return send_04_hciEvent_0e_command_complete(0x203E, ERR_00_SUCCESS);
}

bool hcmd_2040_le_set_periodic_advertising_enable(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2040_LE_set_periodic_adv_enable_TypeDef *pCmd2040 = (hcicmd_2040_LE_set_periodic_adv_enable_TypeDef *)pCmd;         
    
    return send_04_hciEvent_0e_command_complete(0x2040, ERR_00_SUCCESS);
}

bool hcmd_204f_le_receiver_test_v3(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    uint8_t phyRx_rx_channel;	
    
    hcicmd_204F_LE_receiver_test_v3_TypeDef *pCmd204f;
    pCmd204f = (hcicmd_204F_LE_receiver_test_v3_TypeDef *)pCmd;

    phyRx_rx_channel =(pCmd204f->rx_channel+1)*2;
    init_0rf_1adc_dtm_mode(pCmd204f->phy);
    init_0rf_2in1_5602B_set_rfch(phyRx_rx_channel);
    init_2fpga_directed_test_mode(0,phyRx_rx_channel,pCmd204f->phy,pCmd204f->modulation_index,pCmd204f->expected_cte_length,pCmd204f->expected_cte_type);    		
    leTestEnd_evt.num_packets   = 0 ;    
    	
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x04;                 // 0x00:master, 0x01:slave 0x04:dtm_rx
    pacl->channel = phyRx_rx_channel;
    pacl->channelIndex = ChannelIndex_table[pCmd204f->rx_channel];
    pacl->dtm_rx_aod_mode = 0x01;
    pacl->cte_type = pCmd204f->expected_cte_type;
    pacl->cte_length = pCmd204f->expected_cte_length;
        
   if( pCmd204f->expected_cte_type==0x01){			//AOD 1 us slots
       pacl->cte_sample_count = 8+2+((pCmd204f->expected_cte_length-2)*4);
     }
   else if( pCmd204f->expected_cte_type==0x02){
       pacl->cte_sample_count = 8+1+((pCmd204f->expected_cte_length-2)*2);			
     }    
    
    debug_hcfsm_state_ST_0();    	

    return send_04_hciEvent_0e_command_complete(0x204f, ERR_00_SUCCESS);
}

bool hcmd_2050_le_transmitter_test_v3(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    int i;
    hcicmd_2050_LE_transmitter_test_v3_TypeDef *pCmd2050;
    pCmd2050 = (hcicmd_2050_LE_transmitter_test_v3_TypeDef *)pCmd;
    
    leconfig_txTest.tx_channel       =(pCmd2050->tx_channel+1)*2; // N = (freq - 2402) / 2 ,   Range: 0x00--0x27. Frequency Range : 2402 MHz to 2480 MHz
    leconfig_txTest.test_data_length = pCmd2050->test_data_length;
    leconfig_txTest.packet_payload   = pCmd2050->packet_payload;
                                     //              0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
                                     //              0x01:Repeated "11110000"(in transmission order) sequence
                                     //              0x02:Repeated "10101010"(in transmission order) sequence
                                     //              0x03:PRBS15 sequence
                                     //              0x04:Repeated "11111111"(in transmission order) sequence
                                     //              0x05:Repeated "00000000"(in transmission order) sequence
                                     //              0x06:Repeated "00001111"(in transmission order) sequence
                                     //              0x07:Repeated "01010101"(in transmission order) sequence
                                     //            others:RFU
    if     ( pCmd2050->packet_payload==0x00) {                  ;} //0x00:PRBS9 sequence "11111111100000111101..." (in transmission order)
    else if( pCmd2050->packet_payload==0x01) {phyTx_pdudata=0x0F;} //0x01:Repeated "11110000"(in transmission order) sequence
    else if( pCmd2050->packet_payload==0x02) {phyTx_pdudata=0x55;} //0x02:Repeated "10101010"(in transmission order) sequence
    else if( pCmd2050->packet_payload==0x03) {                  ;} //0x03:PRBS15 sequence
    else if( pCmd2050->packet_payload==0x04) {phyTx_pdudata=0xFF;} //0x04:Repeated "11111111"(in transmission order) sequence
    else if( pCmd2050->packet_payload==0x05) {phyTx_pdudata=0x00;} //0x05:Repeated "00000000"(in transmission order) sequence
    else if( pCmd2050->packet_payload==0x06) {phyTx_pdudata=0xF0;} //0x06:Repeated "00001111"(in transmission order) sequence
    else if( pCmd2050->packet_payload==0x07) {phyTx_pdudata=0xAA;} //0x07:Repeated "01010101"(in transmission order) sequence
    if	   ( pCmd2050->packet_payload==0x00){
   		for(i=0; i<pCmd2050->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn9_seq[i];
    		}
    }
    else if( pCmd2050->packet_payload==0x03){
    		for(i=0; i<pCmd2050->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= pn15_seq[i];
    		}
    }		
    else 	{ 
    	        for(i=0; i<pCmd2050->test_data_length; i++) {
      	 		 leconfig_txTestData.advData[i]= phyTx_pdudata;
    		}	
    }	
    
    init_0rf_2in1_5602B_set_rfch(leconfig_txTest.tx_channel);    	
    init_2fpga_directed_test_mode(1,leconfig_txTest.tx_channel,pCmd2050->phy,0x00,pCmd2050->cte_length,pCmd2050->cte_type);      

         
    
    LEACL_TypeDef *pacl;
    pacl = leacl_alloc();
    pacl->role = 0x03;                 // 0x00:master, 0x01:slave  0x03:DTM_TX 0x04:DTM_RX
    pacl->dtm_tx_aoa_mode = 0x01;    
    pacl->cte_type        = pCmd2050->cte_type;
    pacl->cte_length      = pCmd2050->cte_length;   

    debug_hcfsm_state_ST_0();         
    return send_04_hciEvent_0e_command_complete(0x2050, ERR_00_SUCCESS);

}

bool hcmd_2051_le_set_connectionless_cte_transmit_parameters(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    uint16_t handle;
    hcicmd_2051_LE_set_connectionless_cte_transmit_parameters_TypeDef *pCmd2051 = (hcicmd_2051_LE_set_connectionless_cte_transmit_parameters_TypeDef *)pCmd;
    
    ht_write_byte ( (uint8_t *)&(leconfig_ConnectionlessCTETxAdvParam.adv_handle ),       ht_read_byte ((uint8_t *)&(pCmd2051->adv_handle          )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ConnectionlessCTETxAdvParam.cte_length ),       ht_read_byte ((uint8_t *)&(pCmd2051->cte_length          )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ConnectionlessCTETxAdvParam.cte_type ),         ht_read_byte ((uint8_t *)&(pCmd2051->cte_type          )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ConnectionlessCTETxAdvParam.length_of_switching_pattern_length ),        ht_read_byte ((uint8_t *)&(pCmd2051->length_of_switching_pattern_length          )) );
    ht_write_byte ( (uint8_t *)&(leconfig_ConnectionlessCTETxAdvParam.antenna_ids ),       ht_read_byte ((uint8_t *)&(pCmd2051->antenna_ids          )) );            

    handle = pCmd2051->adv_handle  ;
    LEACL_TypeDef *pacl;    
    pacl = leacl_with_connHandle( handle ); 
    pacl->cte_length =  pCmd2051->cte_length ;          
    pacl->cte_type =  pCmd2051->cte_type; 
    pacl->cte_count = pCmd2051->cte_count;
    
    return send_04_hciEvent_0e_command_complete(0x2051, ERR_00_SUCCESS);
}

bool hcmd_2052_le_set_connectionless_cte_transmit_enable(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    hcicmd_2052_LE_set_connectionless_cte_transmit_enable_TypeDef *pCmd2052 = (hcicmd_2052_LE_set_connectionless_cte_transmit_enable_TypeDef *)pCmd;

    return send_04_hciEvent_0e_command_complete(0x2052, ERR_00_SUCCESS);
}

bool hcmd_2058_le_read_antenna_information(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
   hcievt_0e_cmd_complete_leReadAntennaInformation_TypeDef evt0e2058 =
    {
        .event_code_0e         = 0x0E,  //HCIEVT_0E_CMD_COMPLETE
        .param_Total_Length    = sizeof(evt0e2058)-2,
        .num_hci_cmd_pkts      = 0x01,  //allowed cmd packets
        .cmd_opcode_2058       = 0x2058,//(OGF_08_LE<<10)|OCF_02F_LE_R_MAXIMUM_DATA_LEN,//opcode 0x2058
        .status                = 0x00,  //ERR_00_SUCCESS
	.supportedSwitchingSamplingRates = 0x02,		   // bit[0] 1 £gs switching supported for AoD transmission
    								   // bit[1] 1 £gs sampling supported for AoD reception
    								   // bit[2] 1 £gs switching and sampling supported for AoA reception    								   
        .numberofAntennae	      	 = 0x01,		   // Range 0x01 to 0x4B The number of antennae supported by the Controller 
        .maxLengthofSwitchingPattern	 = 0x4B,	           // Range 0x02 to 0x4B Maximum length of antenna switching pattern supported by the Controller
        .maxCTELength			 = 0x14,    // Range 0x02 to 0x14 Maximum length of a transmitted Constant Tone Extension supported in 8 £gs units 	
    };

    return send_04_hciEventPacket((HCI_04_EVENT_PACKET_TypeDef *)&evt0e2058);    

}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
bool receive_01_hciCommandPacket(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
    //pCmd[0] HCI command opcode[7:0]
    //pCmd[1] HCI command opcode[15:8]
    //pCmd[2] hdr_length
    //pCmd[3~257] param[0~254]
{
    if( HcCommandBUFF[0].hcmdstate == 0 ) //buffer available
    {
        uint32_t len;
        unsigned char *src;
        unsigned char *dst;
        len = ((uint32_t)(pCmd->param_total_length))+3;
                                   // +3  octet[0] opcode
                                   //     octet[1] opcode
                                   //     octet[2] param_total_length
        src = (unsigned char *) pCmd;
        dst = (unsigned char *)(HcCommandBUFF[0].octet+0);
        while ( len ) {
            *dst = *src;
            dst ++; src ++;
            len --;
        }
        //
        HcCommandBUFF[0].hcmdstate = 0x01;
        return(1);
    }
    else
    {
        return(0);
    }

}
static rxhcicmdHandle_func_pointer_TypeDef rxhcicmdGetHandler(HCI_01_COMMAND_PACKET_TypeDef *pCmd)
{
    rxhcicmdHandle_func_pointer_TypeDef pFun= (rxhcicmdHandle_func_pointer_TypeDef)0;
    uint8_t  opcode_ogf = pCmd->opcode >> 10;
    uint16_t opcode_ocf = pCmd->opcode & 0x03FF;
    ////////////////////////////////////////////////
    if     ( opcode_ogf == 0x01 ) //OGF_01_LINK_CTRL Link Control
    {
    switch( opcode_ocf )
    {
    case OCF_DISCONNECT:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0406_disconnect ;
        break;
    case OCF_R_REMOTE_VER_INFO:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_041d_read_remote_version_information ;
        break;
    }//switch()
    }//if()

    //----------------------------------------------
    else if( opcode_ogf == 0x02 ) //OGF_02_LINK_POLICY  Link Policy
    {
    switch( opcode_ocf )
    {
    }//switch()
    }//if()

    //---------------------------------------------------------------
    else if( opcode_ogf == 0x03 ) //OGF_03_HC_N_BASEBAND   HC and Baseband
    {
    switch( opcode_ocf )
    {
    case OCF_SET_EVENT_MASK:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0c01_set_event_mask ;
        break;
    case OCF_RESET:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0c03_reset ;
        break;
    case OCF_SET_EVENT_MASK_PAGE_2:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0c63_set_event_mask_page_2 ;
        break;
    case OCF_R_AUTHENTICATED_PAYLOAD_TIMEOUT:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0c7b_read_authenticated_payload_timeout ;
        break;
    case OCF_W_AUTHENTICATED_PAYLOAD_TIMEOUT:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_0c7c_write_authenticated_payload_timeout ;
        break;
    }//switch()
    }//if()

    //---------------------------------------------------------------
    else if( opcode_ogf == 0x04 ) //OGF_04_INFO_PARAMS  Info Parameters
    {
    switch( opcode_ocf )
    {
    case OCF_R_LOC_VERSION_INFO:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_1001_read_local_version_info ;
        break;
    case OCF_R_LOC_SUPPORTED_COMMANDS:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_1002_read_local_supported_commands ;
        break;
    case OCF_R_LOC_SUPPORTED_FEATURES:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_1003_read_local_supported_features ;
        break;
    case OCF_R_BD_ADDR:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_1009_read_bd_addr ;
        break;
    }//switch()
    }//if()

    //---------------------------------------------------------------
    else if( opcode_ogf == 0x05 ) //OGF_05_STATUS_PARAMS   Status Parameters
    {
    switch( opcode_ocf )
    {
    }//switch()
    }//if()
    //---------------------------------------------------------------
    else if( opcode_ogf == 0x08 ) //OGF_08_LE   Low Energy
    {
    switch( opcode_ocf )
    {
    case 0x001: //OCF_001_LE_SET_EVENT_MASK:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2001_le_set_event_mask ;
        break;
    case 0x002: //OCF_002_LE_R_BUFFER_SIZE_V1:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2002_le_read_buffer_size_v1 ;
        break;
    case 0x003: //OCF_003_LE_R_LOC_SUPPORTED_FEATURES:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2003_le_read_local_supported_features ;
        break;
    case 0x005: //OCF_005_LE_SET_RAND_ADDR:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2005_le_set_random_address ;
        break;
    case 0x006: //OCF_006_LE_SET_ADV_PARAM:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2006_le_set_adv_parameters ;
        break;
    case 0x007: //OCF_007_LE_R_ADV_CHANNEL_TX_PWR:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2007_le_read_adv_channel_tx_power ;
        break;
    case 0x008: //OCF_008_LE_SET_ADV_DATA:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2008_le_set_adv_data ;
        break;
    case 0x009: //OCF_009_LE_SET_SCAN_RESP_DATA:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2009_le_set_scanresp_data ;
        break;
    case 0x00A: //OCF_00A_LE_SET_ADV_ENABLE:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200a_le_set_adv_enable ;
        break;
    case 0x00B: //OCF_00B_LE_SET_SCAN_PARAM:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200b_le_set_scan_parameters ;
        break;
    case 0x00C: //OCF_00C_LE_SET_SCAN_ENABLE:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200c_le_set_scan_enable ;
        break;
    case 0x00D: //OCF_00D_LE_CREATE_CONNECTION:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200d_le_create_connection ;
        break;
    case 0x00E: //OCF_00E_LE_CANCEL_CREATE_CONNECTION:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200e_le_create_connection_cancel ;
        break;
    case 0x00F: //OCF_00F_LE_R_WHITE_LIST_SIZE:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_200f_le_read_white_list_size ;
        break;
    case 0x010: //OCF_010_LE_CLEAR_WHITE_LIST:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2010_le_clear_white_list ;
        break;
    case 0x011: //OCF_011_LE_ADD_DEVICE_TO_WHITE_LIST:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2011_le_add_device_to_white_list ;
        break;
    case 0x012: //OCF_012_LE_REMOVE_DEVICE_FM_WHITE_LIST:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2012_le_remove_device_from_white_list ;
        break;
    case 0x013: //OCF_013_LE_UPDATE_CONNECTION:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2013_le_connection_update ;
        break;
    case 0x014: //OCF_014_LE_SET_HOST_CHANNEL_CLASSIFICATION:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2014_le_set_host_channel_classification ;
        break;
    case 0x015: //OCF_015_LE_R_CHANNEL_MAP:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2015_le_read_channel_map ;
        break;
    case 0x016: //OCF_016_LE_R_REMOTE_FEATURES:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2016_le_read_remote_features ;
        break;
    case 0x017: //OCF_017_LE_ENCRYPT:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2017_le_encrypt ;
        break;
    case 0x019: //OCF_019_LE_ENABLE_ENCRYPTION:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2019_le_enable_encryption ;
        break;
    case 0x01A: //OCF_01A_LE_LONG_TERM_KEY_REQ_REPLY:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201a_le_LTK_request_reply ;
        break;
    case 0x01B: //OCF_01B_LE_LONG_TERM_KEY_REQ_NEG_REPLY:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201b_le_LTK_request_negative_reply ;
        break;
    case 0x01C: //OCF_01C_LE_R_SUPPORTED_STATES:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201c_le_read_supported_states ;
        break;
    case 0x01D: //OCF_01D_LE_RECEIVER_TEST_V1:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201d_le_receiver_test_v1 ;
        break;        
    case 0x01E: //OCF_01E_LE_TRANSMITTER_TEST_V1:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201e_le_transmitter_test_v1 ;
        break;
    case 0x01F: //OCF_01F_LE_TEST_END:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_201f_le_test_end ;
        break;
    case 0x020: //OCF_020_LE_REMOTE_CONN_PARAM_REQ_REPLY:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2020_le_remote_conn_param_req_reply ;
        break;
    case 0x021: //OCF_021_LE_REMOTE_CONN_PARAM_REQ_NEG_REPLY:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2021_le_remote_conn_param_req_negative_reply ;
        break;
    case 0x022: //OCF_022_LE_SET_DATA_LEN:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2022_le_set_data_length ;
        break;
    case 0x023: //OCF_023_LE_R_SUGGEST_DEFAULT_DATA_LEN:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2023_le_read_suggested_default_data_length ;
        break;
    case 0x024: //OCF_024_LE_W_SUGGEST_DEFAULT_DATA_LEN:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2024_le_write_suggested_default_data_length ;
        break;
    case 0x02F: //OCF_02F_LE_R_MAXIMUM_DATA_LEN:
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_202f_le_read_maximum_data_length ;
        break;
    case 0x033: //OCF_033_LE_RECEIVER_TEST_V2
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2033_le_receiver_test_v2 ;
    	break; 
    case 0x034: //OCF_034_LE_TRANSMITTER_TEST_V2
        pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2034_le_transmitter_test_v2 ;
        break; 
    case 0x036: //OCF_036_LE Set Extended Advertising Parameters
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2036_le_set_extended_advertising_parameters ;        
        break; 
    case 0x03e: //OCF_03E_LE Set Periodic Advertising Parameters command
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_203e_le_set_periodic_advertising_parameters ;        
        break;  
    case 0x040: //OCF_040_LE Set Periodic Advertising Enable command
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2040_le_set_periodic_advertising_enable ;        
        break;                             
    case 0x04f: //OCF_04F_LE_RECEIVER_TEST_V3
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_204f_le_receiver_test_v3 ;        
        break;   
    case 0x050: //OCF_050_LE_TRANSMITTER_TEST_V3
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2050_le_transmitter_test_v3 ;         
    	break;
    case 0x051: //OCF_051_LE Set Connectionless CTE Transmit Parameters command
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2051_le_set_connectionless_cte_transmit_parameters ;         
    	break;	
    case 0x052: //OCF_051_LE Set Connectionless CTE Transmit Enable command
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef) hcmd_2052_le_set_connectionless_cte_transmit_enable ;         
    	break;	    	
    case 0x058: //OCF_058_LE Read Antenna Information command
    	pFun = (rxhcicmdHandle_func_pointer_TypeDef)hcmd_2058_le_read_antenna_information;
    	break;
    		    
    }//switch()
    }//if()
    //---------------------------------------------------------------
    else if( opcode_ogf == 0x3E ) //OGF_3E_BLUETOOTH_LOGO_TEST
    {
    switch( opcode_ocf )
    {
    }//switch()
    }//if()
    //---------------------------------------------------------------
    else if( opcode_ogf == 0x3F ) //OGF_3F_VENDOR_SPECIFIC   Vendor Specific
    {
    switch( opcode_ocf )
    {
    case 0x000:
        break;
    }//switch()
    }//if()
    ////////////////////////////////////////////////
    return pFun ;
}
void process_HcCommandBUFF(void)
{
    bool done;
    rxhcicmdHandle_func_pointer_TypeDef pFun;
    HCI_01_COMMAND_PACKET_TypeDef *pCmd;
    //pCmd[0] HCI command opcode[7:0]
    //pCmd[1] HCI command opcode[15:8]
    //pCmd[2] hdr_length
    //pCmd[3~257] param[0~254]
    if( HcCommandBUFF[0].hcmdstate == 0 )
    {
        return; //exit empty q
    }
        pCmd = (HCI_01_COMMAND_PACKET_TypeDef *)(HcCommandBUFF[0].octet+0);
        done = 1;
               pFun = rxhcicmdGetHandler( pCmd );
    if(        pFun != 0 ) {
        done = pFun( pCmd );
    }
    if (done)
    {
        HcCommandBUFF[0].hcmdstate = 0;
    }
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
static volatile uint8_t cobsstate;
#define W4_PKT_TYPE          0
#define W4_HC_CMD_OPCODE_L   1
#define W4_HC_CMD_OPCODE_H   2
#define W4_HC_CMD_LENGTH     3
#define W4_HC_CMD_PARAMS     4
#define W4_HC_ACL_HDR_HDL_L  5
#define W4_HC_ACL_HDR_HDL_H  6
#define W4_HC_ACL_HDR_LEN_L  7
#define W4_HC_ACL_HDR_LEN_H  8
#define W4_HC_ACL_DATA       9
#define W4_HC_ACL_DATA_BUFFERNOTFOUND 10


void ur0isr_receive_cobs_block(uint8_t ch)
{
    static          uint8_t * pOutCobsDest ;
    static volatile uint8_t   cmd_param_len_remaining ;
    static volatile uint16_t  acl_data_len_remaining ;
    static union
    {
        struct {
            uint32_t    bc_pb_handle:16;        // 2 octets[11: 0]Handle
                                                //         [13:12]Packet_Boundary_Flag
                                                //         [15:14]Broadcast_Flag
            uint32_t    total_length:16;        // 2 octets       Data_Total_Length
        } field;
        uint8_t octet[4];
    } cobs04_header;
    static hc_leacl_data_buffer_toair_TypeDef *pBuffer ;
    
    switch(cobsstate)
    {
    case W4_PKT_TYPE:
        if(ch == 0x02) { //HCI_PTYPE_02_ACL
            cobsstate = W4_HC_ACL_HDR_HDL_L ;
            break;
        }
        if(ch == 0x01) { //HCI_PTYPE_01_HC_COMMAND
            cobsstate = W4_HC_CMD_OPCODE_L ;
            break;
        }
        //parse error
        //no state transition
        break;

    ///////////////////////////////////////////////
    case W4_HC_ACL_HDR_HDL_L:
            cobs04_header.octet[0]= ch ;
            cobsstate = W4_HC_ACL_HDR_HDL_H ;
            break;
    case W4_HC_ACL_HDR_HDL_H:
            cobs04_header.octet[1]= ch ;
            cobsstate = W4_HC_ACL_HDR_LEN_L ;
            break;
    case W4_HC_ACL_HDR_LEN_L:
            cobs04_header.octet[2]= ch ;
            cobsstate = W4_HC_ACL_HDR_LEN_H ;
            break;
    case W4_HC_ACL_HDR_LEN_H:
            cobs04_header.octet[3]= ch ;
            acl_data_len_remaining = cobs04_header.field.total_length ;
        if( acl_data_len_remaining == 0 ) {
            cobsstate = W4_PKT_TYPE ;
            break;
        }
        if((pBuffer = ur0isr_hcAclDataBuffer_toair_allocate(cobs04_header.field.bc_pb_handle,
                                                            cobs04_header.field.total_length
                                                           )
           ) == 0 )   //NULL:fail
        {
            //buffer not found
            //but still have to receive this whole hci acl packet
            cobsstate = W4_HC_ACL_DATA_BUFFERNOTFOUND ;
            break;
        }
        else {
            pOutCobsDest = pBuffer->octet+0;
            cobsstate = W4_HC_ACL_DATA ;
            break;
        }
    case W4_HC_ACL_DATA:
            acl_data_len_remaining --;
            *pOutCobsDest = ch; pOutCobsDest ++;
        if( acl_data_len_remaining == 0 )
        {
            //received this whole hci acl packet
            //push this buffer to pacl->lldataTxQ
            LEACL_TypeDef *pacl;
                pacl = leacl_with_connHandle( cobs04_header.field.bc_pb_handle&0x0FFF );
            if (pacl == 0)
            {
                hcAclDataBuffer_toair_free( pBuffer );
                cobsstate = W4_PKT_TYPE ;
                break;
            }
          //
            pdma_ram_bleTx_write_payload (pBuffer->ram_start_address + 0x0010, // 0x0010: first 16 bytes for Session Key (SK)
                                          cobs04_header.field.total_length, 
                                          pBuffer->octet+0
                                         );
          //
            if( RINGBUF_isFull(pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ ) ) // full Q
            {
                // full Q
                hcAclDataBuffer_toair_free( pBuffer );
                cobsstate = W4_PKT_TYPE ;
                break;
            }
            else
            {
                //push this buffer to pacl->lldataTxQ
                RINGBUF_push( pacl->lldataTxQ, pBuffer, TOTAL_NUM_ELEMENTS_lldataTxQ );
                cobsstate = W4_PKT_TYPE ;
            }
            break;
        }
        break;
    case W4_HC_ACL_DATA_BUFFERNOTFOUND://buffer not found, but still have to receive this whole hci acl packet
            acl_data_len_remaining --;
        if( acl_data_len_remaining == 0 ) {
            cobsstate = W4_PKT_TYPE ;
            break;
        }
        break;

    /////////////////////////////////////////////////
    case W4_HC_CMD_OPCODE_L:
        HcCommandBUFF[0].octet[0]= ch ; //cmd_hdr ocf[7:0]
        cobsstate = W4_HC_CMD_OPCODE_H ;
        break;
    case W4_HC_CMD_OPCODE_H:
        HcCommandBUFF[0].octet[1]= ch ; //cmd_hdr ogf[5:0] ocf[9:8]
        cobsstate = W4_HC_CMD_LENGTH ;
        break;
    case W4_HC_CMD_LENGTH:
        HcCommandBUFF[0].octet[2]= cmd_param_len_remaining = ch ;   //octet[2] is cmd_hdr.length
        if( cmd_param_len_remaining == 0 ) {
            HcCommandBUFF[0].hcmdstate = 1;
            cobsstate = W4_PKT_TYPE ;
            break;
        }
        pOutCobsDest = HcCommandBUFF[0].octet+3 ; //+3:hdr
        cobsstate = W4_HC_CMD_PARAMS ;
        break;
    case W4_HC_CMD_PARAMS:
        *pOutCobsDest = ch; pOutCobsDest++;
            cmd_param_len_remaining --;
        if( cmd_param_len_remaining == 0 ) {
            HcCommandBUFF[0].hcmdstate = 1;
            cobsstate = W4_PKT_TYPE ;
            break;
        }
        break;

    /////////////////////////////////////////////////
    default:
        cobsstate = W4_PKT_TYPE ;
        break;
    }//switch
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
void initial_hc(void)
{
    cobsstate = W4_PKT_TYPE ;
}
void process_hc(void)
{
    process_HcCommandBUFF();
}


