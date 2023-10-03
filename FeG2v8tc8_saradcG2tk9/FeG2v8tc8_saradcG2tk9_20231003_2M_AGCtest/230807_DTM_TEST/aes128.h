/*********************************************************************************************************//**
 * @file    aes128.h
 * @version $Rev:: 929          $
 * @date    $Date:: 2019-12-02 #$
 * @brief   The header file of the P-256 command functions.
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
#ifndef __AES128_CMD_H
#define __AES128_CMD_H

/* Includes ------------------------------------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------------------------------------*/


/* Exported macro ------------------------------------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------------------------------------*/
void aes128_Encrypt_use_32f87251_engine(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[]);
void aes128_Decrypt_use_32f87251_engine(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[]);

void aes128_encrypt                   (uint8_t state_mso_to_lso[], uint8_t key_mso_to_lso[]);
void aes128_decrypt                   (uint8_t state_mso_to_lso[], uint8_t key_mso_to_lso[]);
void aes128_encrypt___lsbyte_to_msbyte(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[]);
void aes128_decrypt___lsbyte_to_msbyte(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[]);

void test_aes128(void);


#endif /* __AES128_CMD_H ------------------------------------------------------------------------------------*/
