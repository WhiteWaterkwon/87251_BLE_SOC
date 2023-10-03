/*********************************************************************************************************//**
 * @file    leconfig.h
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
#ifndef HT_LE_CONFIG_H
#define HT_LE_CONFIG_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdint.h>

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"


/* Exported constants --------------------------------------------------------------------------------------*/

// <<< Use Configuration Wizard in Context Menu >>>

// <o> my BD_ADDR[0]
//    <0x00-0xFF:0x1>
#define MY_BD_ADDR_0             0x71


// <<< end of configuration section >>>


/* Exported types ------------------------------------------------------------------------------------------*/

typedef struct
{
    uint8_t                     event_mask[8];
}      le_0c01_event_mask_TypeDef ;
extern le_0c01_event_mask_TypeDef leconfig_eventMask;

typedef struct
{
    uint8_t                     event_mask[8];
}      le_0c63_event_mask_page2_TypeDef ;
extern le_0c63_event_mask_page2_TypeDef leconfig_eventMaskPage2;

#define EVTMASK_is_allowed_LE_meta()                                  ( (leconfig_eventMask.event_mask[7] & 0x20) )  //bit 61  LE Meta Event

typedef struct
{
    uint8_t                     le_event_mask[8];
}      le_2001_le_event_mask_TypeDef ;
extern le_2001_le_event_mask_TypeDef leconfig_leEventMask;

#define EVTMASK_is_allowed_LE_connection_complete()                   ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x01) )  //bit 0  LE Connection Complete event
#define EVTMASK_is_allowed_LE_advertising_report()                    ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x02) )  //bit 1  LE Advertising Report event
#define EVTMASK_is_allowed_LE_conn_update_complete()                  ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x04) )  //bit 2  LE Connection Update Complete event
#define EVTMASK_is_allowed_LE_read_remote_features_complete()         ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x08) )  //bit 3  LE Read Remote Features Complete event
#define EVTMASK_is_allowed_LE_long_term_key_request()                 ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x10) )  //bit 4  LE Long Term Key Request event
#define EVTMASK_is_allowed_LE_remote_conn_param_request()             ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x20) )  //bit 5  LE Remote Connection Parameter Request event
#define EVTMASK_is_allowed_LE_data_length_change()                    ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x40) )  //bit 6  LE Data Length Change event
#define EVTMASK_is_allowed_LE_read_local_P_256_public_key_complete()  ( EVTMASK_is_allowed_LE_meta() && (leconfig_leEventMask.le_event_mask[0] & 0x80) )  //bit 7  LE Read Local P-256 Public Key Complete event

typedef struct __attribute__((packed))
{
    uint16_t                    authenticatedPayloadTO; // 2 octets, Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms
}      le_0c7c_authenticated_payload_timeout_TypeDef ;
extern le_0c7c_authenticated_payload_timeout_TypeDef leconfig_authenticatedPayloadTimeout;

typedef struct
{
    uint8_t                     versNr;               //Link Layer Version
                                                        //LL Version    0x06: Bluetooth spec 4.0
                                                        //              0x07: Bluetooth spec 4.1
                                                        //              0x08: Bluetooth spec 4.2
                                                        //              0x09: Bluetooth spec 5.0
                                                        //              0x0A: Bluetooth spec 5.1
                                                        //              0x0B: Bluetooth spec 5.2
                                                        //              0x0C: Bluetooth spec 5.3
    uint16_t                    compId;              //Company identifiers
                                                        //              0x0046	MediaTek, Inc
    uint16_t                    subVersNr;           //Sub Version
}      le_1001_version_info_TypeDef ;
extern le_1001_version_info_TypeDef leconfig_versionInfo;

typedef struct
{
    uint8_t                     lmp_features[8];        // 8 octets
}      le_1003_lmpFeatures_TypeDef ;
extern le_1003_lmpFeatures_TypeDef leconfig_lmpFeatures;

typedef struct
{
    uint8_t                     le_public_AdvA[6];
}      le_1009_bd_addr_TypeDef ;
extern le_1009_bd_addr_TypeDef leconfig_bdaddr;

typedef struct
{
    uint8_t                     le_features[8];         // 8 octets
}      le_2003_features_TypeDef ;
extern le_2003_features_TypeDef leconfig_leFeatures;

typedef struct
{
    uint8_t                     random_address[6];
}      le_2005_random_address_TypeDef ;
extern le_2005_random_address_TypeDef leconfig_randomAddress;

typedef struct __attribute__((packed))
{
    uint16_t                    adv_interval_min;       //Range: 0x0020 to 0x4000, unit 625us, Time Range: 20 ms to 10.24 sec,    Default: N=0x0800 (1.28 second)
    uint16_t                    adv_interval_max;       //Range: 0x0020 to 0x4000, unit 625us, Time Range: 20 ms to 10.24 sec,    Default: N=0x0800 (1.28 second)
    uint8_t                     adv_type;
                                                        //0x00: Connectable and scannable undirected adv (ADV_IND) (default) 
                                                        //0x01: Connectable high duty cycle directed adv (ADV_DIRECT_IND, high duty cycle)
                                                        //0x02: Scannable undirected adv (ADV_SCAN_IND)
                                                        //0x03: Non connectable undirected adv (ADV_NONCONN_IND)
                                                        //0x04: Connectable low duty cycle directed adv (ADV_DIRECT_IND, low duty cycle)
    uint8_t                     own_address_type;
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    uint8_t                     peer_address_type;
	                                                    //0x00: Public Device Address(default) or Public Identity Address
	                                                    //0x01: Random Device Address or Random (static) Identity Address
    uint8_t                     peer_address[6];
    uint8_t                     adv_channel_map;
                                                        //xxxxxxx1b: Channel 37 shall be used
                                                        //xxxxxx1xb: Channel 38 shall be used
                                                        //xxxxx1xxb: Channel 39 shall be used
    uint8_t                     adv_filter_policy;
                                                        //0x00: Process scan and connection requests from all devices (i.e., the White List is not in use) (default)
                                                        //0x01: Process connection requests from all devices and only scan requests from devices that are in the White List
                                                        //0x02: Process scan requests from all devices and only connection requests from devices that are in the White List
                                                        //0x03: Process scan and connection requests only from devices in the White List
}      le_2006_adv_param_TypeDef ;
extern le_2006_adv_param_TypeDef leconfig_AdvParam;



typedef struct __attribute__((packed))
{
    uint8_t  length;
    uint8_t  advData[31]; //37-6
}      le_2008_adv_data_TypeDef ;
extern le_2008_adv_data_TypeDef leconfig_AdvData;


typedef struct __attribute__((packed))
{
    uint8_t  length;
    uint8_t  scanRspData[31]; //37-6
}      le_2009_scanresp_data_TypeDef ;
extern le_2009_scanresp_data_TypeDef leconfig_ScanRspData;


typedef struct __attribute__((packed))
{
    uint8_t                     le_scan_type;
                                                        //0x00: Passive Scanning. No scanning PDUs shall be sent (default)
                                                        //0x01: Active scanning. Scanning PDUs may be sent
    uint16_t                    le_scan_interval;       //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    uint16_t                    le_scan_window;         //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    uint8_t                     own_address_type;
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    uint8_t                     scanning_filter_policy;
                                                        //0x00: Accept all adv and scan response PDUs except directed adv PDUs not addressed to this device (default)
                                                        //0x01: Accept only adv and scan response PDUs from devices where the advertiser address is in the White List. Directed adv PDUs which are not addressed to this device shall be ignored
                                                        //0x02: Accept all adv and scan response PDUs except directed adv PDUs where the initiator's identity address does not address this device
                                                        //0x03: Accept all adv and scan response PDUs except:...
}      le_200b_scan_param_TypeDef ;
extern le_200b_scan_param_TypeDef leconfig_ScanParam;


typedef struct __attribute__((packed))
{
    uint16_t                    le_scan_interval;       //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    uint16_t                    le_scan_window;         //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    uint8_t                     initiator_filter_policy;
	                                                    //0x00: White List is not used to determine which advertiser to connect to Peer_Address_Type and Peer_Address shall be used
	                                                    //0x01: White List is     used to determine which advertiser to connect to Peer_Address_Type and Peer_Address shall be ignored
    uint8_t                     peer_address_type;
	                                                    //0x00: Public Device Address
	                                                    //0x01: Random Device Address
	                                                    //0x02: Public Identity Address (Corresponds to peer Resolvable Private Address)
	                                                    //0x03: Random (static) Identity Address (Corresponds to peer Resolvable Private Address)
    uint8_t                     peer_address[6];
    uint8_t                     own_address_type;
    uint16_t                    conn_interval_min;
    uint16_t                    conn_interval_max;
    uint16_t                    slave_latency;
    uint16_t                    supervision_timeout;
    uint16_t                    min_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
    uint16_t                    max_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
}      le_200d_create_connection_TypeDef;
extern le_200d_create_connection_TypeDef leconfig_CreateConnection;


#define LE_WHITE_LIST_SIZE                           2
typedef struct
{
    uint8_t                     address_type;
    uint8_t                     address[6];
}      le_2011_white_list_TypeDef ;
extern le_2011_white_list_TypeDef leconfig_whiteList[LE_WHITE_LIST_SIZE];


typedef struct
{
    uint8_t                     channel_map[5];         // 5 octets, 37 1-bit fields, Channel n is bad     = 0
                                                        //                            Channel n is unknown = 1
}      le_2014_host_channel_class_TypeDef ;
extern le_2014_host_channel_class_TypeDef leconfig_hostChannelClass;


typedef struct
{
    uint8_t                     tx_channel;             // 1 octet
    uint8_t                     test_data_length;       // 1 octet
    uint8_t                     packet_payload;         // 1 octet
}      le_201e_tx_test_TypeDef ;
extern le_201e_tx_test_TypeDef leconfig_txTest;

typedef struct __attribute__((packed))
{
    uint8_t  length;
    uint8_t  advData[37]; //37
}      le_201e_adv_data_TypeDef ;
extern le_201e_adv_data_TypeDef leconfig_txTestData;

typedef struct
{
    uint16_t                    connInitialMaxTxOctets; // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    connInitialMaxTxTime;   // Range 0x0148-0x4290    (328)~(17040)
}      le_2023_suggested_default_data_length_TypeDef ;
extern le_2023_suggested_default_data_length_TypeDef leconfig_defaultDataLength;

typedef struct
{
    uint16_t                    supportedMaxTxOctets;   // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    supportedMaxTxTime  ;   // Range 0x0148-0x4290    (328)~(17040)  1*8+(4*8+(2+31)*8+3*8)=328 us, 80+(4*8+2+3+(2+255)*8+3*8+3)*8=17040 us
    uint16_t                    supportedMaxRxOctets;   // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    supportedMaxRxTime  ;   // Range 0x0148-0x4290    (328)~(17040)  1*8+(4*8+(2+31)*8+3*8)=328 us, 80+(4*8+2+3+(2+255)*8+3*8+3)*8=17040 us
}      le_202f_maximum_data_length_TypeDef ;
extern le_202f_maximum_data_length_TypeDef leconfig_maximumDataLength;

typedef struct __attribute__((packed))
{
    uint8_t                     adv_handle;
    uint16_t 			adv_event_properties;	//bit[0] Connectable advertising
							//bit[1] Scannable advertising
							//bit[2] Directed advertising
							//bit[3] High Duty Cycle Directed Connectable advertising ( 3.75 ms Advertising Interval)
							//bit[4] Use legacy advertising PDUs
							//bit[5] Omit advertiser's address from all PDUs ("anonymous advertising")
							//bit[6] Include TxPower in the extended header of at least one advertising PDU 
    uint32_t                    primary_adv_interval_min;
    uint32_t                    primary_adv_interval_max;
    uint8_t			primary_adv_channel_map;	//bit[0] Channel 37 shall be used ;bit[1] Channel 38 shall be used; bit[2] Channel 39 shall be used    
    uint8_t                     own_address_type;
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    uint8_t                     peer_address_type;
	                                                    //0x00: Public Device Address(default) or Public Identity Address
	                                                    //0x01: Random Device Address or Random (static) Identity Address
    uint8_t                     peer_address[6];
    uint8_t                     adv_filter_policy;
                                                        //0x00: Process scan and connection requests from all devices (i.e., the White List is not in use) (default)
                                                        //0x01: Process connection requests from all devices and only scan requests from devices that are in the White List
                                                        //0x02: Process scan requests from all devices and only connection requests from devices that are in the White List
                                                        //0x03: Process scan and connection requests only from devices in the White List
    uint8_t			adv_tx_power;		//Range: -127 to +20 Units: dBm
    uint8_t			primary_adv_phy;	//0x01 Primary advertisement PHY is LE 1M 
    							//0x03 Primary advertisement PHY is LE Coded
    uint8_t			secondary_adv_max_skip;	//0x00 AUX_ADV_IND shall be sent prior to the next advertising event
    							//0x01 to 0xFF Maximum advertising events the Controller can skip before sending the AUX_ADV_IND packets on the secondary advertising physical channel 							
    uint8_t			secondary_adv_max_phy;	//0x01 Secondary advertisement PHY is LE 1M
    							//0x02 Secondary advertisement PHY is LE 2M
    							//0x03 Secondary advertisement PHY is LE Coded
    uint8_t			adv_sid;			//0x00 to 0x0F Value of the Advertising SID subfield in the ADI field of the PDU
    uint8_t			scan_req_notification_enable;	//0x00 Scan request notifications disabled ; 0x01 enable
}      le_2036_ext_adv_param_TypeDef ;
extern le_2036_ext_adv_param_TypeDef leconfig_ExtAdvParam;

typedef struct __attribute__((packed))
{
    uint8_t                     adv_handle;
    uint16_t                    periodic_adv_interval_min;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)	
    uint16_t                    periodic_adv_interval_max;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)
    uint16_t			periodic_adv_properties;	//bit[6] Include TxPower in the advertising PDU       						
} le_203e_periodic_adv_param_TypeDef;
extern le_203e_periodic_adv_param_TypeDef leconfig_PeriodicAdvParam;

typedef struct __attribute__((packed))
{
    uint8_t                     adv_handle;  // 
    uint8_t                     cte_length;    // 
    uint8_t                     cte_type;    // 
    uint8_t			cte_count;
    uint8_t			length_of_switching_pattern_length;
    uint8_t			antenna_ids;
} le_2051_connectionless_cte_transmit_parameters_TypeDef;
extern le_2051_connectionless_cte_transmit_parameters_TypeDef leconfig_ConnectionlessCTETxAdvParam;

/* Exported macro ------------------------------------------------------------------------------------------*/
#define LOCAL_FEATURE_get_featureSet(i)                                 ( leconfig_leFeatures.le_features[i] )

#define LOCAL_FEATURE_is_supported_LE_encryption()                      ( leconfig_leFeatures.le_features[0] & 0x01 )  //bit 0  LE Encryption
#define LOCAL_FEATURE_is_supported_conn_params_request_procedure()      ( leconfig_leFeatures.le_features[0] & 0x02 )  //bit 1  Connection Parameters Request procedure  LL_0F_CONNECTION_PARAM_REQ,RSP
#define LOCAL_FEATURE_is_supported_extended_reject_ind()                ( leconfig_leFeatures.le_features[0] & 0x04 )  //bit 2  Extended Reject Indication               LL_11_REJECT_EXT_IND
#define LOCAL_FEATURE_is_supported_slave_initiated_features_exchange()  ( leconfig_leFeatures.le_features[0] & 0x08 )  //bit 3  Peripheral-initiated Features Exchange   LL_0E_SLAVE_FEATURE_REQ
#define LOCAL_FEATURE_is_supported_LE_ping()                            ( leconfig_leFeatures.le_features[0] & 0x10 )  //bit 4  LE Ping                                  LL_12_PING_REQ,RSP
#define LOCAL_FEATURE_is_supported_LE_data_packet_length_extension()    ( leconfig_leFeatures.le_features[0] & 0x20 )  //bit 5  LE Data Packet Length Extension          LL_14_LENGTH_REQ,RSP
#define LOCAL_FEATURE_is_supported_LE_privacy()                         ( leconfig_leFeatures.le_features[0] & 0x40 )  //bit 6  LE Privacy


/* Exported functions --------------------------------------------------------------------------------------*/
void initial_leconfig(void);


#endif /* HT_LE_CONFIG_H */
