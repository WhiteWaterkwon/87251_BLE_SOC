/*********************************************************************************************************//**
 * @file    lecal.c
 * @version $Rev:: 100          $
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

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>                           //printf  debug
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include "hwlib/socal/hps.h"
#include "hwlib/socal/socal.h"
#include "hwlib/socal/alt_uart.h"

#include "bc5602b_host.h"                    //#define  __MALLOC_METHOD__
#include "llc.h"                             //llc_buffer_tx2air_TypeDef
#include "lldata.h"                          //hc_leacl_data_buffer_toair_TypeDef,  LENGTH_LE_ACL_DATA_PACKET_TOHOST
#include "leconfig.h"                        //leconfig_defaultDataLength
#include "leacl.h"


/* Private define ------------------------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/
dllist_TypeDef           leACLlist;



/* Global functions ----------------------------------------------------------------------------------------*/

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
static volatile uint16_t leacl_conn_hdl70 = 0x122;//0x0000~0x0EFF
uint16_t GetNewValueConnHandle(void)
{
    LEACL_TypeDef *pacl;
    while(1) {
            leacl_conn_hdl70 ++ ;
        if (leacl_conn_hdl70 > 0x0EFF)          //0x0000~0x0EFF
            leacl_conn_hdl70 = 0x0000 ;
        //should avoid re-use existing handle
            pacl = leacl_with_connHandle(leacl_conn_hdl70);
        if (pacl == 0) { //unused
            break;
        }
    }
        return( leacl_conn_hdl70 );
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
LEACL_TypeDef * leacl_alloc(void)
{
    LEACL_TypeDef *pacl;
    #if    __MALLOC_METHOD__  == 1
        pacl = (LEACL_TypeDef *)      malloc(sizeof(LEACL_TypeDef));
    #elif  __MALLOC_METHOD__  == 2
        pacl = (LEACL_TypeDef *)pvPortMalloc(sizeof(LEACL_TypeDef));
    #else
    ...
    #endif
    if (pacl != 0)
    {
        ht_memory_set( (uint8_t *)pacl, 0x00, sizeof(LEACL_TypeDef));
        RINGBUF_init( pacl->llcTxQ_high );
        RINGBUF_init( pacl->llcTxQ );
        RINGBUF_init( pacl->llcRxQ );
        RINGBUF_init( pacl->lldataTxQ );
        RINGBUF_init( pacl->lldataRxQ );
        pacl->aclConnHandle = GetNewValueConnHandle();
        //For a new connection, connMaxTxOctets shall be set to connInitialMaxTxOctets and 
        //                      connMaxRxOctets shall be chosen by the Controller
        //                      connMaxTxTime   shall be set to connInitialMaxTxTime and 
        //                      connMaxRxTime   shall be chosen by the Controller
        pacl->connMaxRxOctets          =                   LENGTH_LE_ACL_DATA_PACKET_TOHOST;       //Range 0x001B-0x00FB   (31-4)~(255-4)    -4:MIC
        pacl->connMaxRxTime            = ((uint16_t)(1+4+2+LENGTH_LE_ACL_DATA_PACKET_TOHOST+3))*8; //Range 0x0148-0x4290    (328)~(17040)  1*8+(4*8+(2+31)*8+3*8)=328 us, 80+(4*8+2+3+(2+255)*8+3*8+3)*8=17040 us
        pacl->connMaxTxOctets          = leconfig_defaultDataLength.connInitialMaxTxOctets;
        pacl->connMaxTxTime            = leconfig_defaultDataLength.connInitialMaxTxTime;
        pacl->connRemoteMaxRxOctets    = 27;     //251
        pacl->connRemoteMaxRxTime      = 0x0148;
        pacl->connRemoteMaxTxOctets    = 27;     //251
        pacl->connRemoteMaxTxTime      = 0x0148;
        pacl->connEffectiveMaxRxOctets = 27;     //251
        pacl->connEffectiveMaxRxTime   = 0x0148;
        pacl->connEffectiveMaxTxOctets = 27;     //251
        pacl->connEffectiveMaxTxTime   = 0x0148;
        //
      kidd_taskENTER_CRITICAL();
        dllistPush( (dllist_TypeDef *)(&leACLlist), (dllist_node_TypeDef *)pacl );
      kidd_taskEXIT_CRITICAL();
//printf("%08X, ", (unsigned int)leACLlist.front);
//printf("%08X\n", (unsigned int)leACLlist.rear);
        return pacl;
    }
    else
    {
        return 0;
    }
}

void leacl_delete(LEACL_TypeDef *pacl)
{
    if (pacl->pTemplateConnParam   != 0) {             free( (void *)(pacl->pTemplateConnParam) );    }
    
    if (pacl->pRemoteFeature != 0) {
        #if    __MALLOC_METHOD__  == 1
             free( (void *)(pacl->pRemoteFeature) );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)(pacl->pRemoteFeature) );
        #endif
    }
    if (pacl->pRemoteVersInfo != 0) {
        #if    __MALLOC_METHOD__  == 1
             free( (void *)(pacl->pRemoteVersInfo) );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)(pacl->pRemoteVersInfo) );
        #endif
    }
    if (pacl->pNewUpdate != 0) {
        #if    __MALLOC_METHOD__  == 1
             free( (void *)(pacl->pNewUpdate) );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)(pacl->pNewUpdate) );
        #endif
    }
    if (pacl->pNewChM != 0) {
        #if    __MALLOC_METHOD__  == 1
             free( (void *)(pacl->pNewChM) );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)(pacl->pNewChM) );
        #endif
    }
        while( RINGBUF_isNonempty       ( pacl->llcTxQ_high ) ) {
            llcBuffer_tx2air_free( (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek( pacl->llcTxQ_high, TOTAL_NUM_ELEMENTS_llcTxQ_high) ) );
               RINGBUF_incrementPopIndex( pacl->llcTxQ_high );
        }
        while( RINGBUF_isNonempty       ( pacl->llcTxQ ) ) {
            llcBuffer_tx2air_free( (llc_buffer_tx2air_TypeDef *)( RINGBUF_peek( pacl->llcTxQ, TOTAL_NUM_ELEMENTS_llcTxQ) ) );
               RINGBUF_incrementPopIndex( pacl->llcTxQ );
        }
        while( RINGBUF_isNonempty       ( pacl->llcRxQ ) ) {
            llcBuffer_rx_free    ( (llc_buffer_rx_TypeDef *)    ( RINGBUF_peek( pacl->llcRxQ, TOTAL_NUM_ELEMENTS_llcRxQ) ) );
               RINGBUF_incrementPopIndex( pacl->llcRxQ );
        }
        while( RINGBUF_isNonempty       ( pacl->lldataTxQ ) ) {
            hcAclDataBuffer_toair_free ( (hc_leacl_data_buffer_toair_TypeDef *) ( RINGBUF_peek( pacl->lldataTxQ, TOTAL_NUM_ELEMENTS_lldataTxQ) ) );
               RINGBUF_incrementPopIndex( pacl->lldataTxQ );
        }
        while( RINGBUF_isNonempty       ( pacl->lldataRxQ ) ) {
            hcAclDataBuffer_tohost_free( (hc_leacl_data_buffer_tohost_TypeDef *)( RINGBUF_peek( pacl->lldataRxQ, TOTAL_NUM_ELEMENTS_lldataRxQ) ) );
               RINGBUF_incrementPopIndex( pacl->lldataRxQ );
        }

      kidd_taskENTER_CRITICAL();
        dllistDeleteNode( (dllist_TypeDef *)(&leACLlist), (dllist_node_TypeDef *)pacl );
        #if    __MALLOC_METHOD__  == 1
             free( (void *)pacl );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)pacl );
        #endif
      kidd_taskEXIT_CRITICAL();
//printf("%08X, ", (unsigned int)leACLlist.front);
//printf("%08X\n", (unsigned int)leACLlist.rear);
}

LEACL_TypeDef * leacl_with_connHandle(uint16_t handle)
{
    dllist_node_TypeDef *pNode;
    LEACL_TypeDef *pacl;
           pacl  = 0;
           pNode = leACLlist.front ;
    while (pNode != 0)
    {
            pacl = (LEACL_TypeDef *)(pNode) ;
        if (LEACL_conn_handle(pacl) == handle) {
            break;//end while
        }
        pNode = pNode->next;
    }
    if (pNode == 0)
        return 0;
    else
        return pacl;
}
LEACL_TypeDef * ur0isr_leacl_with_connHandle(uint16_t handle)
{
    dllist_node_TypeDef *pNode;
    LEACL_TypeDef *pacl;
           pacl  = 0;
           pNode = leACLlist.front ;
    while (pNode != 0)
    {
            pacl = (LEACL_TypeDef *)(pNode) ;
        if (LEACL_conn_handle(pacl) == handle) {
            break;//end while
        }
        pNode = pNode->next;
    }
    if (pNode == 0)
        return 0;
    else
        return pacl;
}

LEACL_TypeDef * leacl_with_peerAddress(uint8_t *pPeerAddress)
{
    dllist_node_TypeDef *pNode;
    LEACL_TypeDef *pacl;
           pacl  = 0;
           pNode = leACLlist.front ;
    while (pNode != 0)
    {
            pacl = (LEACL_TypeDef *)(pNode) ;
        if (ht_memory_compare(pPeerAddress, pacl->remoteDeviceAddr, 6) == 0) { // difference == 0
            break;//end while
        }
        pNode = pNode->next;
    }
    if (pNode == 0)
        return 0;
    else
        return pacl;
}

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/



/**
  * @}
  */
