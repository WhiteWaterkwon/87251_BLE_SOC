/*********************************************************************************************************//**
 * @file    ringbuf.h
 * @version $Rev:: 100          $
 * @date    $Date:: 2022-02-22 #$
 * @brief   The header file of the RINGBUF functions.
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
#ifndef __RINGBUF_H
#define __RINGBUF_H

/* Includes ------------------------------------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------------------------------------*/
#define TOTAL_NUM_ELEMENTS_llcTxQ      (2U << 0)  // must be power of 2, must >= TOTAL_NUM_LE_LLC_PACKETS_TX         minimum:2
#define TOTAL_NUM_ELEMENTS_llcTxQ_high (TOTAL_NUM_ELEMENTS_llcTxQ)
#define TOTAL_NUM_ELEMENTS_llcRxQ      (1U << 0)  // must be power of 2, must >= TOTAL_NUM_LE_LLC_PACKETS_RX
#define TOTAL_NUM_ELEMENTS_lldataTxQ   (2U << 0)  // must be power of 2, must >= TOTAL_NUM_LE_ACL_DATA_PACKETS_TOAIR
#define TOTAL_NUM_ELEMENTS_lldataRxQ   (2U << 0)  // must be power of 2, must >= TOTAL_NUM_LE_ACL_DATA_PACKETS_TOHOST


/* Exported types ------------------------------------------------------------------------------------------*/

typedef struct
{
    void * pRBuf[TOTAL_NUM_ELEMENTS_llcTxQ];    // element: pointer to llc_buffer_tx2air_TypeDef
    uint32_t widx;
    uint32_t ridx;
} llcTxQ_TypeDef ;                              // ring buffer queue, send to air

typedef struct
{
    void * pRBuf[TOTAL_NUM_ELEMENTS_llcRxQ];    // element: pointer to llc_buffer_rx_TypeDef
    uint32_t widx;
    uint32_t ridx;
} llcRxQ_TypeDef ;                              // ring buffer queue, buffer received air packets

typedef struct
{
    void * pRBuf[TOTAL_NUM_ELEMENTS_lldataTxQ]; // element: pointer to hc_leacl_data_buffer_toair_TypeDef
    uint32_t widx;
    uint32_t ridx;
} lldataTxQ_TypeDef ;                           // ring buffer queue, buffer received hci packets, then send to air

typedef struct
{
    void * pRBuf[TOTAL_NUM_ELEMENTS_lldataRxQ]; // element: pointer to hc_leacl_data_buffer_tohost_TypeDef
    uint32_t widx;
    uint32_t ridx;
} lldataRxQ_TypeDef ;                           // ring buffer queue, buffer received air packets, then send to host



/* Exported macro ------------------------------------------------------------------------------------------*/
#define RINGBUF_init( ringbuf )                 { ringbuf.widx = ringbuf.ridx = 0; }
#define RINGBUF_length( ringbuf )               ((typeof(ringbuf.widx))(( ringbuf.widx ) - ( ringbuf.ridx )))
#define RINGBUF_isEmpty( ringbuf )              ( ringbuf.widx == ringbuf.ridx )
#define RINGBUF_isNonempty( ringbuf )           ( ringbuf.widx != ringbuf.ridx )
#define RINGBUF_isFull( ringbuf, totalnum )     ( RINGBUF_length( ringbuf ) == ( totalnum ) )
#define RINGBUF_push( ringbuf, element, totalnum ) { \
                                                 (ringbuf.pRBuf)[ ringbuf.widx & ((totalnum)-1) ] = (void *)(element); \
                                                  ringbuf.widx++; }
#define RINGBUF_peek( ringbuf, totalnum )       ((ringbuf.pRBuf)[ ringbuf.ridx & ((totalnum)-1) ] )
#define RINGBUF_incrementPopIndex( ringbuf )    { ringbuf.ridx++; }

/* Exported functions --------------------------------------------------------------------------------------*/


#endif /* __RINGBUF_H ------------------------------------------------------------------------------------*/
