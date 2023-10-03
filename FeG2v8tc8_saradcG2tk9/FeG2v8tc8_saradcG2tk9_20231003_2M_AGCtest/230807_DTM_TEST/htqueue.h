/*********************************************************************************************************//**
 * @file    htqueue.h
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
#ifndef __HTQUEUE_H
#define __HTQUEUE_H

/* Includes ------------------------------------------------------------------------------------------------*/

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"

/* Exported constants --------------------------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------------------------------------*/
typedef struct htQueue_node_t   // node of singly linked list to implement a queue
{
    struct htQueue_node_t *next;
} htQueue_node_TypeDef;

typedef struct                  //         singly linked list to implement a queue
{
    struct htQueue_node_t *front;
    struct htQueue_node_t *rear;
} htQueue_TypeDef;

typedef struct dllist_node_t    // node of doubly linked list
{
    struct dllist_node_t *next;
    struct dllist_node_t *prev;
} dllist_node_TypeDef;

typedef struct                  //         doubly linked list
{
    struct dllist_node_t *front;
    struct dllist_node_t *rear;
} dllist_TypeDef;


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/
static inline bool htqueueIsEmpty(htQueue_TypeDef *q)
{
    return (q->front == 0);
}
static inline htQueue_node_TypeDef * htqueuePeek(htQueue_TypeDef *q)
{
    return (q->front);
}
static inline htQueue_node_TypeDef * htqueueRear(htQueue_TypeDef *q)
{
    return (q->rear);
}
void htqueueReset(htQueue_TypeDef *);    //reset a queue to its original empty state
void htqueuePush(htQueue_TypeDef *, htQueue_node_TypeDef *);
htQueue_node_TypeDef * htqueuePop(htQueue_TypeDef *);


static inline bool dllistIsEmpty(dllist_TypeDef *q)
{
    return (q->front == 0);
}
static inline dllist_node_TypeDef * dllistPeek(dllist_TypeDef *q)
{
    return (q->front);
}
bool dllistNodeExisting(dllist_TypeDef *, dllist_node_TypeDef *);
void dllistPush        (dllist_TypeDef *, dllist_node_TypeDef *);
void dllistDeleteNode  (dllist_TypeDef *, dllist_node_TypeDef *);
dllist_node_TypeDef * dllistPop(dllist_TypeDef *);

void test_htqueue_api(void);
void test_dllist_api(void);


#endif /* __HTQUEUE_H */
