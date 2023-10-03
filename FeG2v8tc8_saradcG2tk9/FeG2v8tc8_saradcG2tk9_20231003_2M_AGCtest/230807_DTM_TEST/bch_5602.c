/*********************************************************************************************************//**
 * @file    BCH_5602.c
 * @version $Rev:: 101          $
 * @date    $Date:: 2017-01-16 #$
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
 * <h2><center>Copyright (C) 2014 Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>                        //bool
#include <string.h>                         //memcpy

#include "hwlib/socal/socal.h"
#include "bch_5602.h"



/* Private variables ---------------------------------------------------------------------------------------*/

/* Global functions ----------------------------------------------------------------------------------------*/

////////////////////////////////////////
#define BCH_32_N            32
#if   BCH_32_SELECT == 0
#define BCH_32_K            26
#define BCH_32_BARKER_LEN       3
#elif BCH_32_SELECT == 1
#define BCH_32_K            26
#define BCH_32_BARKER_LEN       4
#elif BCH_32_SELECT == 2
#define BCH_32_K            26
#define BCH_32_BARKER_LEN       5
#elif BCH_32_SELECT == 3
#define BCH_32_K            26
#define BCH_32_BARKER_LEN       7
#elif BCH_32_SELECT == 4
#define BCH_32_K            21
#define BCH_32_BARKER_LEN       3
#elif BCH_32_SELECT == 5
#define BCH_32_K            21
#define BCH_32_BARKER_LEN       4
#elif BCH_32_SELECT == 6
#define BCH_32_K            21
#define BCH_32_BARKER_LEN       5
#elif BCH_32_SELECT == 7
#define BCH_32_K            21
#define BCH_32_BARKER_LEN       7
#elif BCH_32_SELECT == 8
#define BCH_32_K            16
#define BCH_32_BARKER_LEN       3
#elif BCH_32_SELECT == 9
#define BCH_32_K            16
#define BCH_32_BARKER_LEN       4
#elif BCH_32_SELECT == 10
#define BCH_32_K            16
#define BCH_32_BARKER_LEN       5
#elif BCH_32_SELECT == 11
#define BCH_32_K            16
#define BCH_32_BARKER_LEN       7
#elif BCH_32_SELECT == 12
#define BCH_32_K            11
#define BCH_32_BARKER_LEN       3
#elif BCH_32_SELECT == 13
#define BCH_32_K            11
#define BCH_32_BARKER_LEN       4
#elif BCH_32_SELECT == 14
#define BCH_32_K            11
#define BCH_32_BARKER_LEN       5
#elif BCH_32_SELECT == 15
#define BCH_32_K            11
#define BCH_32_BARKER_LEN       7
#endif
#define LAP_32_K                    (BCH_32_K - BCH_32_BARKER_LEN + 1)
////////////////////////////////////////
#define BCH_24_N            24
#if   BCH_24_SELECT == 0
#define BCH_24_K            12
#define BCH_24_BARKER_LEN        3
#elif BCH_24_SELECT == 1
#define BCH_24_K            12
#define BCH_24_BARKER_LEN        4
#elif BCH_24_SELECT == 2
#define BCH_24_K            12
#define BCH_24_BARKER_LEN        5
#elif BCH_24_SELECT == 3
#define BCH_24_K            12
#define BCH_24_BARKER_LEN        7
#endif
#define LAP_24_K                    (BCH_24_K - BCH_24_BARKER_LEN + 1)
////////////////////////////////////////
#define BCH_16_N            16
#if   BCH_16_SELECT == 0
#define BCH_16_K            11
#define BCH_16_BARKER_LEN        3
#elif BCH_16_SELECT == 1
#define BCH_16_K            11
#define BCH_16_BARKER_LEN        4
#elif BCH_16_SELECT == 2
#define BCH_16_K            11
#define BCH_16_BARKER_LEN        5
#elif BCH_16_SELECT == 3
#define BCH_16_K            11
#define BCH_16_BARKER_LEN        7
#endif
#define LAP_16_K                    (BCH_16_K - BCH_16_BARKER_LEN + 1)
////////////////////////////////////////

static unsigned char g[8];
static unsigned char p[8];

#if   BCH_16_K == 7
static const unsigned char g_16_7[2]  = { 0xC0, 0x9C };
#elif BCH_16_K == 11
static const unsigned char g_16_11[2] = { 0x00, 0xB4 };
#endif

#if   BCH_24_K == 12
static const unsigned char g_24_12[3] = { 0x00, 0x28, 0xF9 };
#endif

#if   BCH_32_K == 11
static const unsigned char g_32_11[4] = { 0x00, 0xFC, 0xAD, 0xE9 };
#elif BCH_32_K == 16
static const unsigned char g_32_16[4] = { 0x00, 0x80, 0x78, 0xC8 };
#elif BCH_32_K == 21
static const unsigned char g_32_21[4] = { 0x00, 0x00, 0xB0, 0x9B };
#elif BCH_32_K == 26
static const unsigned char g_32_26[4] = { 0x00, 0x00, 0x00, 0xDE };
#endif


static const unsigned char p_16[2] = { 0x34, 0xF1 };
static const unsigned char p_24[3] = { 0xB8, 0x58, 0x8F };
static const unsigned char p_32[4] = { 0xAC, 0xB8, 0x4F, 0x86 };

#define BARKER_3_PAD_1                        ((uint32_t)0x00000000)     //           --|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_3_PAD_0                        ((uint32_t)0x00000003)     //           ++|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#if 1
#define BARKER_4_PAD_1                        ((uint32_t)0x00000000)     //          ---|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_4_PAD_0                        ((uint32_t)0x00000007)     //          +++|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#else
#define BARKER_4_PAD_1                        ((uint32_t)0x00000006)     //          ++-|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_4_PAD_0                        ((uint32_t)0x00000001)     //          --+|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#endif
#define BARKER_5_PAD_1                        ((uint32_t)0x0000000E)     //         +++-|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_5_PAD_0                        ((uint32_t)0x00000001)     //         ---+|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_7_PAD_1                        ((uint32_t)0x00000006)     //       ---++-|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_7_PAD_0                        ((uint32_t)0x00000039)     //       +++--+|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_11_PAD_1                       ((uint32_t)0x00000076)     //   ---+++-++-|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_11_PAD_0                       ((uint32_t)0x00000389)     //   +++---+--+|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_13_PAD_1                       ((uint32_t)0x00000F9A)     // +++++--++-+-|+     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11
#define BARKER_13_PAD_0                       ((uint32_t)0x00000065)     // -----++--+-+|-     2021may19 MSB 2 bits of syncword must be 2'b00 or 2'b11


static void array_shift_less(unsigned char bytes, unsigned char *a)
{
    unsigned char i;
    for (i = 0; i<(bytes - 1); i++) {
        a[i] >>= 1;
        if (a[i + 1] & 0x01) { a[i] |= 0x80; }
    }
    a[bytes - 1] >>= 1;
}
static void array_shift_larger(unsigned char bytes, unsigned char *a, unsigned char times)
{
    unsigned char i, tt;
    for (tt = 0; tt < times; tt++) {
    for (i = (bytes - 1); i>0; i--) {
        a[i] <<= 1;
        if (a[i - 1] & 0x80) { a[i] |= 0x01; }
    }
    a[0] <<= 1;
    }
}

/**
 *  @ingroup BC5602_BCH_label
 */
uint32_t bch32_syncword_calc(uint32_t lap)
{
    unsigned char iteration_mod;
    unsigned char aa[4];
    unsigned char gg[4];
    unsigned char tmp[4];
    uint32_t addrbyte4321;

    #if   BCH_32_BARKER_LEN == 3
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_3_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_3_PAD_0 << LAP_32_K); }
    #elif BCH_32_BARKER_LEN == 4
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_4_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_4_PAD_0 << LAP_32_K); }
    #elif BCH_32_BARKER_LEN == 5
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_5_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_5_PAD_0 << LAP_32_K); }
    #elif BCH_32_BARKER_LEN == 7
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_7_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_7_PAD_0 << LAP_32_K); }
    #elif BCH_32_BARKER_LEN == 11
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_11_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_11_PAD_0 << LAP_32_K); }
    #elif BCH_32_BARKER_LEN == 13
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_32_K-1))) { lap |= (BARKER_13_PAD_1 << LAP_32_K); }
    else                                { lap |= (BARKER_13_PAD_0 << LAP_32_K); }
    #endif

    aa[0] = lap;
    aa[1] = lap >> 8;
    aa[2] = lap >> 16;
    aa[3] = lap >> 24;
    //
    array_shift_larger(4, aa, BCH_32_N - BCH_32_K);
    
#if   BCH_32_K == 11
    memcpy(g, (uint8_t *)g_32_11, 4);
#elif BCH_32_K == 16
    memcpy(g, (uint8_t *)g_32_16, 4);
#elif BCH_32_K == 21
	memcpy(g, (uint8_t *)g_32_21, 4);
#elif BCH_32_K == 26
	memcpy(g, (uint8_t *)g_32_26, 4);
#endif
    memcpy(gg, g, 4);
    memcpy(p, (uint8_t *)p_32, 4);
    //
#if   BCH_32_K == 11
    tmp[0] = 0x00;
    tmp[1] = 0x00;
    tmp[2] =(aa[2] ^ p[2])&0xE0;
    tmp[3] = aa[3] ^ p[3];
#elif BCH_32_K == 16
    tmp[0] = 0x00;
    tmp[1] = 0x00;
    tmp[2] = aa[2] ^ p[2];
    tmp[3] = aa[3] ^ p[3];
#elif BCH_32_K == 21
    tmp[0] = 0x00;
	tmp[1] =(aa[1] ^ p[1])&0xF8;
	tmp[2] = aa[2] ^ p[2];
    tmp[3] = aa[3] ^ p[3];
#elif BCH_32_K == 26
	tmp[0] =(aa[0] ^ p[0])&0xC0;
	tmp[1] = aa[1] ^ p[1];
	tmp[2] = aa[2] ^ p[2];
    tmp[3] = aa[3] ^ p[3];
#endif
    //
    for(iteration_mod=0;iteration_mod<BCH_32_K;iteration_mod++)
    {
        unsigned char ii,mask;
        ii=iteration_mod>>3;
        #if 1
        mask=0x80; mask >>= (iteration_mod&0x07);
        #else
        switch (iteration_mod & 0x07) {
        case 0x00: mask = 0x80; break;
        case 0x01: mask = 0x40; break;
        case 0x02: mask = 0x20; break;
        case 0x03: mask = 0x10; break;
        case 0x04: mask = 0x08; break;
        case 0x05: mask = 0x04; break;
        case 0x06: mask = 0x02; break;
        case 0x07: mask = 0x01; break;
        }
        #endif
        if(tmp[3-ii]&mask) {
            tmp[0] ^= gg[0];
            tmp[1] ^= gg[1];
            tmp[2] ^= gg[2];
            tmp[3] ^= gg[3];
        }
        array_shift_less(4, gg);
    }
    //
#if   BCH_32_K == 11
    tmp[0] ^= p[0];
    tmp[1] ^= p[1];
    tmp[2] = ((tmp[2] ^ p[2]) & 0x1F) | (aa[2] & 0xE0);
    tmp[3] = aa[3];
#elif BCH_32_K == 16
    tmp[0] ^= p[0];
    tmp[1] ^= p[1];
    tmp[2] = aa[2];
    tmp[3] = aa[3];
#elif BCH_32_K == 21
    tmp[0] ^= p[0];
	tmp[1] = ((tmp[1] ^ p[1]) & 0x07) | (aa[1] & 0xF8);
	tmp[2] = aa[2];
    tmp[3] = aa[3];
#elif BCH_32_K == 26
	tmp[0] = ((tmp[0] ^ p[0]) & 0x3F) | (aa[0] & 0xC0);
	tmp[1] = aa[1];
	tmp[2] = aa[2];
    tmp[3] = aa[3];
#endif
    //
//  pIdcode[0]=tmp[3];
//  pIdcode[1]=tmp[2];
//  pIdcode[2]=tmp[1];
//  pIdcode[3]=tmp[0];
    addrbyte4321 = tmp[3];
    addrbyte4321 = tmp[2]+(addrbyte4321<<8);
    addrbyte4321 = tmp[1]+(addrbyte4321<<8);
    addrbyte4321 = tmp[0]+(addrbyte4321<<8);
    return addrbyte4321;
}
/**
 *  @ingroup BC5602_BCH_label
 */
uint32_t bch24_syncword_calc(uint32_t lap)
{
    unsigned char iteration_mod;
    unsigned char aa[3];
    unsigned char gg[3];
    unsigned char tmp[3];
    uint32_t addrbyte4321;

    #if   BCH_24_BARKER_LEN == 3
    lap &= ((((uint32_t)1)<<LAP_24_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_24_K-1))) { lap |= (BARKER_3_PAD_1 << LAP_24_K); }
    else                                { lap |= (BARKER_3_PAD_0 << LAP_24_K); }
    #elif BCH_24_BARKER_LEN == 4
    lap &= ((((uint32_t)1)<<LAP_24_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_24_K-1))) { lap |= (BARKER_4_PAD_1 << LAP_24_K); }
    else                                { lap |= (BARKER_4_PAD_0 << LAP_24_K); }
    #elif BCH_24_BARKER_LEN == 5
    lap &= ((((uint32_t)1)<<LAP_24_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_24_K-1))) { lap |= (BARKER_5_PAD_1 << LAP_24_K); }
    else                                { lap |= (BARKER_5_PAD_0 << LAP_24_K); }
    #elif BCH_24_BARKER_LEN == 7
    lap &= ((((uint32_t)1)<<LAP_24_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_24_K-1))) { lap |= (BARKER_7_PAD_1 << LAP_24_K); }
    else                                { lap |= (BARKER_7_PAD_0 << LAP_24_K); }
    #endif

    aa[0] = lap;
    aa[1] = lap >> 8;
    aa[2] = lap >> 16;
    //
    array_shift_larger(3, aa, BCH_24_N - BCH_24_K);
    
    #if   BCH_24_K == 12
    memcpy(g, (uint8_t *)g_24_12, 3);
    #endif
    memcpy(gg, g, 3);
    memcpy(p, (uint8_t *)p_24, 3);
    //
    #if   BCH_24_K == 12
    tmp[0] = 0x00;
    tmp[1] =(aa[1] ^ p[1]) & 0xF0;
    tmp[2] = aa[2] ^ p[2];
    #endif
    for (iteration_mod = 0; iteration_mod < BCH_24_K; iteration_mod++)
    {
        unsigned char ii, mask;
        ii = iteration_mod >> 3;
        #if 1
        mask=0x80; mask >>= (iteration_mod&0x07);
        #else
        switch (iteration_mod & 0x07) {
        case 0x00: mask = 0x80; break;
        case 0x01: mask = 0x40; break;
        case 0x02: mask = 0x20; break;
        case 0x03: mask = 0x10; break;
        case 0x04: mask = 0x08; break;
        case 0x05: mask = 0x04; break;
        case 0x06: mask = 0x02; break;
        case 0x07: mask = 0x01; break;
        }
        #endif
        if (tmp[2 - ii] & mask) {
            tmp[0] ^= gg[0];
            tmp[1] ^= gg[1];
            tmp[2] ^= gg[2];
        }
        array_shift_less(3, gg);
    }
    //
    #if   BCH_24_K == 12
    tmp[0] ^= p[0];
    tmp[1] = ((tmp[1] ^ p[1]) & 0x0F) | (aa[1] & 0xF0);
    tmp[2] = aa[2];
    #endif
    //
//  pIdcode[0]=tmp[2];
//  pIdcode[1]=tmp[1];
//  pIdcode[2]=tmp[0];
    addrbyte4321 = tmp[2];
    addrbyte4321 = tmp[1]+(addrbyte4321<<8);
    addrbyte4321 = tmp[0]+(addrbyte4321<<8);
    return addrbyte4321;
}
/**
 *  @ingroup BC5602_BCH_label
 */
uint32_t bch16_syncword_calc(uint32_t lap)
{
    unsigned char iteration_mod;
    unsigned char aa[2];
    unsigned char gg[2];
    unsigned char tmp[2];
    uint32_t addrbyte4321;

    #if   BCH_16_BARKER_LEN == 3
    lap &= ((((uint32_t)1)<<LAP_16_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_16_K-1))) { lap |= (BARKER_3_PAD_1 << LAP_16_K); }
    else                                { lap |= (BARKER_3_PAD_0 << LAP_16_K); }
    #elif BCH_16_BARKER_LEN == 4
    lap &= ((((uint32_t)1)<<LAP_16_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_16_K-1))) { lap |= (BARKER_4_PAD_1 << LAP_16_K); }
    else                                { lap |= (BARKER_4_PAD_0 << LAP_16_K); }
    #elif BCH_16_BARKER_LEN == 5
    lap &= ((((uint32_t)1)<<LAP_16_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_16_K-1))) { lap |= (BARKER_5_PAD_1 << LAP_16_K); }
    else                                { lap |= (BARKER_5_PAD_0 << LAP_16_K); }
    #elif BCH_16_BARKER_LEN == 7
    lap &= ((((uint32_t)1)<<LAP_16_K)-1);
    if (lap & (((uint32_t)1)<<(LAP_16_K-1))) { lap |= (BARKER_7_PAD_1 << LAP_16_K); }
    else                                { lap |= (BARKER_7_PAD_0 << LAP_16_K); }
    #endif

    aa[0] = lap;
    aa[1] = lap >> 8;
    //
    array_shift_larger(2, aa, BCH_16_N - BCH_16_K);
    
    #if   BCH_16_K == 7
    memcpy(g, (uint8_t *)g_16_7, 2);
    #elif BCH_16_K == 11
    memcpy(g, (uint8_t *)g_16_11, 2);
    #endif
    memcpy(gg, g, 2);
    memcpy(p, (uint8_t *)p_16, 2);
    //
    #if   BCH_16_K == 7
    tmp[0] = 0x00;
    tmp[1] =(aa[1] ^ p[1]) & 0xFE;
    #elif BCH_16_K == 11
    tmp[0] =(aa[0] ^ p[0]) & 0xE0;
    tmp[1] = aa[1] ^ p[1];
    #endif
    for (iteration_mod = 0; iteration_mod < BCH_16_K; iteration_mod++)
    {
        unsigned char ii, mask;
        ii = iteration_mod >> 3;
        #if 1
        mask=0x80; mask >>= (iteration_mod&0x07);
        #else
        switch (iteration_mod & 0x07) {
        case 0x00: mask = 0x80; break;
        case 0x01: mask = 0x40; break;
        case 0x02: mask = 0x20; break;
        case 0x03: mask = 0x10; break;
        case 0x04: mask = 0x08; break;
        case 0x05: mask = 0x04; break;
        case 0x06: mask = 0x02; break;
        case 0x07: mask = 0x01; break;
        }
        #endif
        if (tmp[1 - ii] & mask) {
            tmp[0] ^= gg[0];
            tmp[1] ^= gg[1];
        }
        array_shift_less(2, gg);
    }
    //
    #if   BCH_16_K == 7
    tmp[0] =   tmp[0] ^ p[0];
    tmp[1] = ((tmp[1] ^ p[1]) & 0x01) | (aa[1] & 0xFE);
    #elif BCH_16_K == 11
    tmp[0] = ((tmp[0] ^ p[0]) & 0x1F) | (aa[0] & 0xE0);
    tmp[1] =   aa[1];
    #endif
    //
//  pIdcode[0]=tmp[1];
//  pIdcode[1]=tmp[0];
    addrbyte4321 = tmp[1];
    addrbyte4321 = tmp[0]+(addrbyte4321<<8);
    return addrbyte4321;
}

uint32_t calc_syncword_16(uint32_t lap)
{
    return bch16_syncword_calc(lap);
}
uint32_t calc_syncword_24(uint32_t lap)
{
    return bch24_syncword_calc(lap);
}
uint32_t calc_syncword_32(uint32_t lap)
{
    return bch32_syncword_calc(lap);
}

/*
 * calc_syncword_XX() are Systematic Encoding: the input data is embedded in the encoded output.
 * syncXX_to_lap()    are not Decoding, but just to truncate syncword and get only the LAP part.
 */
uint32_t sync16_to_lap(uint32_t syncword)
{
    volatile uint32_t lap;
    lap = syncword;
    lap >>= (BCH_16_N - BCH_16_K);
    lap &= ((((uint32_t)1)<<LAP_16_K)-1);
    return lap;
}
uint32_t sync24_to_lap(uint32_t syncword)
{
    volatile uint32_t lap;
    lap = syncword;
    lap >>= (BCH_24_N - BCH_24_K);
    lap &= ((((uint32_t)1)<<LAP_24_K)-1);
    return lap;
}
uint32_t sync32_to_lap(uint32_t syncword)
{
    volatile uint32_t lap;
    lap = syncword;
    lap >>= (BCH_32_N - BCH_32_K);
    lap &= ((((uint32_t)1)<<LAP_32_K)-1);
    return lap;
}


