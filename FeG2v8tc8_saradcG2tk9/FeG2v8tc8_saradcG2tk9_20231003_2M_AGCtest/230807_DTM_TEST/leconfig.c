/*********************************************************************************************************//**
 * @file    leconfig.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
 * @brief   This file provides all LE CONFIG functions.
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

#include "leconfig.h"

/* Private define ------------------------------------------------------------------------------------------*/

/* Private types -------------------------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/

le_0c01_event_mask_TypeDef leconfig_eventMask = {
    {0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x20}  // 0x08: read_remote_version_info_complete,    0x20:LE Meta Event
  //{0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00}  // 0x08: read_remote_version_info_complete,    0x20:LE Meta Event   7262 no LE meta event
};
le_0c63_event_mask_page2_TypeDef leconfig_eventMaskPage2 = {
    {0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00}  // bit-23: Authenticated Payload Timeout Expired event
};
le_2001_le_event_mask_TypeDef leconfig_leEventMask = {
    {0x6D,
                                   //bit 0  LE Connection Complete event
                                   //bit 1  LE Advertising Report event
                                   //bit 2  LE Connection Update Complete event
                                   //bit 3  LE Read Remote Features Complete event
                                   //bit 4  LE Long Term Key Request event
                                   //bit 5  LE Remote Connection Parameter Request event
                                   //bit 6  LE Data Length Change event
                                   //bit 7  LE Read Local P-256 Public Key Complete event
     0x00,
                                   //bit 8  LE Generate DHKey Complete event
                                   //bit 9  LE Enhanced Connection Complete event
                                   //bit 10 LE Directed Advertising Report event
                                   //bit 11 LE PHY Update Complete event
                                   //bit 12 LE Extended Advertising Report event
                                   //bit 13 LE Periodic Advertising Sync Established event
                                   //bit 14 LE Periodic Advertising Report event
                                   //bit 15 LE Periodic Advertising Sync Lost event
     0x00,
                                   //bit 16 LE Scan Timeout event
                                   //bit 17 LE Advertising Set Terminated event
                                   //bit 18 LE Scan Request Received event
                                   //bit 19 LE Channel Selection Algorithm event
                                   //bit 20 LE Connectionless IQ Report event
                                   //bit 21 LE Connection IQ Report event
                                   //bit 22 LE CTE Request Failed event
                                   //bit 23 LE Periodic Advertising Sync Transfer Received event
     0x00,
                                   //bit 24 LE CIS Established event
                                   //bit 25 LE CIS Request event
                                   //bit 26 LE Create BIG Complete event
                                   //bit 27 LE Terminate BIG Complete event
                                   //bit 28 LE BIG Sync Established event
                                   //bit 29 LE BIG Sync Lost event
                                   //bit 30 LE Request Peer SCA Complete event
                                   //bit 31 LE Path Loss Threshold event
     0x00,
                                   //bit 32 LE Transmit Power Reporting event
                                   //bit 33 LE BIGInfo Advertising Report event
     0x00,
     0x00,
     0x00}
};

le_0c7c_authenticated_payload_timeout_TypeDef leconfig_authenticatedPayloadTimeout = {
    (30000/10),   //authenticatedPayloadTO, 2 octets, Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms
};

le_1001_version_info_TypeDef leconfig_versionInfo = {
    0x0C,             //Link Layer Version
                                                        //LL Version    0x06: Bluetooth spec 4.0
                                                        //              0x07: Bluetooth spec 4.1
                                                        //              0x08: Bluetooth spec 4.2
                                                        //              0x09: Bluetooth spec 5.0
                                                        //              0x0A: Bluetooth spec 5.1
                                                        //              0x0B: Bluetooth spec 5.2
                                                        //              0x0C: Bluetooth spec 5.3
    0x0046,        //Company identifiers
                                                        //              0x0046	MediaTek, Inc
    0x0101        //Sub Version
};

le_1003_lmpFeatures_TypeDef leconfig_lmpFeatures = {
    {0x00,        //lmp_features[0]
     0x00,        //lmp_features[1]
     0x00,        //lmp_features[2]
     0x00,        //lmp_features[3]
     0x60,        //lmp_features[4]
                                   //bit 32=0:EV4 eSCO not supported
                                   //bit 33=0:EV5 eSCO not supported
                                   //bit 34=0:reserved
                                   //bit 35  :"AFH capable slave"
                                   //bit 36  :"AFH classification slave"
                                   //bit 37=1:BR/EDR Not Supported
                                   //bit 38=1:LE Supported (Controller)
                                   //bit 39  :3-slot EDR ACL packets
     0x00,        //lmp_features[5]
     0x00,        //lmp_features[6]
     0x00}        //lmp_features[7]
};

le_1009_bd_addr_TypeDef leconfig_bdaddr = {
    {0x00,0x00,0x00,0x00,0x00,0x00}        //le_public_AdvA[6]
};

le_2003_features_TypeDef leconfig_leFeatures = {
////spec Vol-6, Part-B  4.6   FEATURE SUPPORT
    {0x3F,        //features[0]
                                   //bit 0  LE Encryption                            LL_03_ENC_REQ,RSP, LL_05_START_ENC_REQ,RSP, LL_0A_PAUSE_ENC_REQ,RSP
                                   //bit 1  Connection Parameters Request procedure  LL_11_REJECT_EXT_IND, LL_0F_CONNECTION_PARAM_REQ,RSP
                                   //bit 2  Extended Reject Indication               LL_11_REJECT_EXT_IND
                                   //bit 3  Peripheral-initiated Features Exchange   LL_0E_SLAVE_FEATURE_REQ
                                   //bit 4  LE Ping                                  LL_12_PING_REQ,RSP, LE Ping procedure, LE Authenticated Payload Timeout
                                   //bit 5  LE Data Packet Length Extension          LL_14_LENGTH_REQ,RSP, Data Length Update procedure
                                   //bit 6  LL Privacy
                                   //bit 7  Extended Scanner Filter Policies
     0x00,        //features[1]
                                   //bit 8  LE 2M PHY Y N
                                   //bit 9  Stable Modulation Index - Transmitter Y N
                                   //bit 10 Stable Modulation Index - Receiver    Y N
                                   //bit 11 LE Coded PHY Y N
                                   //bit 12 LE Extended Advertising O N
                                   //bit 13 LE Periodic Advertising O N
                                   //bit 14 Channel Selection Algorithm #2 Y N
                                   //bit 15 LE Power Class 1 Y N
     0x00,        //features[2]
                                   //bit 16 Minimum Number of Used Channels procedure Y N
                                   //bit 17 Connection CTE Request  Y N
                                   //bit 18 Connection CTE Response Y N
                                   //bit 19 Connectionless CTE Transmitter O N
                                   //bit 20 Connectionless CTE Receiver    O N
                                   //bit 21 Antenna Switching During CTE Transmission (AoD) O N
                                   //bit 22 Antenna Switching During CTE Reception    (AoA) O N
                                   //bit 23 Receiving Constant Tone Extensions Y N
     0x00,        //features[3]
                                   //bit 24 Periodic Advertising Sync Transfer - Sender    Y N
                                   //bit 25 Periodic Advertising Sync Transfer - Recipient Y N
                                   //bit 26 Sleep Clock Accuracy Updates Y N
                                   //bit 27 Remote Public Key Validation N N
                                   //bit 28 Connected Isochronous Stream - Central    Y N
                                   //bit 29 Connected Isochronous Stream - Peripheral Y N
                                   //bit 30 Isochronous Broadcaster Y N
                                   //bit 31 Synchronized Receiver Y N
     0x00,        //features[4]
                                   //bit 32 Connected Isochronous Stream (Host Support) Y Y
                                   //bit 33 LE Power Control Request Y N ____(Bits 33 and 34 shall always have the same value)
                                   //bit 34 LE Power Control Request Y N ____(Bits 33 and 34 shall always have the same value)
                                   //bit 35 LE Path Loss Monitoring Y N
                                   //bit 36 Periodic Advertising ADI support O N
                                   //bit 37 Connection Subrating Y N
                                   //bit 38 Connection Subrating (Host Support) Y Y
                                   //bit 39 Channel Classification Y N
     0x00,        //features[5] Reserved for future use (used for specification development purposes)
     0x00,        //features[6] Reserved for future use (used for specification development purposes)
     0x00}        //features[7] Reserved for future use (used for specification development purposes)
};

le_2005_random_address_TypeDef leconfig_randomAddress = {
    {0x00,0x00,0x00,0x00,0x00,0x00}        //random_address[6]
};

le_2006_adv_param_TypeDef leconfig_AdvParam = {
    0x0800,   //adv_interval_min                           Range: 0x0020 to 0x4000, unit 625us, Time Range: 20 ms to 10.24 sec,    Default: N=0x0800 (1.28 second)
    0x0800,   //adv_interval_max                           Range: 0x0020 to 0x4000, unit 625us, Time Range: 20 ms to 10.24 sec,    Default: N=0x0800 (1.28 second)
    0x00,     //adv_type
                                                           //0x00: Connectable and scannable undirected advertising (ADV_IND) (default) 
                                                           //0x01: Connectable high duty cycle directed advertising (ADV_DIRECT_IND, high duty cycle)
                                                           //0x02: Scannable undirected advertising (ADV_SCAN_IND)
                                                           //0x03: Non connectable undirected advertising (ADV_NONCONN_IND)
                                                           //0x04: Connectable low duty cycle directed advertising (ADV_DIRECT_IND, low duty cycle)
    0x00,     //own_address_type
                                                           //0x00: Public Device Address(default)
                                                           //0x01: Random Device Address
                                                           //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
                                                           //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    0x00,     //peer_address_type
                                                           //0x00: Public Device Address(default) or Public Identity Address
                                                           //0x01: Random Device Address or Random (static) Identity Address
    {0x00,0x00,0x00,0x00,0x00,0x00},   //peer_address[6]
    0x07,     //adv_channel_map
                                                           //xxxxxxx1b: Channel 37 shall be used
                                                           //xxxxxx1xb: Channel 38 shall be used
                                                           //xxxxx1xxb: Channel 39 shall be used
    0x00      //adv_filter_policy
                                                           //0x00: Process scan and connection requests from all devices (i.e., the White List is not in use) (default)
                                                           //0x01: Process connection requests from all devices and only scan requests from devices that are in the White List
                                                           //0x02: Process scan requests from all devices and only connection requests from devices that are in the White List
                                                           //0x03: Process scan and connection requests only from devices in the White List
};


le_2008_adv_data_TypeDef leconfig_AdvData = {
    0x1F,     //length
   {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F       }
// {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
//  0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11       }
};

le_2009_scanresp_data_TypeDef leconfig_ScanRspData = {
    0x1F,     //length
   {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22       }
};

le_200b_scan_param_TypeDef leconfig_ScanParam = {
    0x01,     //le_scan_type
                                                        //0x00: Passive Scanning. No scanning PDUs shall be sent (default)
                                                        //0x01: Active scanning. Scanning PDUs may be sent
    0x0200,   //le_scan_interval                           Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    0x0010,   //le_scan_window                             Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    0x00,     //own_address_type
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    0x00      //scanning_filter_policy
                                                        //0x00: Accept all adv and scan response PDUs except directed adv PDUs not addressed to this device (default)
                                                        //0x01: Accept only adv and scan response PDUs from devices where the advertiser address is in the White List. Directed adv PDUs which are not addressed to this device shall be ignored
                                                        //0x02: Accept all adv and scan response PDUs except directed adv PDUs where the initiator's identity address does not address this device
                                                        //0x03: Accept all adv and scan response PDUs except:...
};

le_200d_create_connection_TypeDef leconfig_CreateConnection = {
    0x0200,                   //le_scan_interval        //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    0x0010,                   //le_scan_window          //Range:0x0004~0x4000  Default:0x0010 (10 ms), Time = N * 0.625 ms, Time Range: 2.5 ms to 10.24 s
    0x00,                     //initiator_filter_policy
	                                                    //0x00: White List is not used to determine which advertiser to connect to Peer_Address_Type and Peer_Address shall be used
	                                                    //0x01: White List is     used to determine which advertiser to connect to Peer_Address_Type and Peer_Address shall be ignored
    0x00,                     //peer_address_type
	                                                    //0x00: Public Device Address
	                                                    //0x01: Random Device Address
	                                                    //0x02: Public Identity Address (Corresponds to peer Resolvable Private Address)
	                                                    //0x03: Random (static) Identity Address (Corresponds to peer Resolvable Private Address)
    {0x00,0x00,0x00,0x00,0x00,0x00},     //peer_address[6]
    0x00,                     //own_address_type
    0x002A,                   //conn_interval_min   (2 octets) unit 1.25ms
    0x002A,                   //conn_interval_max   (2 octets) unit 1.25ms
    0x0000,                   //slave_latency       (2 octets)
    0x0C80,                   //supervision_timeout (2 octets) unit 10ms,   range: 100ms to 32.0sec (0x000A to 0x0C80)
    0x0000,                   //min_CE_length           // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
    0x0000                    //max_CE_length           // Range 0x0000-0xFFFF   Time = N * 0.625 ms    Time Range: 0 ms to 40.9 s
};

le_2011_white_list_TypeDef leconfig_whiteList[LE_WHITE_LIST_SIZE];

le_2014_host_channel_class_TypeDef leconfig_hostChannelClass = {
    {0xFF,0xFF,0xFF,0xFF,0x1F},                         //channel_map[5] 37 1-bit fields, Channel n is bad     = 0
                                                        //                                Channel n is unknown = 1
};

le_201e_tx_test_TypeDef leconfig_txTest;

le_201e_adv_data_TypeDef leconfig_txTestData = {
    0x25,     //length
   {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,     
    0x11, 0x11, 0x11, 0x11, 0x11}
};



le_2023_suggested_default_data_length_TypeDef leconfig_defaultDataLength = {    //initial data length
    0x00FB,                   //connInitialMaxTxOctets  // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    0x0148                    //connInitialMaxTxTime    // Range 0x0148-0x4290    (328)~(17040)
};

le_202f_maximum_data_length_TypeDef leconfig_maximumDataLength = {
    0x00FB,                   //supportedMaxTxOctets    // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    0x0148,                   //supportedMaxTxTime      // Range 0x0148-0x4290    (328)~(17040)
    0x00FB,                   //supportedMaxRxOctets    // Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
    0x0148                    //supportedMaxRxTime      // Range 0x0148-0x4290    (328)~(17040)
};
le_2036_ext_adv_param_TypeDef leconfig_ExtAdvParam = {		
    0x00,                       //adv_handle;
    0x0040, 			//adv_event_properties;	//bit[0] Connectable advertising
							//bit[1] Scannable advertising
							//bit[2] Directed advertising
							//bit[3] High Duty Cycle Directed Connectable advertising ( 3.75 ms Advertising Interval)
							//bit[4] Use legacy advertising PDUs
							//bit[5] Omit advertiser's address from all PDUs ("anonymous advertising")
							//bit[6] Include TxPower in the extended header of at least one advertising PDU 
    0x000000A0,            //primary_adv_interval_min;
    0x000000B0,            //primary_adv_interval_max;
    0x07,			 //primary_adv_channel_map;	//bit[0] Channel 37 shall be used ;bit[1] Channel 38 shall be used; bit[2] Channel 39 shall be used    
    0x00,                     	 //own_address_type;
	                                                    //0x00: Public Device Address(default)
	                                                    //0x01: Random Device Address
	                                                    //0x02: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use public address
	                                                    //0x03: Controller generates Resolvable Private Address based on the local IRK from resolving list. If resolving list contains no matching entry, use random address from LE_Set_Random_Address
    0x00,	                 //peer_address_type;
	                                                    //0x00: Public Device Address(default) or Public Identity Address
	                                                    //0x01: Random Device Address or Random (static) Identity Address
    {0xFF,0xFE,0xFD,0xFC,0xFB,0xFA},//peer_address[6];
    0x00,	                    // adv_filter_policy;
                                                        //0x00: Process scan and connection requests from all devices (i.e., the White List is not in use) (default)
                                                        //0x01: Process connection requests from all devices and only scan requests from devices that are in the White List
                                                        //0x02: Process scan requests from all devices and only connection requests from devices that are in the White List
                                                        //0x03: Process scan and connection requests only from devices in the White List
    0x05,			//adv_tx_power;		//Range: -127 to +20 Units: dBm
    0x01,			//primary_adv_phy;	//0x01 Primary advertisement PHY is LE 1M 
    							//0x03 Primary advertisement PHY is LE Coded
    0x00,			//secondary_adv_max_skip;	//0x00 AUX_ADV_IND shall be sent prior to the next advertising event
    							//0x01 to 0xFF Maximum advertising events the Controller can skip before sending the AUX_ADV_IND packets on the secondary advertising physical channel 							
    0x01,			//secondary_adv_max_phy;	//0x01 Secondary advertisement PHY is LE 1M
    							//0x02 Secondary advertisement PHY is LE 2M
    							//0x03 Secondary advertisement PHY is LE Coded
    0x00,			//adv_sid;			//0x00 to 0x0F Value of the Advertising SID subfield in the ADI field of the PDU
    0x00			//scan_req_notification_enable;	//0x00 Scan request notifications disabled ; 0x01 enable

};
le_203e_periodic_adv_param_TypeDef leconfig_PeriodicAdvParam = {
    0x00,                      //adv_handle;
    0x00A0,                    //periodic_adv_interval_min;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)	
    0x00B0,                    //periodic_adv_interval_max;       	//Range: 0x0006 to 0xFFFF, unit 625us, Time Range: 7.5ms to 81.91875s,    Default: N=0x0800 (1.28 second)
    0x0040		      //periodic_adv_properties;	        //bit[6] Include TxPower in the advertising PDU   
};

le_2051_connectionless_cte_transmit_parameters_TypeDef leconfig_ConnectionlessCTETxAdvParam = {
    0x00,                     //adv_handle;  // 
    0x14,                     //cte_length;    // 0x02 to 0x14 Constant Tone Extension length in 8 �gs units
    0x00,                     //cte_type;    // 0x00:AoA , 0x01:AoD 1us , 0x02:AoD 2us
    0x02,		      //cte_count;   //The number of Constant Tone Extensions to transmit in each periodic advertising interval Range: 0x01 to 0x10
    0x01,		      //length_of_switching_pattern_length  // The number of Antenna IDs in the pattern 
    0x00		      //antenna_ids;	
};	
/* Global functions ----------------------------------------------------------------------------------------*/
void reset_leconfig_hostChannelClass(void)
{
    leconfig_hostChannelClass.channel_map[0] = 0xFF;    //channel_map[] 37 1-bit fields, Channel n is bad = 0, unknown = 1
    leconfig_hostChannelClass.channel_map[1] = 0xFF;    //channel_map[] 37 1-bit fields, Channel n is bad = 0, unknown = 1
    leconfig_hostChannelClass.channel_map[2] = 0xFF;    //channel_map[] 37 1-bit fields, Channel n is bad = 0, unknown = 1
    leconfig_hostChannelClass.channel_map[3] = 0xFF;    //channel_map[] 37 1-bit fields, Channel n is bad = 0, unknown = 1
    leconfig_hostChannelClass.channel_map[4] = 0x1F;    //channel_map[] 37 1-bit fields, Channel n is bad = 0, unknown = 1
}

void initial_leconfig(void)
{
    reset_leconfig_hostChannelClass();
}
