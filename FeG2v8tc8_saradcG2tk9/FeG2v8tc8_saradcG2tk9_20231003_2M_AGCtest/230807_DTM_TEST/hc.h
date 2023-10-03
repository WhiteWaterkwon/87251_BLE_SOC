/*********************************************************************************************************//**
 * @file    hc.h
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
 * <h2><center>Copyright (C) 2016 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __BC5602B_HC_H
#define __BC5602B_HC_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include "leacl.h"          //LEACL_TypeDef


/* Exported types ------------------------------------------------------------------------------------------*/


//////////////////////////////////////////////////////////
typedef struct __attribute__((packed))
{
    uint32_t                    bc_pb_handle:16;        // 2 octets[11: 0]Handle
                                                        //         [13:12]Packet_Boundary_Flag
                                                        //         [15:14]Broadcast_Flag
    uint32_t                    data_total_length:16;   // 2 octets       Data_Total_Length
    uint8_t                     data[255-0];            // data[]

} HCI_02_ACL_DATA_PACKET_TypeDef;


//////////////////////////////////////////////////////////
typedef struct __attribute__((packed))
{
    uint16_t                    opcode;                 // opcode        2 octets
    uint8_t                     param_total_length;     // length        1 octet
    uint8_t                     params[255];            // params[]  0~255 octets

} HCI_01_COMMAND_PACKET_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0406;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     reason;                 // 1 octet

} hcicmd_0406_disconnect_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_041d;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcicmd_041d_read_remote_version_info_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0c01;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 8
    uint8_t                     event_mask[8];          // 8 octets

} hcicmd_0c01_set_event_mask_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0c03;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 0

} hcicmd_0c03_reset_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0c01;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 8
    uint8_t                     event_mask[8];          // 8 octets

} hcicmd_0c63_set_event_mask_page_2_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0c7b;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcicmd_0c7b_read_authenticated_payload_timeout_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_0c7c;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    authenticatedPayloadTO; // 2 octets, Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms

} hcicmd_0c7c_write_authenticated_payload_timeout_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2001;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 8
    uint8_t                     le_event_mask[8];       // 8 octets

} hcicmd_2001_le_set_event_mask_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2005;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 6
    uint8_t                     random_address[6];      // 6 octets

} hcicmd_2005_LE_set_random_address_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2006;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 15
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

} hcicmd_2006_LE_set_adv_parameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2008;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     adv_data_length;        // 1 octet   value 0x00~0x1F
    uint8_t                     adv_data[31];           //31 octets

} hcicmd_2008_LE_set_adv_data_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2009;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     scanresp_data_length;   // 1 octet   value 0x00~0x1F
    uint8_t                     scanresp_data[31];      //31 octets

} hcicmd_2009_LE_set_scanresp_data_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_200a;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     adv_enable;             // 1 octet

} hcicmd_200a_LE_set_adv_enable_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_200b;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
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
} hcicmd_200b_LE_set_scan_parameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_200c;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     scan_enable;            // 1 octet
    uint8_t                     Filter_Duplicates;
                                                        //0x00: Duplicate filtering disabled
                                                        //0x01: Duplicate filtering enabled
} hcicmd_200c_LE_set_scan_enable_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_200d;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 25
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
    uint16_t                    conn_interval_min;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    conn_interval_max;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    slave_latency;
    uint16_t                    supervision_timeout;
    uint16_t                    min_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
    uint16_t                    max_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s

} hcicmd_200d_LE_create_connection_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_200e;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_200e_LE_create_connection_cancel_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2011;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     addr_type;              // 1 octet
    uint8_t                     address[6];             // 6 octets

} hcicmd_2011_LE_add_device_to_whitelist_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2012;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     addr_type;              // 1 octet
    uint8_t                     address[6];             // 6 octets

} hcicmd_2012_LE_remove_device_from_whitelist_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2013;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    conn_interval_min;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    conn_interval_max;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    max_slave_latency;      // Range 0x0000-0x01F3
    uint16_t                    supervision_timeout;    // Range 0x000A-0x0C80   Time = N * 10 ms       Time Range: 100 ms to 32 s
    uint16_t                    min_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
    uint16_t                    max_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s

} hcicmd_2013_LE_connection_update_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2014;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     channel_map[5];         // 5 octets

} hcicmd_2014_LE_set_host_channel_classification_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2015;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcicmd_2015_LE_read_channel_map_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2016;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcicmd_2016_LE_read_remote_features_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2017;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     key[16];                // 16 octets
    uint8_t                     plaintext_data[16];     // 16 octets

} hcicmd_2017_LE_encrypt_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2018;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_2018_LE_rand_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2019;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     random_number[8];       // 8 octets
    uint8_t                     encrypted_diversifier[2];// 2 octets
    uint8_t                     long_term_key[16];      // 16 octets

} hcicmd_2019_LE_enable_encryption_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201a;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     long_term_key[16];      // 16 octets

} hcicmd_201a_LE_LTK_request_reply_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201b;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcicmd_201b_LE_LTK_request_negative_reply_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201c;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_201c_LE_read_supported_states_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201d;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     rx_channel;             // 1 octet

} hcicmd_201d_LE_receiver_test_v1_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201e;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     tx_channel;             // 1 octet
    uint8_t                     test_data_length;       // 1 octet
    uint8_t                     packet_payload;         // 1 octet

} hcicmd_201e_LE_transmitter_test_v1_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_201f;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_201f_LE_test_end_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2020;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    conn_interval_min;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    conn_interval_max;      // Range 0x0006-0x0C80   Time = N * 1.25 ms     Time Range: 7.5 ms to 4 s
    uint16_t                    max_slave_latency;      // Range 0x0000-0x01F3
    uint16_t                    supervision_timeout;    // Range 0x000A-0x0C80   Time = N * 10 ms       Time Range: 100 ms to 32 s
    uint16_t                    min_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
    uint16_t                    max_CE_length;          // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s

} hcicmd_2020_LE_remote_conn_param_req_reply_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2021;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     reason;                 // 1 octet

} hcicmd_2021_LE_remote_conn_param_req_negative_reply_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2022;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    tx_octets;              // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    tx_time;                // Range 0x0148-0x4290

} hcicmd_2022_LE_set_data_length_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2023;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_2023_LE_read_suggested_default_data_length_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2024;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint16_t                    connInitialMaxTxOctets; // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    connInitialMaxTxTime;   // Range 0x0148-0x4290

} hcicmd_2024_LE_write_suggested_default_data_length_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_202f;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet

} hcicmd_202f_LE_read_maximum_data_length_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_204f;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     rx_channel;		// 1 octet
    uint8_t                     phy;		        // 1 octet
    uint8_t 			modulation_index;	// 1 octet
    uint8_t			expected_cte_length;	// 1 octet
    uint8_t			expected_cte_type;	// 1 octet
    uint8_t			slot_duration;		// 1 octet
    uint8_t			switching_pattern_length;// 1 octet
    uint8_t			antenna_ids;		// switching_pattern_length octets
} hcicmd_204f_LE_receiver_test_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2033;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     rx_channel;             // 1 octet
    uint8_t                     phy;             	// 1 octet
    uint8_t                     modulation_index;             // 1 octet
    
} hcicmd_2033_LE_receiver_test_v2_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2034;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     tx_channel;  // 
    uint8_t                     test_data_length;    // 
    uint8_t                     packet_payload;    // 
    uint8_t			phy;

} hcicmd_2034_LE_transmitter_test_v2_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2036;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 15
    uint8_t                     adv_handle;
    uint16_t 			adv_event_properties;	//bit[0] Connectable advertising
							//bit[1] Scannable advertising
							//bit[2] Directed advertising
							//bit[3] High Duty Cycle Directed Connectable advertising ( 3.75 ms Advertising Interval)
							//bit[4] Use legacy advertising PDUs
							//bit[5] Omit advertiser's address from all PDUs ("anonymous advertising")
							//bit[6] Include TxPower in the extended header of at least one advertising PDU 
    uint8_t                    primary_adv_interval_min[3];
    uint8_t                    primary_adv_interval_max[3];
   // uint16_t                    primary_adv_interval_min;       	//Range: 0x000020 to 0xFFFFFF, unit 625us, Time Range: 20 ms to 10,485.759375 sec,    Default: N=0x0800 (1.28 second)
   // uint8_t			primary_adv_interval_min_highbyte;	//	
   // uint16_t                    primary_adv_interval_max;       //Range: 0x0020 to 0x4000, unit 625us, Time Range: 20 ms to 10.24 sec,    Default: N=0x0800 (1.28 second)
   // uint8_t			primary_adv_interval_max_highbyte;
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
       							
} hcicmd_2036_LE_set_ext_adv_parameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2039;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 15
    uint8_t                     enable;   
    uint8_t			number_of_sets; 
    uint8_t                     adv_handle;    
    uint16_t			duration;
    uint8_t			max_extended_adv_events; 						
} hcicmd_2039_LE_set_ext_adv_enable_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_203e;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 15
    uint8_t                     adv_handle;
    uint16_t                    periodic_adv_interval_min;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)	
    uint16_t                    periodic_adv_interval_max;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)
    uint16_t			periodic_adv_properties;	//bit[6] Include TxPower in the advertising PDU       						
} hcicmd_203E_LE_set_periodic_adv_parameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2040;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet   value 15
    uint8_t                     enable;    
    uint8_t                     adv_handle;     						
} hcicmd_2040_LE_set_periodic_adv_enable_TypeDef;


typedef struct __attribute__((packed))
{
    uint16_t                    opcode_204f;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     rx_channel;  // 
    uint8_t                     phy;    // 
    uint8_t			modulation_index;
    uint8_t                     expected_cte_length;
    uint8_t                     expected_cte_type;
    uint8_t                     slot_durations;
    uint8_t			switching_pattern_length;
    uint8_t			antenna_ids;

} hcicmd_204F_LE_receiver_test_v3_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2050;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     tx_channel;  // 
    uint8_t                     test_data_length;    // 
    uint8_t                     packet_payload;    // 
    uint8_t			phy;
    uint8_t                     cte_length;
    uint8_t                     cte_type;
    uint8_t			switching_pattern_length;
    uint8_t			antenna_ids;

} hcicmd_2050_LE_transmitter_test_v3_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2051;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     adv_handle;  // 
    uint8_t                     cte_length;    // 0x02 to 0x14 Constant Tone Extension length in 8 £gs units
    uint8_t                     cte_type;      // 0x00:AoA , 0x01:AoD 1us , 0x02:AoD 2us
    uint8_t			cte_count;	//The number of Constant Tone Extensions to transmit in each periodic advertising interval Range: 0x01 to 0x10
    uint8_t			length_of_switching_pattern_length;
    uint8_t			antenna_ids;
} hcicmd_2051_LE_set_connectionless_cte_transmit_parameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint16_t                    opcode_2052;            // 2 octets
    uint8_t                     param_total_length;     // 1 octet
    uint8_t                     adv_handle;  // 
    uint8_t                     cte_enable;  // 	
} hcicmd_2052_LE_set_connectionless_cte_transmit_enable_TypeDef;
//////////////////////////////////////////////////////////
typedef struct __attribute__((packed))
{
    uint8_t                     event_code;             // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     params[255];            // params[]  0~255 octets

} HCI_04_EVENT_PACKET_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_05;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     reason;                 // 1 octet
} hcievt_05_disconnection_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_08;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     encryption_enabled;     // 1 octet
} hcievt_08_encryption_change_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0C;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     version;                // 1 octet
    uint16_t                    manufacturer_name;      // 2 octets
    uint16_t                    subversion;             // 2 octets
} hcievt_0C_read_remote_version_info_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode;             // 2 octets
    uint8_t                     status;                 // 1 octet      Return_Parameters[]   depend on cmd_opcode
} hcievt_0e_cmd_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_0c7b;        // 2 octets opcode 0x0c7b
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    authenticatedPayloadTO; // 2 octets Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms
} hcievt_0e_cmd_complete_leReadAuthenticatedPayloadTO_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_0c7c;        // 2 octets opcode 0x0c7c
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
} hcievt_0e_cmd_complete_leWriteAuthenticatedPayloadTO_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_1001;        // 2 octets opcode 0x1001
    uint8_t                     status;                 // 1 octet
    uint8_t                     hci_version;            // 1 octet
    uint16_t                    hci_subversion;         // 2 octets
    uint8_t                     lmp_version;            // 1 octet
    uint16_t                    company_identifier;     // 2 octets
    uint16_t                    lmp_subversion;         // 2 octets
} hcievt_0e_cmd_complete_readLocalVersion_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_1002;        // 2 octets opcode 0x1002
    uint8_t                     status;                 // 1 octet
    uint8_t                     supported_commands[64]; //64 octets
} hcievt_0e_cmd_complete_readLocalSupportedCommands_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_1003;        // 2 octets opcode 0x1003
    uint8_t                     status;                 // 1 octet
    uint8_t                     lmp_features[8];        // 8 octets
} hcievt_0e_cmd_complete_readLocalSupportedFeatures_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_1009;        // 2 octets opcode 0x1009
    uint8_t                     status;                 // 1 octet
    uint8_t                     bd_addr[6];             // 6 octets
} hcievt_0e_cmd_complete_readBdAddr_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2002;        // 2 octets opcode 0x2002
    uint8_t                     status;                 // 1 octet
    uint16_t                    le_acl_data_packet_length;      // 2 octets
    uint8_t                     total_num_le_acl_data_packets;  // 1 octet
} hcievt_0e_cmd_complete_leReadBufferSizeV1_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2003;        // 2 octets opcode 0x2003
    uint8_t                     status;                 // 1 octet
    uint8_t                     le_features[8];         // 8 octets
} hcievt_0e_cmd_complete_leReadLocalSupportedFeatures_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2007;        // 2 octets opcode 0x2007
    uint8_t                     status;                 // 1 octet
    uint8_t                     tx_power_level;         // 1 octet
} hcievt_0e_cmd_complete_leReadAdvPhyChannelTxPower_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_200f;        // 2 octets opcode 0x200f
    uint8_t                     status;                 // 1 octet
    uint8_t                     white_list_size;        // 1 octet
} hcievt_0e_cmd_complete_leReadWhiteListSize_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2015;        // 2 octets opcode 0x2015
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     channel_map[5];         // 5 octets
} hcievt_0e_cmd_complete_leReadChannelMap_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2017;        // 2 octets opcode 0x2017
    uint8_t                     status;                 // 1 octet
    uint8_t                     encrypted_data[16];     // 16 octets
} hcievt_0e_cmd_complete_leEncrypt_TypeDef; // AES-128 block cipher

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_201c;        // 2 octets opcode 0x201C
    uint8_t                     status;                 // 1 octet
    uint8_t                     le_states[8];           // 8 octets
} hcievt_0e_cmd_complete_leReadLocalSupportedStates_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_201f;        // 2 octets opcode 0x201f
    uint8_t                     status;                 // 1 octet
    uint16_t                    num_packets;            // 2 octets
} hcievt_0e_cmd_complete_leTestEnd_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_201f;        // 2 octets opcode 0x201f
    uint8_t                     status;                 // 1 octet
    uint16_t                    num_packets;            // 2 octets
    uint16_t			num_crcf;
    uint16_t			num_synclost;
} debug_hcievt_0e_command_complete_leTestEnd_debug_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2020;        // 2 octets opcode 0x2020
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
} hcievt_0e_cmd_complete_leRemoteConnParamReqReply_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2021;        // 2 octets opcode 0x2021
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
} hcievt_0e_cmd_complete_leRemoteConnParamReqNegativeReply_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2022;        // 2 octets opcode 0x2022
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
} hcievt_0e_cmd_complete_leSetDataLength_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2023;        // 2 octets opcode 0x2023
    uint8_t                     status;                 // 1 octet
    uint16_t                    connInitialMaxTxOctets; // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    connInitialMaxTxTime;   // Range 0x0148-0x4290
} hcievt_0e_cmd_complete_leReadSuggestedDefaultDataLength_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_202f;        // 2 octets opcode 0x202f
    uint8_t                     status;                 // 1 octet
    uint16_t                    supportedMaxTxOctets;   // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    supportedMaxTxTime;     // Range 0x0148-0x4290
    uint16_t                    supportedMaxRxOctets;   // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    uint16_t                    supportedMaxRxTime;     // Range 0x0148-0x4290
} hcievt_0e_cmd_complete_leReadMaximumDataLength_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2058;        // 2 octets opcode 0x2058
    uint8_t                     status;                 // 1 octet
    uint8_t                     supportedSwitchingSamplingRates;   // bit[0] 1 £gs switching supported for AoD transmission
    								   // bit[1] 1 £gs sampling supported for AoD reception
    								   // bit[2] 1 £gs switching and sampling supported for AoA reception    								   
    uint8_t                     numberofAntennae;      		   // Range 0x01 to 0x4B The number of antennae supported by the Controller 
    uint8_t                     maxLengthofSwitchingPattern;       // Range 0x02 to 0x4B Maximum length of antenna switching pattern supported by the Controller
    uint16_t                    maxCTELength;     		   // Range 0x02 to 0x14 Maximum length of a transmitted Constant Tone Extension supported in 8 £gs units 
} hcievt_0e_cmd_complete_leReadAntennaInformation_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0e;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode_2036;        // 2 octets opcode 0x2058
    uint8_t                     status;                 // 1 octet
    uint8_t                     selectedTxPower;   
} hcievt_0e_cmd_complete_leSetExtAdvParameters_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_0f;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     status;                 // 1 octet
    uint8_t                     num_hci_cmd_pkts;       // 1 octet
    uint16_t                    cmd_opcode;             // 2 octets
} hcievt_0f_cmd_status_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_13;          // 1 octet       HCIEVT_13_NUMBER_OF_COMPLETED_PACKETS
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     num_handles_01;         // 1 octet      (assumed 1)
    uint16_t                    conn_handle;            // 2 octets * 1 (assumed num_handles_01 == 1)
    uint16_t                    num_completed_pkts;     // 2 octets * 1 (assumed num_handles_01 == 1)
} hcievt_13_number_of_completed_packets_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_30;          // 1 octet
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
} hcievt_30_encryption_key_refresh_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_01;       // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     role;                   // 1 octet      0x00:master, 0x01:slave
    uint8_t                     peer_addr_type;         // 1 octet
    uint8_t                     peer_address[6];        // 6 octets
    uint16_t                    conn_interval;          // 2 octets
    uint16_t                    slave_latency;          // 2 octets
    uint16_t                    supervision_timeout;    // 2 octets
    uint8_t                     master_clock_accuracy;  // 1 octet

} hcievt_3e_01_LE_connection_complete_TypeDef;
/*
typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_02;       // 1 octet
    uint8_t                     num_reports;            // 1 octet
    uint8_t                     event_type;             // 1 octet  *N
    uint8_t                     address_type;           // 1 octet  *N
    uint8_t                     address[6];             // 6 octets *N
    uint8_t                     length_data;            // 1 octet  *N
    uint8_t                     Data[99];               // _ octets *N  << [length_data]
    uint8_t                     rssi;                   // 1 octet  *N

} hcievt_3e_02_LE_advertising_report_TypeDef;
*/
typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_03;       // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    conn_interval;          // 2 octets
    uint16_t                    slave_latency;          // 2 octets
    uint16_t                    supervision_timeout;    // 2 octets

} hcievt_3e_03_LE_conn_update_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_04;       // 1 octet
    uint8_t                     status;                 // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     LE_features[8];         // 8 octets

} hcievt_3e_04_LE_read_remote_feature_complete_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_05;       // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint8_t                     random_number[8];       // 8 octets
    uint8_t                     encrypted_diversifier[2];// 2 octets

} hcievt_3e_05_LE_LTK_request_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_06;       // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                    interval_min;           // 2 octets
    uint16_t                    interval_max;           // 2 octets
    uint16_t                    latency;                // 2 octets
    uint16_t                    timeout;                // 2 octets

} hcievt_3e_06_LE_remote_conn_param_request_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_07;       // 1 octet
    uint16_t                    conn_handle;            // 2 octets
    uint16_t                connEffectiveMaxTxOctets;   // 2 octets     the lesser of connMaxTxOctets and connRemoteMaxRxOctets
    uint16_t                connEffectiveMaxTxTime;     // 2 octets     
    uint16_t                connEffectiveMaxRxOctets;   // 2 octets     the lesser of connMaxRxOctets and connRemoteMaxTxOctets
    uint16_t                connEffectiveMaxRxTime;     // 2 octets     

} hcievt_3e_07_LE_data_length_change_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_13;       // 1 octet
    uint8_t                     adv_handle;             // 1 octet
    uint8_t                     scanner_addr_type;      // 1 octet
    uint8_t                     scanner_address[6];     // 6 octets

} hcievt_3e_13_LE_scan_request_received_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_15;       // 1 octet
    uint16_t                    sync_handle;            // 2 octets
    uint8_t                     channel_index;          // 1 octet
    uint16_t                    rssi;    	        // 2 octets
    uint8_t                     rssi_antenna_id;    	// 1 octet
    uint8_t			cte_type;		// 1 octet
    uint8_t			slot_durations;         // 1 octet
    uint8_t			packet_status;		// 1 octet
    uint16_t			periodic_event_counter; // 2 octets
    uint8_t			sample_count;		// 1 octet
    unsigned char iq_sample[164]; 
} hcievt_3e_15_LE_connectionless_iq_report_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_3e;          // 1 octet      0x3E:LE meta event
    uint8_t                     param_Total_Length;     // 1 octet
    uint8_t                     subevent_code_ff;       // 1 octet
    uint8_t                     param[37];              // 37 octets

} hcievt_3e_ff_LE_kidd_debug_TypeDef;

typedef struct __attribute__((packed))
{
    uint8_t                     event_code_57;          // 1 octet      0x57:HCIEVT_57_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED
    uint8_t                     param_Total_Length;     // 1 octet
    uint16_t                    conn_handle;            // 2 octets

} hcievt_57_authenticated_payload_timeout_expired_TypeDef;

/* Exported constants --------------------------------------------------------------------------------------*/

//===== HCI packet TYPE =======================================================
#define HCI_PTYPE_01_HC_COMMAND     0x01
#define HCI_PTYPE_02_ACL            0x02
//efine HCI_PTYPE_03_SCO            0x03
#define HCI_PTYPE_04_HCI_EVENT      0x04
//efine HCI_PTYPE_05_HCI_ERROR      0x05
//efine HCI_PTYPE_06_HCI_NEGOTIATE  0x06

//===== HCI COMMAND OGF =======================================================
#define OGF_01_LINK_CTRL            0x01
#define OGF_02_LINK_POLICY          0x02
#define OGF_03_HC_N_BASEBAND        0x03
#define OGF_04_INFO_PARAMS          0x04
#define OGF_05_STATUS_PARAMS        0x05
#define OGF_06_TESTING              0x06
#define OGF_08_LE                   0x08    //LowEnergy
#define OGF_3E_BLUETOOTH_LOGO_TEST  0x3E
#define OGF_3F_VENDOR_SPECIFIC      0x3F

//===== HCI COMMAND OCF with (OGF 0x01) =======================================
// OGF 0x01 link control commands
#define OCF_DISCONNECT                                0x006     // LE mandatory if supports Connection State
#define OCF_R_REMOTE_VER_INFO                         0x01D     // 

//===== HCI COMMAND OCF with (OGF 0x02) =======================================
// OGF 0x02 link policy commands

//===== HCI COMMAND OCF with (OGF 0x03) =======================================
// OGF 0x03 host controller & baseband commands
#define OCF_SET_EVENT_MASK                            0x001     // LE mandatory 
#define OCF_RESET                                     0x003     // LE mandatory 
#define OCF_SET_CONTROLLER_TO_HOST_FLOW_CONTROL       0x031     //                          Set Controller To Host Flow Control command
#define OCF_HOST_BUFFER_SIZE                          0x033     //C107: LE Mandatory if the Set Controller To Host Flow Control command is supported
#define OCF_HOST_NUM_OF_COMPLETED_PKTS                0x035     //C107: LE Mandatory if the Set Controller To Host Flow Control command is supported
#define OCF_SET_EVENT_MASK_PAGE_2                     0x063     //
#define OCF_R_AUTHENTICATED_PAYLOAD_TIMEOUT           0x07B     //C155: LE Mandatory if the Write Authenticated Payload Timeout command is supported, otherwise excluded
#define OCF_W_AUTHENTICATED_PAYLOAD_TIMEOUT           0x07C     //C7:   LE Mandatory if LE Feature (LE Encryption) and LE Feature (LE Ping) are supported, otherwise excluded

//===== HCI COMMAND OCF with (OGF 0x04) =======================================
// OGF 0x04 information parameters
#define OCF_R_LOC_VERSION_INFO                        0x001     // LE mandatory
#define OCF_R_LOC_SUPPORTED_COMMANDS                  0x002     // LE mandatory
#define OCF_R_LOC_SUPPORTED_FEATURES                  0x003     // LE mandatory
#define OCF_R_BD_ADDR                                 0x009     // LE mandatory

//===== HCI COMMAND OCF with (OGF 0x05) =======================================
// OGF 0x05 status parameters

//===== HCI COMMAND OCF with (OGF 0x06) =======================================
// OGF 0x06 testing commands

//===== HCI COMMAND OCF with (OGF 0x08) =======================================
// OGF 0x08 LowEnergy (LE) controller commands
#define OCF_001_LE_SET_EVENT_MASK                           0x001    //spec 4.0   mandatory
#define OCF_002_LE_R_BUFFER_SIZE_V1                         0x002    //           mandatory
#define OCF_003_LE_R_LOC_SUPPORTED_FEATURES                 0x003    //           mandatory
//efine OCF_004_LE_SET_LOC_USED_FEATURES                    0x004
#define OCF_005_LE_SET_RAND_ADDR                            0x005    //           C1:Tx
#define OCF_006_LE_SET_ADV_PARAM                            0x006    //           C1:Tx
#define OCF_007_LE_R_ADV_CHANNEL_TX_PWR                     0x007    //           C1:Tx
#define OCF_008_LE_SET_ADV_DATA                             0x008    //           C1:Tx
#define OCF_009_LE_SET_SCAN_RESP_DATA                       0x009
#define OCF_00A_LE_SET_ADV_ENABLE                           0x00A    //           C1:Tx
#define OCF_00B_LE_SET_SCAN_PARAM                           0x00B
#define OCF_00C_LE_SET_SCAN_ENABLE                          0x00C
#define OCF_00D_LE_CREATE_CONNECTION                        0x00D
#define OCF_00E_LE_CANCEL_CREATE_CONNECTION                 0x00E
#define OCF_00F_LE_R_WHITE_LIST_SIZE                        0x00F    //           mandatory
#define OCF_010_LE_CLEAR_WHITE_LIST                         0x010    //           mandatory
#define OCF_011_LE_ADD_DEVICE_TO_WHITE_LIST                 0x011    //           mandatory
#define OCF_012_LE_REMOVE_DEVICE_FM_WHITE_LIST              0x012    //           mandatory
#define OCF_013_LE_UPDATE_CONNECTION                        0x013
#define OCF_014_LE_SET_HOST_CHANNEL_CLASSIFICATION          0x014
#define OCF_015_LE_R_CHANNEL_MAP                            0x015
#define OCF_016_LE_R_REMOTE_FEATURES                        0x016
#define OCF_017_LE_ENCRYPT                                  0x017
#define OCF_018_LE_RAND                                     0x018
#define OCF_019_LE_ENABLE_ENCRYPTION                        0x019
#define OCF_01A_LE_LONG_TERM_KEY_REQ_REPLY                  0x01A
#define OCF_01B_LE_LONG_TERM_KEY_REQ_NEG_REPLY              0x01B
#define OCF_01C_LE_R_SUPPORTED_STATES                       0x01C    //           mandatory
#define OCF_01D_LE_RECEIVER_TEST_V1                         0x01D
#define OCF_01E_LE_TRANSMITTER_TEST_V1                      0x01E    //           C1:Tx
#define OCF_01F_LE_TEST_END                                 0x01F    //           mandatory
#define OCF_020_LE_REMOTE_CONN_PARAM_REQ_REPLY              0x020    //spec 4.1
#define OCF_021_LE_REMOTE_CONN_PARAM_REQ_NEG_REPLY          0x021    //spec 4.1
#define OCF_022_LE_SET_DATA_LEN                             0x022    //spec 4.2
#define OCF_023_LE_R_SUGGEST_DEFAULT_DATA_LEN               0x023    //spec 4.2
#define OCF_024_LE_W_SUGGEST_DEFAULT_DATA_LEN               0x024    //spec 4.2
#define OCF_025_LE_R_LOC_P256_PUBLIC_KEY                    0x025    //spec 4.2
#define OCF_026_LE_GENERATE_DHKEY_V1                        0x026    //spec 4.2
#define OCF_027_LE_ADD_DEVICE_TO_RESOLVING_LIST             0x027    //spec 4.2
#define OCF_028_LE_REMOVE_DEVICE_FROM_RESOLVING_LIST        0x028    //spec 4.2
#define OCF_029_LE_CLEAR_RESOLVING_LIST                     0x029    //spec 4.2
#define OCF_02A_LE_R_RESOLVING_LIST_SIZE                    0x02A    //spec 4.2
#define OCF_02B_LE_R_PEER_RESOLVABLE_ADDR                   0x02B    //spec 4.2
#define OCF_02C_LE_R_LOC_RESOLVABLE_ADDR                    0x02C    //spec 4.2
#define OCF_02D_LE_SET_ADDR_RESOLUTION_ENABLE               0x02D    //spec 4.2
#define OCF_02E_LE_SET_RESOLVABLE_PRIVATE_ADDR_TIMEOUT      0x02E    //spec 4.2
#define OCF_02F_LE_R_MAXIMUM_DATA_LEN                       0x02F    //spec 4.2
#define OCF_030_LE_R_PHY                                    0x030    //spec 5.0
#define OCF_031_LE_SET_DEFAULT_PHY                          0x031    //spec 5.0
#define OCF_032_LE_SET_PHY                                  0x032    //spec 5.0
#define OCF_033_LE_RECEIVER_TEST_V2                         0x033    //spec 5.0
#define OCF_034_LE_TRANSMITTER_TEST_V2                      0x034    //spec 5.0
#define OCF_035_LE_SET_ADV_SET_RANDOM_ADDR                  0x035    //spec 5.0
#define OCF_036_LE_SET_EXT_ADV_PARAM                        0x036    //spec 5.0
#define OCF_037_LE_SET_EXT_ADV_DATA                         0x037    //spec 5.0
#define OCF_038_LE_SET_EXT_SCAN_RSP_DATA                    0x038    //spec 5.0
#define OCF_039_LE_SET_EXT_ADV_ENABLE                       0x039    //spec 5.0
#define OCF_03A_LE_R_MAX_ADV_DATA_LENGTH                    0x03A    //spec 5.0
#define OCF_03B_LE_R_NUM_OF_SUPPORT_ADV_SETS                0x03B    //spec 5.0
#define OCF_03C_LE_REMOVE_ADV_SET                           0x03C    //spec 5.0
#define OCF_03D_LE_CLEAR_ADV_SETS                           0x03D    //spec 5.0
#define OCF_03E_LE_SET_PERIODIC_ADV_PARAM                   0x03E    //spec 5.0
#define OCF_03F_LE_SET_PERIODIC_ADV_DATA                    0x03F    //spec 5.0
#define OCF_040_LE_SET_PERIODIC_ADV_ENABLE                  0x040    //spec 5.0
#define OCF_041_LE_SET_EXT_SCAN_PARAM                       0x041    //spec 5.0
#define OCF_042_LE_SET_EXT_SCAN_ENABLE                      0x042    //spec 5.0
#define OCF_043_LE_EXT_CREATE_CONNECTION                    0x043    //spec 5.0
#define OCF_044_LE_PERIODIC_ADV_CREATE_SYNC                 0x044    //spec 5.0
#define OCF_045_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL          0x045    //spec 5.0
#define OCF_046_LE_PERIODIC_ADV_TERMINATE_SYNC              0x046    //spec 5.0
#define OCF_047_LE_ADD_DEVICE_TO_PERIODIC_ADV_LIST          0x047    //spec 5.0
#define OCF_048_LE_REMOVE_DEVICE_FM_PERIODIC_ADV_LIST       0x048    //spec 5.0
#define OCF_049_LE_CLEAR_PERIODIC_ADV_LIST                  0x049    //spec 5.0
#define OCF_04A_LE_R_PERIODIC_ADV_LIST_SIZE                 0x04A    //spec 5.0
#define OCF_04B_LE_R_TX_POWER                               0x04B    //spec 5.0
#define OCF_04C_LE_R_RF_PATH_COMPENSATION                   0x04C    //spec 5.0
#define OCF_04D_LE_W_RF_PATH_COMPENSATION                   0x04D    //spec 5.0
#define OCF_04E_LE_SET_PRIVACY_MODE                         0x04E    //spec 5.0
#define OCF_04F_LE_RECEIVER_TEST_V3                         0x04F    //spec 5.1
#define OCF_050_LE_TRANSMITTER_TEST_V3                      0x050    //spec 5.1
#define OCF_051_LE_SET_CONNLESS_CTE_TX_PARAM                0x051    //spec 5.1
#define OCF_052_LE_SET_CONNLESS_CTE_TX_ENABLE               0x052    //spec 5.1
#define OCF_053_LE_SET_CONNLESS_IQ_SAMPLING_ENABLE          0x053    //spec 5.1
#define OCF_054_LE_SET_CONN_CTE_RX_PARAM                    0x054    //spec 5.1
#define OCF_055_LE_SET_CONN_CTE_TX_PARAM                    0x055    //spec 5.1
#define OCF_056_LE_CONN_CTE_REQ_ENABLE                      0x056    //spec 5.1
#define OCF_057_LE_CONN_CTE_RSP_ENABLE                      0x057    //spec 5.1
#define OCF_058_LE_R_ANTENNA_INFO                           0x058    //spec 5.1
#define OCF_059_LE_SET_PERIODIC_ADV_RX_ENABLE               0x059    //spec 5.1
#define OCF_05A_LE_PERIODIC_ADV_SYNC_TRANSFER               0x05A    //spec 5.1
#define OCF_05B_LE_PERIODIC_ADV_SET_INFO_TRANS              0x05B    //spec 5.1
#define OCF_05C_LE_SET_PERIODIC_ADV_SYNC_TRANS_PARAM        0x05C    //spec 5.1
#define OCF_05D_LE_SET_DEFA_PERIODIC_ADV_SYNC_TRANS_PARAM   0x05D    //spec 5.1
#define OCF_05E_LE_GENERATE_DHKEY_V2                        0x05E    //spec 5.1
#define OCF_05F_LE_MODIFY_SLEEP_CLK_ACCURACY                0x05F    //spec 5.1
#define OCF_060_LE_R_BUFFER_SIZE_V2                         0x060    //spec 5.2
#define OCF_061_LE_READ_ISO_TX_SYNC                         0x061    //spec 5.2
#define OCF_062_LE_SET_CIG_PARAMETERS                       0x062    //spec 5.2
#define OCF_063_LE_SET_CIG_PARAMETERS_TEST                  0x063    //spec 5.2
#define OCF_064_LE_CREATE_CIS                               0x064    //spec 5.2
#define OCF_065_LE_REMOVE_CIG                               0x065    //spec 5.2
#define OCF_066_LE_ACCEPT_CIS_REQUEST                       0x066    //spec 5.2
#define OCF_067_LE_REJECT_CIS_REQUEST                       0x067    //spec 5.2
#define OCF_068_LE_CREATE_BIG                               0x068    //spec 5.2
#define OCF_069_LE_CREATE_BIG_TEST                          0x069    //spec 5.2
#define OCF_06A_LE_TERMINATE_BIG                            0x06A    //spec 5.2
#define OCF_06B_LE_BIG_CREATE_SYNC                          0x06B    //spec 5.2
#define OCF_06C_LE_BIG_TERMINATE_SYNC                       0x06C    //spec 5.2
#define OCF_06D_LE_REQUEST_PEER_SCA                         0x06D    //spec 5.2
#define OCF_06E_LE_SETUP_ISO_DATA_PATH                      0x06E    //spec 5.2
#define OCF_06F_LE_REMOVE_ISO_DATA_PATH                     0x06F    //spec 5.2
#define OCF_070_LE_ISO_TRANSMIT_TEST                        0x070    //spec 5.2
#define OCF_071_LE_ISO_RECEIVE_TEST                         0x071    //spec 5.2
#define OCF_072_LE_ISO_READ_TEST_COUNTERS                   0x072    //spec 5.2
#define OCF_073_LE_ISO_TEST_END                             0x073    //spec 5.2
#define OCF_074_LE_SET_HOST_FEATURE                         0x074    //spec 5.2
#define OCF_075_LE_READ_ISO_LINK_QUALITY                    0x075    //spec 5.2
#define OCF_076_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL       0x076    //spec 5.2
#define OCF_077_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL         0x077    //spec 5.2
#define OCF_078_LE_SET_PATH_LOSS_REPORTING_PARAMETERS       0x078    //spec 5.2
#define OCF_079_LE_SET_PATH_LOSS_REPORTING_ENABLE           0x079    //spec 5.2
#define OCF_07A_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE      0x07A    //spec 5.2
#define OCF_07B_LE_TRANSMITTER_TEST_V4                      0x07B    //spec 5.2
#define OCF_07C_LE_SET_DATA_RELATED_ADDRESS_CHANGES         0x07C    //spec 5.3
#define OCF_07D_LE_SET_DEFAULT_SUBRATE                      0x07D    //spec 5.3
#define OCF_07E_LE_SUBRATE_REQUEST                          0x07E    //spec 5.3


//===== HCI COMMAND OCF with (OGF 0x3F) =======================================
// OGF 0x3F verdor specific debug commands


//===== HCI EVENT opcode ======================================================
#define HCIEVT_05_DISCONN_COMPLETE                      0x05    // LE mandatory if supports Connection State
#define HCIEVT_08_ENCRYPTION_CHANGE                     0x08    // LE mandatory if LE Feature (LE Encryption) is supported
#define HCIEVT_0C_READ_REMOTE_VER_INFO_COMPLETE         0x0C    // 
#define HCIEVT_0E_CMD_COMPLETE                          0x0E    // LE mandatory
#define HCIEVT_0F_CMD_STATUS                            0x0F    // LE mandatory
#define HCIEVT_10_HARDWARE_ERROR                        0x10    // LE optional
#define HCIEVT_13_NUMBER_OF_COMPLETED_PACKETS           0x13    // C3: Mandatory if the LE Controller supports Connection State, otherwise excluded
#define HCIEVT_1A_DATA_BUFFER_OVERFLOW                  0x1A    // LE optional
#define HCIEVT_30_ENCRYPTION_KEY_REFRESH_COMPLETE       0x30    // LE mandatory if LE Feature (LE Encryption) is supported
#define HCIEVT_3E_LE_META_EVENT                         0x3E
#define HCIEVT_57_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED 0x57    // LE mandatory if the Write Authenticated Payload Timeout command is supported
#define HCIEVT_FF_VENDOR_SPECIFIC_DEBUG                 0xFF

//===== LE SUB-EVENT opcode ===================================================
#define SUBEVT_01_LE_CONN_COMPLETE                          0x01
#define SUBEVT_02_LE_ADVERTISING_REPORT                     0x02
#define SUBEVT_03_LE_CONN_UPDATE_COMPLETE                   0x03
#define SUBEVT_04_LE_READ_REMOTE_FEATURES                   0x04
#define SUBEVT_05_LE_LONG_TERM_KEY_REQUEST                  0x05
#define SUBEVT_06_LE_REMOTE_CONN_PARAM_REQUEST              0x06
#define SUBEVT_07_LE_DATA_LENGTH_CHANGE                     0x07
#define SUBEVT_08_LE_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE    0x08
#define SUBEVT_09_LE_GENERATE_DHKEY_COMPLETE                0x09
#define SUBEVT_0A_LE_ENHANCED_CONN_COMPLETE                 0x0A
#define SUBEVT_0B_LE_DIRECTED_ADV_REPORT                    0x0B
#define SUBEVT_0C_LE_PHY_UPDATE_COMPLETE                    0x0C
#define SUBEVT_0D_LE_EXTENDED_ADV_REPORT                    0x0D
#define SUBEVT_0E_LE_PERIODIC_ADV_SYNC_ESTABLISHED          0x0E
#define SUBEVT_0F_LE_PERIODIC_ADV_REPORT                    0x0F
#define SUBEVT_10_LE_PERIODIC_ADV_SYNC_LOST                 0x10
#define SUBEVT_11_LE_SCAN_TIMEOUT                           0x11
#define SUBEVT_12_LE_ADV_SET_TERMINATED                     0x12
#define SUBEVT_13_LE_SCAN_REQUEST_RECEIVED                  0x13
#define SUBEVT_14_LE_CHANNEL_SELECTION_ALGORITHM            0x14
#define SUBEVT_15_LE_CONNECTIONLESS_IQ_REPORT               0x15
#define SUBEVT_16_LE_CONN_IQ_REPORT                         0x16
#define SUBEVT_17_LE_CTE_REQUEST_FAILED                     0x17
#define SUBEVT_18_LE_PERIODIC_ADV_SYNC_TRANSFER_RECEIVED    0x18
#define SUBEVT_19_LE_CIS_ESTABLISHED                        0x19
#define SUBEVT_1A_LE_CIS_REQUEST                            0x1A
#define SUBEVT_1B_LE_CREATE_BIG_COMPLETE                    0x1B
#define SUBEVT_1C_LE_TERMINATE_BIG_COMPLETE                 0x1C
#define SUBEVT_1D_LE_BIG_SYNC_ESTABLISHED                   0x1D
#define SUBEVT_1E_LE_BIG_SYNC_LOST                          0x1E
#define SUBEVT_1F_LE_REQUEST_PEER_SCA_COMPLETE              0x1F
#define SUBEVT_20_LE_PATH_LOSS_THRESHOLD                    0x20
#define SUBEVT_21_LE_TRANSMIT_POWER_REPORTING               0x21
#define SUBEVT_22_LE_BIGINFO_ADV_REPORT                     0x22
#define SUBEVT_23_LE_SUBRATE_CHANGE                         0x23
#define SUBEVT_FF_LE_KIDD_DEBUG                             0xFF //debug

//===== ERROR CODES ===========================================================
// spec Vol-1, Part-F  Controller Error Codes
#define ERR_00_SUCCESS                                      0x00
#define ERR_01_UNKNOWN_HCI_COMMAND                          0x01
#define ERR_02_UNKNOWN_CONN_IDENTIFIER                      0x02    //the connection does not exist
#define ERR_03_HARDWARE_FAILURE                             0x03
#define ERR_04_PAGE_TIMEOUT                                 0x04
#define ERR_05_AUTH_FAILURE                                 0x05
#define ERR_06_PIN_OR_KEY_MISSING                           0x06
#define ERR_07_MEMORY_CAPACITY_EXCEEDED                     0x07
#define ERR_08_CONN_TIMEOUT                                 0x08
#define ERR_09_MAX_NUM_OF_CONNS                             0x09
#define ERR_0A_MAX_NUM_OF_SCO_CONNS_TO_A_DEVICE             0x0A
#define ERR_0B_ACL_CONN_ALREADY_EXISTS                      0x0B
#define ERR_0C_COMMAND_DISALLOWED                           0x0C
#define ERR_0D_HOST_REJECT_LIMITED_RESOURCES                0x0D
#define ERR_0E_HOST_REJECT_SECURITY_REASONS                 0x0E
#define ERR_0F_HOST_REJECT_REM_DEV_IS_ONLY_A_PERSONAL_DEV   0x0F
#define ERR_10_HOST_TIMEOUT                                 0x10
#define ERR_11_UNSUPPORTED_FEATURE_OR_PARAM_VALUE           0x11    //param in HCI command not supported. Not used in an LMP PDU
#define ERR_12_INVALID_HCI_CMD_PARAMS                       0x12
#define ERR_13_OTHER_END_TERMINATED_CONN_USER_END           0x13
#define ERR_14_OTHER_END_TERMINATED_CONN_LOW_RESOURCES      0x14
#define ERR_15_OTHER_END_TERMINATED_CONN_POWER_OFF          0x15
#define ERR_16_CONN_TERMINATED_BY_LOCAL_HOST                0x16
#define ERR_17_REPEATED_ATTEMPTS                            0x17
#define ERR_18_PAIRING_NOT_ALLOWED                          0x18
#define ERR_19_UNKNOWN_LMP_PDU                              0x19
#define ERR_1A_UNSUPPORTED_REMOTE_FEATURE                   0x1A    //remote does not support the feature associated with the issued command, LMP or LL Control PDU
#define ERR_1B_SCO_OFFSET_REJECTED                          0x1B
#define ERR_1C_SCO_INTERVAL_REJECTED                        0x1C
#define ERR_1D_SCO_AIR_MODE_REJECTED                        0x1D
#define ERR_1E_INVALID_LL_PARAMS                            0x1E
#define ERR_1F_UNSPECIFIED_ERROR                            0x1F
#define ERR_20_UNSUPPORTED_LL_PARAM_VALUE                   0x20
#define ERR_21_ROLE_CHANGE_NOT_ALLOWED                      0x21
#define ERR_22_LL_RESPONSE_TIMEOUT                          0x22    //
#define ERR_23_LL_PROCEDURE_COLLISION                       0x23    //     same procedure collision
#define ERR_24_LMP_PDU_NOT_ALLOWED                          0x24
#define ERR_25_ENCR_MODE_NOT_ACCEPTABLE                     0x25
#define ERR_26_UNIT_KEY_USED                                0x26
#define ERR_27_QOS_NOT_SUPPORTED                            0x27
#define ERR_28_INSTANT_PASSED                               0x28
#define ERR_29_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED          0x29
#define ERR_2A_DIFFERENT_TRANSACTION_COLLISION              0x2A    //different procedure collision
#define ERR_2C_QOS_UNACCEPTED_PARAMS                        0x2C
#define ERR_2D_QOS_REJECTED                                 0x2D
#define ERR_2E_CHANNEL_CLASSIFICATION_NOT_SUPPORTED         0x2E
#define ERR_2F_INSUFFICIENT_SECURITY                        0x2F
#define ERR_30_PARAM_OUT_OF_MANDATORY_RANGE                 0x30
#define ERR_32_ROLE_SWITCH_PENDING                          0x32
#define ERR_34_RESERVED_SLOT_VIOLATION                      0x34
#define ERR_35_ROLE_SWITCH_FAILED                           0x35
#define ERR_36_EXTENDED_INQ_RESPONSE_TOO_LARGE              0x36    //spec 2.1
#define ERR_37_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST         0x37    //spec 2.1
#define ERR_38_HOST_BUSY_PAIRING                            0x38    //spec 2.1
#define ERR_39_CONN_REJECTED_DUE_TO_NO_SUITABLE_CH_FOUND    0x39    //spec 3.0
#define ERR_3A_CONTROLLER_BUSY                              0x3A    //spec 3.0
#define ERR_3B_UNACCEPTABLE_CONN_PARAMS                     0x3B    //spec 4.0
#define ERR_3C_ADVERTISING_TIMEOUT                          0x3C    //spec 4.0
#define ERR_3D_CONN_TERMINATED_DUE_TO_MIC_FAILURE           0x3D    //spec 4.0
#define ERR_3E_CONN_FAILED_TO_BE_ESTABLISHED_SYNCH_TIMEOUT  0x3E    //spec 4.0
#define ERR_3F_PREVIOUSLY_USED                              0x3F    //spec 4.0
#define ERR_40_COARSE_CLOCK_ADJUSTMENT_REJECTED_BUT_WILL_TRY_TO_ADJUST_USING_CLOCK_DRAGGING 0x40      //spec 4.1
#define ERR_41_TYPE0_SUBMAP_NOT_DEFINED                     0x41    //spec 5.0
#define ERR_42_UNKNOWN_ADVERTISING_IDENTIFIER               0x42    //spec 5.0
#define ERR_43_LIMIT_REACHED                                0x43    //spec 5.0
#define ERR_44_OPERATION_CANCELLED_BY_HOST                  0x44    //spec 5.0
#define ERR_45_PACKET_TOO_LONG                              0x45    //spec 5.1

/* Exported functions --------------------------------------------------------------------------------------*/
void initial_hc(void);
void process_hc(void);

void ur0isr_receive_cobs_block(uint8_t ch);

bool receive_01_hciCommandPacket                      ( HCI_01_COMMAND_PACKET_TypeDef * );

bool hcmd_0406_disconnect                             ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_041d_read_remote_version_information        ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_0c01_set_event_mask                         ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_0c03_reset                                  ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_0c63_set_event_mask_page_2                  ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_0c7b_read_authenticated_payload_timeout     ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_0c7c_write_authenticated_payload_timeout    ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_1001_read_local_version_info                ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_1002_read_local_supported_commands          ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_1003_read_local_supported_features          ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_1009_read_bd_addr                           ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2001_le_set_event_mask                      ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2002_le_read_buffer_size_v1                 ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2003_le_read_local_supported_features       ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2005_le_set_random_address                  ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2006_le_set_adv_parameters                  ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2007_le_read_adv_channel_tx_power           ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2008_le_set_adv_data                        ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2009_le_set_scanresp_data                   ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200a_le_set_adv_enable                      ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200b_le_set_scan_parameters                 ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200c_le_set_scan_enable                     ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200d_le_create_connection                   ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200e_le_create_connection_cancel            ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_200f_le_read_white_list_size                ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2010_le_clear_white_list                    ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2011_le_add_device_to_white_list            ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2012_le_remove_device_from_white_list       ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2013_le_connection_update                   ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2014_le_set_host_channel_classification     ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2015_le_read_channel_map                    ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2016_le_read_remote_features                ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2017_le_encrypt                             ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2019_le_enable_encryption                   ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_201a_le_LTK_request_reply                   ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_201b_le_LTK_request_negative_reply          ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_201c_le_read_supported_states               ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_201e_le_transmitter_test_v1                 ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_201f_le_test_end                            ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2020_le_remote_conn_param_req_reply         ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2021_le_remote_conn_param_req_negative_reply( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2022_le_set_data_length                     ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2023_le_read_suggested_default_data_length  ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2024_le_write_suggested_default_data_length ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_202f_le_read_maximum_data_length            ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2033_le_receiver_test_v2		      ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2034_le_transmitter_test_v2                 ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2040_le_set_periodic_advertising_enable_debug ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_204f_le_receiver_test_v3		      ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2050_le_transmitter_test_v3		      ( HCI_01_COMMAND_PACKET_TypeDef * );
bool hcmd_2058_le_read_antenna_information	      ( HCI_01_COMMAND_PACKET_TypeDef * );

bool send_04_hciEvent_05_disconnection_complete                    (uint16_t connHandle, uint8_t reason);
bool send_04_hciEvent_08_encryption_change                         (uint16_t connHandle, uint8_t status, uint8_t encryption_enabled);
bool send_04_hciEvent_0c_read_remote_version_info_complete         (LEACL_TypeDef *, uint8_t reason);
bool send_04_hciEvent_13_number_of_completed_packets               (uint16_t connHandle, uint16_t num_completed_packets);
bool send_04_hciEvent_30_encryption_key_refresh_complete           (uint16_t connHandle, uint8_t status);
bool send_04_hciEvent_57_authenticated_payload_timeout_expired     (uint16_t connHandle);

bool send_04_hciEvent_3e_01_LE_connection_complete                 (LEACL_TypeDef *);
bool send_04_hciEvent_3e_03_LE_conn_update_complete                (LEACL_TypeDef *, uint8_t reason);
bool send_04_hciEvent_3e_04_LE_read_remote_feature_complete        (LEACL_TypeDef *, uint8_t reason);
bool send_04_hciEvent_3e_05_LE_LTK_request                         (LEACL_TypeDef *pacl);
bool send_04_hciEvent_3e_06_LE_remote_conn_param_request           (LEACL_TypeDef *);
bool send_04_hciEvent_3e_07_LE_data_length_change                  (LEACL_TypeDef *, 
                                                                    uint16_t connEffectiveMaxTxOctets,
                                                                    uint16_t connEffectiveMaxTxTime,
                                                                    uint16_t connEffectiveMaxRxOctets,
                                                                    uint16_t connEffectiveMaxRxTime
                                                                   );
bool send_04_hciEvent_3e_ff_LE_kidd_debug                          (uint8_t length, uint8_t *pParam);

extern hcievt_0e_cmd_complete_leTestEnd_TypeDef leTestEnd_evt ;
extern debug_hcievt_0e_command_complete_leTestEnd_debug_TypeDef debug_leTestEnd_evt ;
extern bool send_04_hciEvent_3e_15_LE_connectionless_iq_report(uint8_t channel_index, uint8_t cte_type, uint8_t sample_count, unsigned char *iq_sample_value);

#endif /* __BC5602B_HC_H */
