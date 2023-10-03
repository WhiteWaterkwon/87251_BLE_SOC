/*********************************************************************************************************//**
 * @file    reg87251.h
 * @version $Rev:: 101          $
 * @date    $Date:: 2022-10-10 #$
 * @brief   The header file of the REG87251 functions.
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
#ifndef __REG87251_H
#define __REG87251_H


/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdbool.h>                         //bool
#include <stdint.h>                          //uint8_t


/* Exported constants --------------------------------------------------------------------------------------*/


/* Exported types ------------------------------------------------------------------------------------------*/
typedef union
{
    struct {
        uint8_t ___z    : 1;    //34h [0]-
        uint8_t rxto    : 1;    //34h [1]RX_TO
        uint8_t bcmatch : 1;    //34h [2]BCMATCH
        uint8_t address : 1;    //34h [3]ADDRESS
        uint8_t maxrt   : 1;    //34h [4]MAX_RT
        uint8_t txds    : 1;    //34h [5]TX_DS
        uint8_t rxdr    : 1;    //34h [6]RX_DR
        uint8_t crcf    : 1;    //34h [7]CRCF crc fail
    } field;
    uint8_t reg;

} _34h_IRQ_TypeDef;

typedef union
{
    struct {
        uint8_t bccnt_70;       //0Ch BC_CNT[7:0]
        uint8_t bccnt_f8;       //0Dh BC_CNT[15:8]
    } field;
    uint16_t reg;

} _0Ch_bccnt_TypeDef;

typedef union
{
    struct {
        uint8_t max_rxpl : 7;   //53h [6:0]
        uint8_t rxpl_en  : 1;   //53h [7]
    } field;
    uint8_t reg;

} _53h_max_rxpl_TypeDef;

typedef union
{
    struct {
        uint8_t en_wk_up  : 1;  //E0h [0]  EN_WK_UP    Wake UP      IRQ enable
        uint8_t en_b_set  : 1;  //E0h [1]  EN_B_SET    BLE Setup    IRQ enable
        uint8_t en_anchor : 1;  //E0h [2]  EN_ANCHOR   Anchor Point IRQ enable
        uint8_t en_b_t      : 1;//E0h [3]  EN_B_T      BLE timer enable, and disable by HRESETN reset
        uint8_t en_test_lo  : 1;//E0h [4]  EN_TEST_LO  LO test   enable, and used in engineering mode
        uint8_t _________z  : 2;//E0h [6:5]-
        uint8_t mask_anchor : 1;//E0h [7]  MASK_ANCHOR Anchor Point IRQ mask used in engineering mode
    } field;
    uint8_t reg;

} _E0h_EN_B_TIMER_TypeDef;

typedef union
{
    struct {
        uint8_t wk_up  : 1;     //E1h [0]  IRQ_WK_UP  wake up      IRQ
        uint8_t b_set  : 1;     //E1h [1]  IRQ_B_SET  ble setup    IRQ
        uint8_t anchor : 1;     //E1h [2]  IRQ_ANCHOR anchor point IRQ
        uint8_t _____z : 5;     //E1h [3~7]
    } field;
    uint8_t reg;

} _E1h_IRQ_TypeDef;


/* Exported macro ------------------------------------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------------------------------------*/


#endif /* __REG87251_H */
