/*********************************************************************************************************//**
 * @file    hcmsg.c
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
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcmp, memcpy

#include "bc5602b_host.h"                    //#define  __MALLOC_METHOD__
#include "pdma.h"
#include "usart.h"                           //uart_puts()
#include "lldata.h"
#include "hcmsg.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"


/* Private function prototypes -----------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/
htQueue_TypeDef hcmsgQ;
htQueue_TypeDef llcmsgQ;

/* Private types -------------------------------------------------------------------------------------------*/


/* Global functions ----------------------------------------------------------------------------------------*/
bool hcmsgQ_push(msgcodeHCMSG_t msgCode, uint32_t msgparam3210, uint32_t LengthExtramsgparam, uint8_t *pExtramsgparam)
               //uint32_t msgCode, 
               //uint32_t msgparam3210,         //msgparam[0],[1],[2],[3]
               //uint32_t LengthExtramsgparam,  //length of extra msgparam[] appended after msgparam+4
               //uint8_t *pExtramsgparam        //          extra msgparam[] appended after msgparam+4
{
    hcmsgQ_node_TypeDef *qNode;
    #if    __MALLOC_METHOD__  == 1
  //kidd_taskENTER_CRITICAL();
    qNode = (hcmsgQ_node_TypeDef *)      malloc(sizeof(hcmsgQ_node_TypeDef) + LengthExtramsgparam);
  //kidd_taskEXIT_CRITICAL();
    #elif  __MALLOC_METHOD__  == 2
    qNode = (hcmsgQ_node_TypeDef *)pvPortMalloc(sizeof(hcmsgQ_node_TypeDef) + LengthExtramsgparam);
    #else
    ...
    #endif
    if( qNode != 0 )
    {
            uint8_t *dst, *src;
        qNode->msgcode             = msgCode ;
        qNode->LengthExtramsgparam = LengthExtramsgparam ;
        qNode->msgparam[0]         = (uint8_t)(msgparam3210);
        qNode->msgparam[1]         = (uint8_t)(msgparam3210>>8);
        qNode->msgparam[2]         = (uint8_t)(msgparam3210>>16);
        qNode->msgparam[3]         = (uint8_t)(msgparam3210>>24);
            dst = qNode->msgparam+4; // +4: uint8_t msgparam[4]
            src = pExtramsgparam;
        while (LengthExtramsgparam) {
            *dst = *src;
             dst ++;
             src ++;
             LengthExtramsgparam --;
        }
        /////////
    //kidd_taskENTER_CRITICAL();
        htqueuePush( &(hcmsgQ), (void *)qNode );
    //kidd_taskEXIT_CRITICAL();
        return 1;
    }
    else
    {
uart_puts("hcmsgQ_push fail____"); uart_putchar_n('\n');
        return 0;
    }
}

bool llcmsgQ_push(msgcodeLLCMSG_t msgCode, uint32_t msgparam3210, uint32_t LengthExtramsgparam, uint8_t *pExtramsgparam)
               //uint32_t msgCode, 
               //uint32_t msgparam3210,         //msgparam[0],[1],[2],[3]
               //uint32_t LengthExtramsgparam,  //length of extra msgparam[] appended after msgparam+4
               //uint8_t *pExtramsgparam        //          extra msgparam[] appended after msgparam+4
{
    llcmsgQ_node_TypeDef *qNode;
    #if    __MALLOC_METHOD__  == 1
  //kidd_taskENTER_CRITICAL();
    qNode = (llcmsgQ_node_TypeDef *)      malloc(sizeof(llcmsgQ_node_TypeDef) + LengthExtramsgparam);
  //kidd_taskEXIT_CRITICAL();
    #elif  __MALLOC_METHOD__  == 2
    qNode = (llcmsgQ_node_TypeDef *)pvPortMalloc(sizeof(llcmsgQ_node_TypeDef) + LengthExtramsgparam);
    #else
    ...
    #endif
    if( qNode != 0 )
    {
            uint8_t *dst, *src;
        qNode->msgcode             = msgCode ;
        qNode->LengthExtramsgparam = LengthExtramsgparam ;
        qNode->msgparam[0]         = (uint8_t)(msgparam3210);
        qNode->msgparam[1]         = (uint8_t)(msgparam3210>>8);
        qNode->msgparam[2]         = (uint8_t)(msgparam3210>>16);
        qNode->msgparam[3]         = (uint8_t)(msgparam3210>>24);
            dst = qNode->msgparam+4; // +4: uint8_t msgparam[4]
            src = pExtramsgparam;
        while (LengthExtramsgparam) {
            *dst = *src;
             dst ++;
             src ++;
             LengthExtramsgparam --;
        }
        /////////
    //kidd_taskENTER_CRITICAL();
        htqueuePush( &(llcmsgQ), (void *)qNode );
    //kidd_taskEXIT_CRITICAL();
        return 1;
    }
    else
    {
uart_puts("llcmsgQ_push fail____"); uart_putchar_n('\n');
        return 0;
    }
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
void proc_hcmsgQ(void)
{
    bool done;
    hcmsgQ_node_TypeDef *qNode;
    if( (qNode = (hcmsgQ_node_TypeDef *)htqueuePeek(&(hcmsgQ))) == 0 )
    {
        return; //exit empty q
    }
        #if 0 //print debug
        {
            uint32_t i;
            uart_putchar_n('\n');
            uart_puts("proc_hcmsgQ ");
	        uart_putu32(qNode->msgcode);
            uart_puts(", ");
          for(i=0; i<(4+qNode->LengthExtramsgparam); i++) { //+4: uint8_t msgparam[4]
	        uart_putu8(qNode->msgparam[i]);
            uart_puts(" ");
	      }
            uart_putchar_n('\n');
	    }
	    #endif
    /////////////////////////
    done = 1;
    switch( qNode->msgcode )
    {
    case HCMSG_SEND_HCIEVT:
        done = proc_hcmsg_send_hcievent(qNode);
        break;
    case HCMSG_DELETE_LEACL_FREE_MEMORY:
        done = proc_hcmsg_delete_leacl_free_memory(qNode);
        break;
    case HCMSG_CHECK_CONN_EFFECTIVE_MAX_TRX_OCTETS_TIME_CHANGE:
        done = proc_hcmsg_check_conn_effective_max_trx_octets_time_change(qNode);
        break;
    }
    /////////////////////////
    if( done )
    {
            kidd_taskENTER_CRITICAL();
                htqueuePop(&(hcmsgQ));
            kidd_taskEXIT_CRITICAL();
        #if    __MALLOC_METHOD__  == 1
            kidd_taskENTER_CRITICAL();
                 free( (void *)qNode );
            kidd_taskEXIT_CRITICAL();
        #elif  __MALLOC_METHOD__  == 2
            vPortFree( (void *)qNode );
        #endif
    }
}

void proc_llcmsgQ(void)
{
    bool done;
    llcmsgQ_node_TypeDef *qNode;
    if( (qNode = (llcmsgQ_node_TypeDef *)htqueuePeek(&(llcmsgQ))) == 0 )
    {
        return; //exit empty q
    }
        #if 0 //print debug
        {
            uint32_t i;
            uart_putchar_n('\n');
            uart_puts("proc_llcmsgQ ");
	        uart_putu32(qNode->msgcode);
            uart_puts(", ");
          for(i=0; i<(4+qNode->LengthExtramsgparam); i++) { //+4: uint8_t msgparam[4]
	        uart_putu8(qNode->msgparam[i]);
            uart_puts(" ");
	      }
            uart_putchar_n('\n');
	    }
	    #endif
    /////////////////////////
    done = 1;
    switch( qNode->msgcode )
    {
    case LLCMSG_INITIATE_LLC_CONN_UPDATE_PROCEDURE:
        done = proc_llcmsg_initiate_llc_conn_update_procedure(qNode);        //send LL_00_CONNECTION_UPDATE_IND
        break;
    case LLCMSG_INITIATE_LLC_CHANNEL_MAP_UPDATE_PROCEDURE:
        done = proc_llcmsg_initiate_llc_channel_map_update_procedure(qNode); //send LL_01_CHANNEL_MAP_IND
        break;
    case LLCMSG_INITIATE_LLC_ACL_TERMINATION_PROCEDURE:
        done = proc_llcmsg_initiate_llc_acl_termination_procedure(qNode);    //send LL_02_TERMINATE_IND
        break;
    case LLCMSG_INITIATE_LLC_START_ENCRYPTION_PROCEDURE:
        done = proc_llcmsg_initiate_llc_encryption_start_procedure(qNode);   //send LL_03_ENC_REQ
        break;
    case LLCMSG_INITIATE_LLC_PAUSE_ENCRYPTION_PROCEDURE:
        done = proc_llcmsg_initiate_llc_encryption_pause_procedure(qNode);   //send LL_0A_PAUSE_ENC_REQ
        break;
    case LLCMSG_INITIATE_LLC_FEATURE_EXCHANGE_PROCEDURE:
        done = proc_llcmsg_initiate_llc_feature_exchange_procedure(qNode);   //send LL_08_FEATURE_REQ/LL_0E_SLAVE_FEATURE_REQ, wait LL_09_FEATURE_RSP
        break;
    case LLCMSG_INITIATE_LLC_VERSION_EXCHANGE_PROCEDURE:
        done = proc_llcmsg_initiate_llc_version_exchange_procedure(qNode);   //send LL_0C_VERSION_IND, wait LL_0C_VERSION_IND
        break;
    case LLCMSG_INITIATE_LLC_CONN_PARAMS_REQUEST_PROCEDURE:
        done = proc_llcmsg_initiate_llc_conn_params_request_procedure(qNode);//send LL_0F_CONNECTION_PARAM_REQ
        break;
    case LLCMSG_INITIATE_LLC_LE_PING_PROCEDURE:
        done = proc_llcmsg_initiate_llc_le_ping_procedure(qNode);            //send LL_12_PING_REQ, wait LL_LPING_RSP
        break;
    case LLCMSG_INITIATE_LLC_DATA_LENGTH_UPDATE_PROCEDURE:
        done = proc_llcmsg_initiate_llc_data_length_update_procedure(qNode); //send LL_14_LENGTH_REQ, wait LL_15_LENGTH_RSP
        break;
    }
    /////////////////////////
    if( done )
    {
            kidd_taskENTER_CRITICAL();
                htqueuePop(&(llcmsgQ));
            kidd_taskEXIT_CRITICAL();
        #if    __MALLOC_METHOD__  == 1
            kidd_taskENTER_CRITICAL();
                 free( (void *)qNode );
            kidd_taskEXIT_CRITICAL();
        #elif  __MALLOC_METHOD__  == 2
            vPortFree( (void *)qNode );
        #endif
    }
}

void process_hcmsgQ(void)
{
    proc_hcmsgQ();
    proc_llcmsgQ();
}



/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/







