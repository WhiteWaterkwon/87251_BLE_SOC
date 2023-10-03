/*********************************************************************************************************//**
 * @file    htqueue.c
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

/* Includes ------------------------------------------------------------------------------------------------*/

#include <stdio.h>                           //printf  debug
#include <stdint.h>                          //uint32_t
#include <stdlib.h>                          //malloc, free

#include "bc5602b_host.h"                    //#define  __MALLOC_METHOD__
#include "htqueue.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"

/* Private function prototypes -----------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/


/* Private types -------------------------------------------------------------------------------------------*/


/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
void htqueueReset(htQueue_TypeDef *q)           //reset a queue to its original empty state
{
    htQueue_node_TypeDef *qNode;
    while( (qNode = htqueuePop(q)) != 0 )
    {
        #if    __MALLOC_METHOD__  == 1
             free( qNode );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)qNode );
        #endif
    }
  //q->front = 0;
  //q->rear  = 0;
}

void htqueuePush(htQueue_TypeDef *q, htQueue_node_TypeDef *qNode)
{
    qNode->next = 0;
    if( htqueueIsEmpty(q) )
    {
        q->front = qNode;
    }
    else
    {
        q->rear->next = qNode;
    }
        q->rear = qNode;
      //q->size++;
}

htQueue_node_TypeDef * htqueuePop(htQueue_TypeDef *q)
{
    htQueue_node_TypeDef *qNode;
        qNode = q->front;
    if( qNode != 0 )
    {
            q->front = qNode->next;
          //q->size--;
        if( q->front == 0 ) {
            q->rear = 0;
        }
    }
    return qNode;
}

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
bool dllistNodeExisting(dllist_TypeDef *q, dllist_node_TypeDef *pNode)
{
    dllist_node_TypeDef * tmp;
        tmp = q->front ;
    while( tmp != 0 )
    {
        if( tmp == pNode )  return 1 ;
        tmp = tmp->next ;
    }
    
    return 0;
}

void dllistPush(dllist_TypeDef *q, dllist_node_TypeDef *pNode)
{
        pNode->next = 0;
    if( dllistIsEmpty(q) )
    {
        pNode->prev = 0;
        q->front = pNode;
        q->rear  = pNode;
    }
    else
    {
        pNode->prev = q->rear;
        q->rear->next = pNode;
        q->rear       = pNode;
    }
      //q->size++;
}

dllist_node_TypeDef * dllistPop(dllist_TypeDef *q)
{
    dllist_node_TypeDef *pNode;
    
        pNode = q->front;
    if( pNode != 0 )
    {
            q->front = q->front->next;
        if( q->front != 0 ) {
            q->front->prev = 0;
        }
          //q->size--;
        if( q->front == 0 ) {
            q->rear = 0;
        }
    }
    return pNode;
}

void dllistDeleteNode(dllist_TypeDef *q, dllist_node_TypeDef *pNode)
{
    if( 0 == dllistNodeExisting(q,pNode) ) {
        return;
    }
    if( pNode->prev != 0 )
        pNode->prev->next = pNode->next ;
    if( pNode->next != 0 )
        pNode->next->prev = pNode->prev ;

    if( q->front == pNode ) {q->front = pNode->next ;}
    if( q->rear  == pNode ) {q->rear  = pNode->prev ;}
      //q->size++;
}


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
#if 0
#include "llc.h"          //llcTxQ_node_TypeDef
#include "usart.h"        // uart_puts, uart_putu8
    htQueue_TypeDef sllQ; //singly linked list
void test_htqueue_api(void)
{
    uint8_t i;
    size_t remaining;
    llcTxQ_node_TypeDef *qNode;
    htQueue_TypeDef *q;
    q = &sllQ;

        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');

    for( i=0; i<3; i++)
    {    
        #if    __MALLOC_METHOD__  == 1
        qNode = (llcTxQ_node_TypeDef *)      malloc(sizeof(llcTxQ_node_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        qNode = (llcTxQ_node_TypeDef *)pvPortMalloc(sizeof(llcTxQ_node_TypeDef));
        #else
        ...
        #endif
        htqueuePush( q, (void *)qNode );
        uart_puts("i="); uart_putu8(i);
        uart_puts(" qNode=");     uart_putu32((uint32_t)qNode);
        {
            htQueue_node_TypeDef * tmp;
            tmp = q->front ;
          while( tmp != 0 )
          {
            uart_puts(" temp=");      uart_putu32((uint32_t)tmp);
            uart_puts(" next=");      uart_putu32((uint32_t)tmp->next); uart_puts(",   ");
            tmp = tmp->next ;
          }
        }
        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');
    }
    uart_putchar_n('\n');
    
    for( i=0; i<3; i++)
    {    
      //  htqueueReset( q );
        qNode = (llcTxQ_node_TypeDef *)htqueuePop(q);
        #if    __MALLOC_METHOD__  == 1
             free( (void *)qNode );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)qNode );
        #endif
        uart_puts("i="); uart_putu8(i);
        uart_puts(" qNode=");     uart_putu32((uint32_t)qNode);
        {
            htQueue_node_TypeDef * tmp;
            tmp = q->front ;
          while( tmp != 0 )
          {
            uart_puts(" temp=");      uart_putu32((uint32_t)tmp);
            uart_puts(" next=");      uart_putu32((uint32_t)tmp->next); uart_puts(",   ");
            tmp = tmp->next ;
          }
        }
        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');
    }
}
#endif

/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/
#if 0
#include "mac.h"  //LEACL_TypeDef
#include "usart.h"                           // uart_puts, uart_putu8
    dllist_TypeDef dllQ; //doubly linked list
void test_dllist_api(void)
{
    uint8_t i;
    size_t remaining;
    LEACL_TypeDef *pacl;
    dllist_TypeDef *q;
    q = &dllQ;
    
        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');

    for( i=0; i<3; i++)
    {    
        #if    __MALLOC_METHOD__  == 1
        pacl = (LEACL_TypeDef *)      malloc(sizeof(LEACL_TypeDef));
        #elif  __MALLOC_METHOD__  == 2
        pacl = (LEACL_TypeDef *)pvPortMalloc(sizeof(LEACL_TypeDef));
        #else
        ...
        #endif
        dllistPush( q, (void *)pacl );
        uart_puts("i="); uart_putu8(i);
        uart_puts(" pacl=");     uart_putu32((uint32_t)pacl);
        {
            dllist_node_TypeDef * tmp;
            tmp = q->front ;
          while( tmp != 0 )
          {
            uart_puts(" temp=");      uart_putu32((uint32_t)tmp);
            uart_puts(" prev=");      uart_putu32((uint32_t)tmp->prev);
            uart_puts(" next=");      uart_putu32((uint32_t)tmp->next); uart_puts(",   ");
            tmp = tmp->next ;
          }
        }
        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');
    }
    uart_putchar_n('\n');
    for( i=0; i<3; i++)
    {    
      //  htqueueReset( q );
        pacl = (LEACL_TypeDef *)dllistPop(q);
        #if    __MALLOC_METHOD__  == 1
             free( (void *)pacl );
        #elif  __MALLOC_METHOD__  == 2
        vPortFree( (void *)pacl );
        #endif
        uart_puts("i="); uart_putu8(i);
        uart_puts(" pacl=");     uart_putu32((uint32_t)pacl);
        {
            dllist_node_TypeDef * tmp;
            tmp = q->front ;
          while( tmp != 0 )
          {
            uart_puts(" temp=");      uart_putu32((uint32_t)tmp);
            uart_puts(" prev=");      uart_putu32((uint32_t)tmp->prev);
            uart_puts(" next=");      uart_putu32((uint32_t)tmp->next); uart_puts(",   ");
            tmp = tmp->next ;
          }
        }
        uart_puts(" front=");     uart_putu32((uint32_t)q->front);
        uart_puts(" rear=");      uart_putu32((uint32_t)q->rear);
        remaining = xPortGetFreeHeapSize();
        uart_puts(" Remain=");    uart_putu32(remaining); uart_putchar_n('\n');
    }
}
#endif


/*********************************************************************************************************//**
  *
  ***********************************************************************************************************/


