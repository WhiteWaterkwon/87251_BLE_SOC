/*********************************************************************************************************//**
 * @file    lldata.h
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
#ifndef HT_LLDATA_H
#define HT_LLDATA_H

/* Includes ------------------------------------------------------------------------------------------------*/
#include "leacl.h"          //LEACL_TypeDef



/* Exported constants --------------------------------------------------------------------------------------*/


/* Exported types ------------------------------------------------------------------------------------------*/

#define LENGTH_LE_ACL_DATA_PACKET_TOAIR        (255-4)   // (31-4)~(255-4)    -4:MIC
#define TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR    (TOTAL_NUM_ELEMENTS_lldataTxQ - 0)
typedef struct //__attribute__((packed))
{
  //uint8_t                     octet[LENGTH_LE_ACL_DATA_PACKET_TOAIR];   // 0~( 31-4) octets   4:MIC
    uint8_t                     octet[                     (256/4)<<2];   // 0~( 31-4) octets   4:MIC
    uint32_t                    ram_start_address ;
    uint16_t                    hci_hdr_bc_pb_hdl ;                 //[15:14]:BC flag, [13:12]:PB flag, [11:0]:handle
    uint16_t                    hci_hdr_length ;                    //[15: 0]:length
    uint16_t                    r_index ;           // read  index, not include sizeof hci header
    uint16_t                    r_idx_w4_acked ;    // read  index, not include sizeof hci header
    uint16_t                     tttstate ;          // 0x00:free, 0x01:booked, 0x02:loaded
    uint16_t                     air_pb_start;       //

}      hc_leacl_data_buffer_toair_TypeDef ;
extern hc_leacl_data_buffer_toair_TypeDef HcAclDataBUFF2air[TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR];


#define LENGTH_LE_ACL_DATA_PACKET_TOHOST       (255-4)   // (31-4)~(255-4)    -4:MIC
#define TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST   (TOTAL_NUM_ELEMENTS_lldataRxQ - 0)
typedef struct //__attribute__((packed))
{
  //uint8_t                     octet[LENGTH_LE_ACL_DATA_PACKET_TOHOST];   // 0~( 31-4) octets   4:MIC
    uint8_t                     octet[                      (256/4)<<2];   // 0~( 31-4) octets   4:MIC
  //uint16_t                    hci_hdr_bc_pb_hdl ;                 //[15:14]:BC flag, [13:12]:PB flag, [11:0]:handle
  //uint16_t                    hci_hdr_length ;                    //[15: 0]:length
    uint32_t                    received_AESCCM_MIC_F_C0h;
    uint16_t                     rrrstate ;          // 0x00:free, 0x01:booked
    uint16_t                     length;
    uint16_t                    hci_pb_start;           // 0x2000:start, 0x1000:continue
    uint16_t                    connection_handle;

}      hc_leacl_data_buffer_tohost_TypeDef ;
extern hc_leacl_data_buffer_tohost_TypeDef HcAclDataBUFF2host[TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST];


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/
void process_lldataRxQ(LEACL_TypeDef *);

void hcAclDataBuffer_toair_init(void);
void hcAclDataBuffer_tohost_init(void);

bool receive_02_hciAclDataPacket_fromhost(uint16_t bc_pb_hdl, uint16_t dataTotalLength, uint8_t *pData);

bool send_02_hciAclDataPacket_tohost(uint16_t bc_pb_hdl, uint16_t dataTotalLength, uint8_t *pData);

static inline void hcAclDataBuffer_toair_free(hc_leacl_data_buffer_toair_TypeDef * pBuff)
{
    pBuff->tttstate = 0x00;  // 0x00:free, 0x01:booked
}
static inline void hcAclDataBuffer_tohost_free(hc_leacl_data_buffer_tohost_TypeDef * pBuff)
{
    pBuff->rrrstate = 0x00;  // 0x00:free, 0x01:booked
}
hc_leacl_data_buffer_toair_TypeDef * ur0isr_hcAclDataBuffer_toair_allocate(uint16_t bc_pb_hdl, uint16_t hdr_len);

                                 bool irq1isr_hcAclDataBuffer_tohost_isAvailable(void);
hc_leacl_data_buffer_tohost_TypeDef * irq1isr_hcAclDataBuffer_tohost_alloc(void);
                        unsigned char irq1isr_read_rxfifo_to_lldataRxQ(LEACL_TypeDef *);


#endif /* HT_LLDATA_H */
