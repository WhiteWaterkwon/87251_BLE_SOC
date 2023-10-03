/*********************************************************************************************************//**
 * @file    pdma.c
 * @version $Rev:: 100          $
 * @date    $Date:: 2022-02-22 #$
 * @brief   This file provides all PDMA functions.
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
#include <stdint.h>
#include "hwlib/socal/hps.h"
#include "hwlib/socal/socal.h"
#include "hwlib/socal/alt_uart.h"

#include "ble_soc.h"
#include "pdma.h"


/* Private define ------------------------------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------------------------------------*/



/* Global functions ----------------------------------------------------------------------------------------*/

/*********************************************************************************************************//**
 * @brief   
 * @retval  None
 ************************************************************************************************************/
void pdma_ram_bleRx_read_payload(uint16_t ramStartAddr, uint16_t read_length, unsigned char *pch)
{
//RF_WT08(0x100, 0x1);
    while (read_length)
    {
        if ( ((uint32_t)pch) & 0x03 ||
                ramStartAddr & 0x03
           )
        {
            //not word aligned, cannot use RAM_RD32()
            *pch = RAM_RD08(ramStartAddr);
            ramStartAddr ++;
            pch ++;
            read_length --;
        }
        else if ( read_length >> 2 )
        {
          /*
            *((uint32_t *)pch) = RAM_RD32(ramStartAddr);
            ramStartAddr += 4;
            pch += 4;
            read_length -= 4;
          */
            uint16_t len;
            len = (read_length >> 2);
            RAM_RD32_4aligned((uint32_t)pch, ramStartAddr, len);
            len <<= 2;
            ramStartAddr += len;
            pch += len;
            read_length -= len;
          //
        }
        else
        {
            *pch = RAM_RD08(ramStartAddr);
            ramStartAddr ++;
            pch ++;
            read_length --;
        }
    }
//RF_WT08(0x100, 0x1);
//RF_WT08(0x100, 0x1);
}

void pdma_ram_bleTx_write_payload(uint16_t ramStartAddr, uint16_t length, unsigned char *pch)
{
    while (length)
    {
        if ( ((uint32_t)pch) & 0x03 ||
                ramStartAddr & 0x03
           )
        {
            //not word aligned, cannot use RAM_WT32()
            RAM_WT08(ramStartAddr, *pch);
            ramStartAddr ++;
            pch ++;
            length --;
        }
        else if ( length >> 2 )
        {
          /*
            RAM_WT32(ramStartAddr, *((uint32_t *)pch));
            ramStartAddr += 4;
            pch += 4;
            length -= 4;
          */
            uint16_t len;
            len = (length >> 2);
            RAM_WT32_4aligned(ramStartAddr, (uint32_t)pch, len);
            len <<= 2;
            ramStartAddr += len;
            pch += len;
            length -= len;
          //
        }
        else
        {
            RAM_WT08(ramStartAddr, *pch);
            ramStartAddr ++;
            pch ++;
            length --;
        }
    }
}


/**
  * @}
  */
