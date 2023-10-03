/*********************************************************************************************************//**
 * @file    BCH_5602.h
 * @version $Rev:: 101          $
 * @date    $Date:: 2021-10-10 #$
 * @brief   Calc SYNC by BCH code.
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
#ifndef HT_BCH_5602_H
#define HT_BCH_5602_H

/* Includes ------------------------------------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------------------------------------*/

// <<< Use Configuration Wizard in Context Menu >>>

// <o> BCH(32) Select
//    <0=> BCH(32,26, 4,t1) Barker 3, support 2^(26-(3-1))= 16,777,216 devices
//    <1=> BCH(32,26, 4,t1) Barker 4, support 2^(26-(4-1))=  8,388,608 devices
//    <2=> BCH(32,26, 4,t1) Barker 5, support 2^(26-(5-1))=  4,194,304 devices
//    <3=> BCH(32,26, 4,t1) Barker 7, support 2^(26-(7-1))=  1,048,576 devices
//    <4=> BCH(32,21, 6,t2) Barker 3, support 2^(21-(3-1))=    524,288 devices
//    <5=> BCH(32,21, 6,t2) Barker 4, support 2^(21-(4-1))=    262,144 devices
//    <6=> BCH(32,21, 6,t2) Barker 5, support 2^(21-(5-1))=    131,072 devices
//    <7=> BCH(32,21, 6,t2) Barker 7, support 2^(21-(7-1))=     32,768 devices
//    <8=> BCH(32,16, 8,t3) Barker 3, support 2^(16-(3-1))=     16,384 devices
//    <9=> BCH(32,16, 8,t3) Barker 4, support 2^(16-(4-1))=      8,192 devices
//   <10=> BCH(32,16, 8,t3) Barker 5, support 2^(16-(5-1))=      4,096 devices
//   <11=> BCH(32,16, 8,t3) Barker 7, support 2^(16-(7-1))=      1,024 devices
//   <12=> BCH(32,11,12,t5) Barker 3, support 2^(11-(3-1))=        512 devices
//   <13=> BCH(32,11,12,t5) Barker 4, support 2^(11-(4-1))=        256 devices
//   <14=> BCH(32,11,12,t5) Barker 5, support 2^(11-(5-1))=        128 devices
//   <15=> BCH(32,11,12,t5) Barker 7, support 2^(11-(7-1))=         32 devices
#define BCH_32_SELECT            7

// <o> BCH(24) Select
//    <0=> BCH(24,12, 8,t3) Barker 3, support 2^(12-(3-1))=      1,024 devices
//    <1=> BCH(24,12, 8,t3) Barker 4, support 2^(12-(4-1))=        512 devices
//    <2=> BCH(24,12, 8,t3) Barker 5, support 2^(12-(5-1))=        256 devices
//    <3=> BCH(24,12, 8,t3) Barker 7, support 2^(12-(7-1))=         64 devices
#define BCH_24_SELECT            0      

// <o> BCH(16) Select
//    <0=> BCH(16,11, 4,t1) Barker 3, support 2^(11-(3-1))=        512 devices
//    <1=> BCH(16,11, 4,t1) Barker 4, support 2^(11-(4-1))=        256 devices
//    <2=> BCH(16,11, 4,t1) Barker 5, support 2^(11-(5-1))=        128 devices
//    <3=> BCH(16,11, 4,t1) Barker 7, support 2^(11-(7-1))=         32 devices
#define BCH_16_SELECT            0      

/* Exported macro ------------------------------------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------------------------------------*/
uint32_t calc_syncword_32(uint32_t lap);
     /* #define BCH_32_SELECT  0: lap 0 ~ 00FFFFFF , BCH(32,26,t1) Barker 3, support 16,777,216 devices */
     /* #define BCH_32_SELECT  1: lap 0 ~ 007FFFFF , BCH(32,26,t1) Barker 4, support  8,388,608 devices */
     /* #define BCH_32_SELECT  2: lap 0 ~ 003FFFFF , BCH(32,26,t1) Barker 5, support  4,194,304 devices */
     /* #define BCH_32_SELECT  3: lap 0 ~ 000FFFFF , BCH(32,26,t1) Barker 7, support  1,048,576 devices */
     /* #define BCH_32_SELECT  4: lap 0 ~ 0007FFFF , BCH(32,21,t2) Barker 3, support    524,288 devices */
     /* #define BCH_32_SELECT  5: lap 0 ~ 0003FFFF , BCH(32,21,t2) Barker 4, support    262,144 devices */
     /* #define BCH_32_SELECT  6: lap 0 ~ 0001FFFF , BCH(32,21,t2) Barker 5, support    131,072 devices */
     /* #define BCH_32_SELECT  7: lap 0 ~ 00007FFF , BCH(32,21,t2) Barker 7, support     32,768 devices */
     /* #define BCH_32_SELECT  8: lap 0 ~ 00003FFF , BCH(32,16,t3) Barker 3, support     16,384 devices */
     /* #define BCH_32_SELECT  9: lap 0 ~ 00001FFF , BCH(32,16,t3) Barker 4, support      8,192 devices */
     /* #define BCH_32_SELECT 10: lap 0 ~ 00000FFF , BCH(32,16,t3) Barker 5, support      4,096 devices */
     /* #define BCH_32_SELECT 11: lap 0 ~ 000003FF , BCH(32,16,t3) Barker 7, support      1,024 devices */
     /* #define BCH_32_SELECT 12: lap 0 ~ 000001FF , BCH(32,11,t5) Barker 3, support        512 devices */
     /* #define BCH_32_SELECT 13: lap 0 ~ 000000FF , BCH(32,11,t5) Barker 4, support        256 devices */
     /* #define BCH_32_SELECT 14: lap 0 ~ 0000007F , BCH(32,11,t5) Barker 5, support        128 devices */
     /* #define BCH_32_SELECT 15: lap 0 ~ 0000001F , BCH(32,11,t5) Barker 7, support         32 devices */

uint32_t calc_syncword_24(uint32_t lap);
     /* #define BCH_24_SELECT  0: lap 0 ~ 000003FF , BCH(24,12,t3) Barker  3, support     1,024 devices */
     /* #define BCH_24_SELECT  1: lap 0 ~ 000001FF , BCH(24,12,t3) Barker  4, support       512 devices */
     /* #define BCH_24_SELECT  2: lap 0 ~ 000000FF , BCH(24,12,t3) Barker  5, support       256 devices */
     /* #define BCH_24_SELECT  3: lap 0 ~ 0000003F , BCH(24,12,t3) Barker  7, support        64 devices */

uint32_t calc_syncword_16(uint32_t lap);
     /* #define BCH_16_SELECT  0: lap 0 ~ 000001FF , BCH(16,11,t1) Barker  3, support       512 devices */
     /* #define BCH_16_SELECT  1: lap 0 ~ 000000FF , BCH(16,11,t1) Barker  4, support       256 devices */
     /* #define BCH_16_SELECT  2: lap 0 ~ 0000007F , BCH(16,11,t1) Barker  5, support       128 devices */
     /* #define BCH_16_SELECT  3: lap 0 ~ 0000001F , BCH(16,11,t1) Barker  7, support        32 devices */

/*
 * calc_syncword_XX() are Systematic Encoding: the input data is embedded in the encoded output.
 * syncXX_to_lap()    are not Decoding, but just to truncate syncword and get only the LAP part.
 */
uint32_t sync32_to_lap(uint32_t syncword);
uint32_t sync24_to_lap(uint32_t syncword);
uint32_t sync16_to_lap(uint32_t syncword);


#endif /* HT_BCH_5602_H */
