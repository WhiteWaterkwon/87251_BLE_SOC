/*********************************************************************************************************//**
 * @file    aes128.c
 * @version $Rev:: 929          $
 * @date    $Date:: 2019-12-02 #$
 * @brief   This file provides all P-256 command functions.
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

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>                          //malloc, free
#include <string.h>                          //memcpy,memset   ==>   ht_memory_copy    ht_memory_set
#include <stdbool.h>                         //bool

#include "hwlib/socal/socal.h"
#include "bc5602b_host.h"                    // delay_unit625us
#include "ble_soc.h"                         // RAM_WT32
#include "usart.h"                           // uart_puts, uart_putu8
#include "pdma.h"                            // PDMA_CH4_DT
#include "aes128.h"

#define _CODE_TEST_AESAVS_PATTERN_

/* Private define ------------------------------------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------------------------------------*/
volatile bool aes128_irq_occur = 0;

/* Private variables ---------------------------------------------------------------------------------------*/

/* Global functions ----------------------------------------------------------------------------------------*/
void debug_print_16_bytes_inverse(uint8_t a[])
{
    uint8_t i;
    for (i=0; i<16; i++) {
    	uart_putu8(a[15-i]);
    }
    uart_putchar_n('\n');
}

//====================================================================================
// foreward sbox
const uint8_t sbox[256] = {
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76, //0
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0, //1
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15, //2
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75, //3
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84, //4
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf, //5
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8, //6
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2, //7
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73, //8
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb, //9
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79, //A
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08, //B
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a, //C
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e, //D
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf, //E
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16  //F
};
// inverse sbox
const uint8_t rsbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};
// round constant
const uint8_t Rcon[11] = {
    0x8d,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};


// expand the key
void expandKey(uint8_t expandedKey[], uint8_t key[])
{
    unsigned short ii, buf1;
    for (ii = 0; ii<16; ii++)
        expandedKey[ii] = key[ii];
    for (ii = 1; ii<11; ii++){
        buf1 = expandedKey[ii * 16 - 4];
        expandedKey[ii * 16 + 0] = sbox[expandedKey[ii * 16 - 3]] ^ expandedKey[(ii - 1) * 16 + 0] ^ Rcon[ii];
        expandedKey[ii * 16 + 1] = sbox[expandedKey[ii * 16 - 2]] ^ expandedKey[(ii - 1) * 16 + 1];
        expandedKey[ii * 16 + 2] = sbox[expandedKey[ii * 16 - 1]] ^ expandedKey[(ii - 1) * 16 + 2];
        expandedKey[ii * 16 + 3] = sbox[buf1] ^ expandedKey[(ii - 1) * 16 + 3];
        expandedKey[ii * 16 + 4] = expandedKey[(ii - 1) * 16 + 4] ^ expandedKey[ii * 16 + 0];
        expandedKey[ii * 16 + 5] = expandedKey[(ii - 1) * 16 + 5] ^ expandedKey[ii * 16 + 1];
        expandedKey[ii * 16 + 6] = expandedKey[(ii - 1) * 16 + 6] ^ expandedKey[ii * 16 + 2];
        expandedKey[ii * 16 + 7] = expandedKey[(ii - 1) * 16 + 7] ^ expandedKey[ii * 16 + 3];
        expandedKey[ii * 16 + 8] = expandedKey[(ii - 1) * 16 + 8] ^ expandedKey[ii * 16 + 4];
        expandedKey[ii * 16 + 9] = expandedKey[(ii - 1) * 16 + 9] ^ expandedKey[ii * 16 + 5];
        expandedKey[ii * 16 + 10] = expandedKey[(ii - 1) * 16 + 10] ^ expandedKey[ii * 16 + 6];
        expandedKey[ii * 16 + 11] = expandedKey[(ii - 1) * 16 + 11] ^ expandedKey[ii * 16 + 7];
        expandedKey[ii * 16 + 12] = expandedKey[(ii - 1) * 16 + 12] ^ expandedKey[ii * 16 + 8];
        expandedKey[ii * 16 + 13] = expandedKey[(ii - 1) * 16 + 13] ^ expandedKey[ii * 16 + 9];
        expandedKey[ii * 16 + 14] = expandedKey[(ii - 1) * 16 + 14] ^ expandedKey[ii * 16 + 10];
        expandedKey[ii * 16 + 15] = expandedKey[(ii - 1) * 16 + 15] ^ expandedKey[ii * 16 + 11];
    }
}

// multiply by 2 in the galois field GF(2^8) with the irreducible polynomial is m(x)= x^8 + x^4 + x^3 + x + 1
uint8_t galois_mul2(uint8_t value)
{
    if (value >> 7)
    {
        value = value << 1;
        return (value ^ 0x1b);                 // the irreducible polynomial is m(x)= x^8 + x^4 + x^3 + x + 1
    }
    else
        return value << 1;
}

// straight foreward aes encryption implementation
//   first the group of operations
//     - addroundkey
//     - subbytes
//     - shiftrows
//     - mixcolums
//   is executed 9 times, after this addroundkey to finish the 9th round, 
//   after that the 10th round without mixcolums
//   no further subfunctions to save cycles for function calls
//   no structuring with "for (....)" to save cycles
void aes_encr(uint8_t state[], uint8_t expandedKey[])
{
    uint8_t buf1, buf2, buf3, round;

    for (round = 0; round < 9; round++) {
        // addroundkey, sbox and shiftrows
        // row 0
        state[0]  = sbox[(state[0]  ^ expandedKey[(round * 16)])];
        state[4]  = sbox[(state[4]  ^ expandedKey[(round * 16) + 4])];
        state[8]  = sbox[(state[8]  ^ expandedKey[(round * 16) + 8])];
        state[12] = sbox[(state[12] ^ expandedKey[(round * 16) + 12])];
        // row 1
        buf1 = state[1] ^ expandedKey[(round * 16) + 1];
        state[1]  = sbox[(state[5]  ^ expandedKey[(round * 16) + 5])];
        state[5]  = sbox[(state[9]  ^ expandedKey[(round * 16) + 9])];
        state[9]  = sbox[(state[13] ^ expandedKey[(round * 16) + 13])];
        state[13] = sbox[buf1];
        // row 2
        buf1 = state[2] ^ expandedKey[(round * 16) + 2];
        buf2 = state[6] ^ expandedKey[(round * 16) + 6];
        state[2]  = sbox[(state[10] ^ expandedKey[(round * 16) + 10])];
        state[6]  = sbox[(state[14] ^ expandedKey[(round * 16) + 14])];
        state[10] = sbox[buf1];
        state[14] = sbox[buf2];
        // row 3
        buf1 = state[15] ^ expandedKey[(round * 16) + 15];
        state[15] = sbox[(state[11] ^ expandedKey[(round * 16) + 11])];
        state[11] = sbox[(state[7]  ^ expandedKey[(round * 16) + 7])];
        state[7]  = sbox[(state[3]  ^ expandedKey[(round * 16) + 3])];
        state[3]  = sbox[buf1];

        // mixcolums //////////
        // col1
        buf1 = state[0] ^ state[1] ^ state[2] ^ state[3];
        buf2 = state[0];
        buf3 = state[0] ^ state[1]; buf3 = galois_mul2(buf3); state[0] = state[0] ^ buf3 ^ buf1;
        buf3 = state[1] ^ state[2]; buf3 = galois_mul2(buf3); state[1] = state[1] ^ buf3 ^ buf1;
        buf3 = state[2] ^ state[3]; buf3 = galois_mul2(buf3); state[2] = state[2] ^ buf3 ^ buf1;
        buf3 = state[3] ^ buf2;     buf3 = galois_mul2(buf3); state[3] = state[3] ^ buf3 ^ buf1;
        // col2
        buf1 = state[4] ^ state[5] ^ state[6] ^ state[7];
        buf2 = state[4];
        buf3 = state[4] ^ state[5]; buf3 = galois_mul2(buf3); state[4] = state[4] ^ buf3 ^ buf1;
        buf3 = state[5] ^ state[6]; buf3 = galois_mul2(buf3); state[5] = state[5] ^ buf3 ^ buf1;
        buf3 = state[6] ^ state[7]; buf3 = galois_mul2(buf3); state[6] = state[6] ^ buf3 ^ buf1;
        buf3 = state[7] ^ buf2;     buf3 = galois_mul2(buf3); state[7] = state[7] ^ buf3 ^ buf1;
        // col3
        buf1 = state[8] ^ state[9] ^ state[10] ^ state[11];
        buf2 = state[8];
        buf3 = state[8] ^ state[9];   buf3 = galois_mul2(buf3); state[8] = state[8] ^ buf3 ^ buf1;
        buf3 = state[9] ^ state[10];  buf3 = galois_mul2(buf3); state[9] = state[9] ^ buf3 ^ buf1;
        buf3 = state[10] ^ state[11]; buf3 = galois_mul2(buf3); state[10] = state[10] ^ buf3 ^ buf1;
        buf3 = state[11] ^ buf2;      buf3 = galois_mul2(buf3); state[11] = state[11] ^ buf3 ^ buf1;
        // col4
        buf1 = state[12] ^ state[13] ^ state[14] ^ state[15];
        buf2 = state[12];
        buf3 = state[12] ^ state[13]; buf3 = galois_mul2(buf3); state[12] = state[12] ^ buf3 ^ buf1;
        buf3 = state[13] ^ state[14]; buf3 = galois_mul2(buf3); state[13] = state[13] ^ buf3 ^ buf1;
        buf3 = state[14] ^ state[15]; buf3 = galois_mul2(buf3); state[14] = state[14] ^ buf3 ^ buf1;
        buf3 = state[15] ^ buf2;      buf3 = galois_mul2(buf3); state[15] = state[15] ^ buf3 ^ buf1;

    }
    // 10th round without mixcols
    state[0] = sbox[(state[0] ^ expandedKey[(round * 16)])];
    state[4] = sbox[(state[4] ^ expandedKey[(round * 16) + 4])];
    state[8] = sbox[(state[8] ^ expandedKey[(round * 16) + 8])];
    state[12] = sbox[(state[12] ^ expandedKey[(round * 16) + 12])];
    // row 1
    buf1 = state[1] ^ expandedKey[(round * 16) + 1];
    state[1] = sbox[(state[5] ^ expandedKey[(round * 16) + 5])];
    state[5] = sbox[(state[9] ^ expandedKey[(round * 16) + 9])];
    state[9] = sbox[(state[13] ^ expandedKey[(round * 16) + 13])];
    state[13] = sbox[buf1];
    // row 2
    buf1 = state[2] ^ expandedKey[(round * 16) + 2];
    buf2 = state[6] ^ expandedKey[(round * 16) + 6];
    state[2] = sbox[(state[10] ^ expandedKey[(round * 16) + 10])];
    state[6] = sbox[(state[14] ^ expandedKey[(round * 16) + 14])];
    state[10] = sbox[buf1];
    state[14] = sbox[buf2];
    // row 3
    buf1 = state[15] ^ expandedKey[(round * 16) + 15];
    state[15] = sbox[(state[11] ^ expandedKey[(round * 16) + 11])];
    state[11] = sbox[(state[7] ^ expandedKey[(round * 16) + 7])];
    state[7] = sbox[(state[3] ^ expandedKey[(round * 16) + 3])];
    state[3] = sbox[buf1];
    // last addroundkey
    state[0]  ^= expandedKey[160];
    state[1]  ^= expandedKey[161];
    state[2]  ^= expandedKey[162];
    state[3]  ^= expandedKey[163];
    state[4]  ^= expandedKey[164];
    state[5]  ^= expandedKey[165];
    state[6]  ^= expandedKey[166];
    state[7]  ^= expandedKey[167];
    state[8]  ^= expandedKey[168];
    state[9]  ^= expandedKey[169];
    state[10] ^= expandedKey[170];
    state[11] ^= expandedKey[171];
    state[12] ^= expandedKey[172];
    state[13] ^= expandedKey[173];
    state[14] ^= expandedKey[174];
    state[15] ^= expandedKey[175];
}

// straight foreward aes decryption implementation
//   the order of substeps is the exact reverse of decryption
//   inverse functions:
//       - addRoundKey is its own inverse
//       - rsbox is inverse of sbox
//       - rightshift instead of leftshift
//       - invMixColumns = barreto + mixColumns
//   no further subfunctions to save cycles for function calls
//   no structuring with "for (....)" to save cycles
void aes_decr(uint8_t state[], uint8_t expandedKey[])
{
    uint8_t buf1, buf2, buf3;
    signed char round;
    round = 9;

    // initial addroundkey
    state[0] ^= expandedKey[160];
    state[1] ^= expandedKey[161];
    state[2] ^= expandedKey[162];
    state[3] ^= expandedKey[163];
    state[4] ^= expandedKey[164];
    state[5] ^= expandedKey[165];
    state[6] ^= expandedKey[166];
    state[7] ^= expandedKey[167];
    state[8] ^= expandedKey[168];
    state[9] ^= expandedKey[169];
    state[10] ^= expandedKey[170];
    state[11] ^= expandedKey[171];
    state[12] ^= expandedKey[172];
    state[13] ^= expandedKey[173];
    state[14] ^= expandedKey[174];
    state[15] ^= expandedKey[175];

    // 10th round without mixcols
    state[0] = rsbox[state[0]] ^ expandedKey[(round * 16)];
    state[4] = rsbox[state[4]] ^ expandedKey[(round * 16) + 4];
    state[8] = rsbox[state[8]] ^ expandedKey[(round * 16) + 8];
    state[12] = rsbox[state[12]] ^ expandedKey[(round * 16) + 12];
    // row 1
    buf1 = rsbox[state[13]] ^ expandedKey[(round * 16) + 1];
    state[13] = rsbox[state[9]] ^ expandedKey[(round * 16) + 13];
    state[9] = rsbox[state[5]] ^ expandedKey[(round * 16) + 9];
    state[5] = rsbox[state[1]] ^ expandedKey[(round * 16) + 5];
    state[1] = buf1;
    // row 2
    buf1 = rsbox[state[2]] ^ expandedKey[(round * 16) + 10];
    buf2 = rsbox[state[6]] ^ expandedKey[(round * 16) + 14];
    state[2] = rsbox[state[10]] ^ expandedKey[(round * 16) + 2];
    state[6] = rsbox[state[14]] ^ expandedKey[(round * 16) + 6];
    state[10] = buf1;
    state[14] = buf2;
    // row 3
    buf1 = rsbox[state[3]] ^ expandedKey[(round * 16) + 15];
    state[3] = rsbox[state[7]] ^ expandedKey[(round * 16) + 3];
    state[7] = rsbox[state[11]] ^ expandedKey[(round * 16) + 7];
    state[11] = rsbox[state[15]] ^ expandedKey[(round * 16) + 11];
    state[15] = buf1;

    for (round = 8; round >= 0; round--){
        // barreto
        //col1
        buf1 = galois_mul2(galois_mul2(state[0] ^ state[2]));
        buf2 = galois_mul2(galois_mul2(state[1] ^ state[3]));
        state[0] ^= buf1;     state[1] ^= buf2;    state[2] ^= buf1;    state[3] ^= buf2;
        //col2
        buf1 = galois_mul2(galois_mul2(state[4] ^ state[6]));
        buf2 = galois_mul2(galois_mul2(state[5] ^ state[7]));
        state[4] ^= buf1;    state[5] ^= buf2;    state[6] ^= buf1;    state[7] ^= buf2;
        //col3
        buf1 = galois_mul2(galois_mul2(state[8] ^ state[10]));
        buf2 = galois_mul2(galois_mul2(state[9] ^ state[11]));
        state[8] ^= buf1;    state[9] ^= buf2;    state[10] ^= buf1;    state[11] ^= buf2;
        //col4
        buf1 = galois_mul2(galois_mul2(state[12] ^ state[14]));
        buf2 = galois_mul2(galois_mul2(state[13] ^ state[15]));
        state[12] ^= buf1;    state[13] ^= buf2;    state[14] ^= buf1;    state[15] ^= buf2;
        // mixcolums //////////
        // col1
        buf1 = state[0] ^ state[1] ^ state[2] ^ state[3];
        buf2 = state[0];
        buf3 = state[0] ^ state[1]; buf3 = galois_mul2(buf3); state[0] = state[0] ^ buf3 ^ buf1;
        buf3 = state[1] ^ state[2]; buf3 = galois_mul2(buf3); state[1] = state[1] ^ buf3 ^ buf1;
        buf3 = state[2] ^ state[3]; buf3 = galois_mul2(buf3); state[2] = state[2] ^ buf3 ^ buf1;
        buf3 = state[3] ^ buf2;     buf3 = galois_mul2(buf3); state[3] = state[3] ^ buf3 ^ buf1;
        // col2
        buf1 = state[4] ^ state[5] ^ state[6] ^ state[7];
        buf2 = state[4];
        buf3 = state[4] ^ state[5]; buf3 = galois_mul2(buf3); state[4] = state[4] ^ buf3 ^ buf1;
        buf3 = state[5] ^ state[6]; buf3 = galois_mul2(buf3); state[5] = state[5] ^ buf3 ^ buf1;
        buf3 = state[6] ^ state[7]; buf3 = galois_mul2(buf3); state[6] = state[6] ^ buf3 ^ buf1;
        buf3 = state[7] ^ buf2;     buf3 = galois_mul2(buf3); state[7] = state[7] ^ buf3 ^ buf1;
        // col3
        buf1 = state[8] ^ state[9] ^ state[10] ^ state[11];
        buf2 = state[8];
        buf3 = state[8] ^ state[9];   buf3 = galois_mul2(buf3); state[8] = state[8] ^ buf3 ^ buf1;
        buf3 = state[9] ^ state[10];  buf3 = galois_mul2(buf3); state[9] = state[9] ^ buf3 ^ buf1;
        buf3 = state[10] ^ state[11]; buf3 = galois_mul2(buf3); state[10] = state[10] ^ buf3 ^ buf1;
        buf3 = state[11] ^ buf2;      buf3 = galois_mul2(buf3); state[11] = state[11] ^ buf3 ^ buf1;
        // col4
        buf1 = state[12] ^ state[13] ^ state[14] ^ state[15];
        buf2 = state[12];
        buf3 = state[12] ^ state[13]; buf3 = galois_mul2(buf3); state[12] = state[12] ^ buf3 ^ buf1;
        buf3 = state[13] ^ state[14]; buf3 = galois_mul2(buf3); state[13] = state[13] ^ buf3 ^ buf1;
        buf3 = state[14] ^ state[15]; buf3 = galois_mul2(buf3); state[14] = state[14] ^ buf3 ^ buf1;
        buf3 = state[15] ^ buf2;      buf3 = galois_mul2(buf3); state[15] = state[15] ^ buf3 ^ buf1;

        // addroundkey, rsbox and shiftrows
        // row 0
        state[0] = rsbox[state[0]] ^ expandedKey[(round * 16)];
        state[4] = rsbox[state[4]] ^ expandedKey[(round * 16) + 4];
        state[8] = rsbox[state[8]] ^ expandedKey[(round * 16) + 8];
        state[12] = rsbox[state[12]] ^ expandedKey[(round * 16) + 12];
        // row 1
        buf1 = rsbox[state[13]] ^ expandedKey[(round * 16) + 1];
        state[13] = rsbox[state[9]] ^ expandedKey[(round * 16) + 13];
        state[9] = rsbox[state[5]] ^ expandedKey[(round * 16) + 9];
        state[5] = rsbox[state[1]] ^ expandedKey[(round * 16) + 5];
        state[1] = buf1;
        // row 2
        buf1 = rsbox[state[2]] ^ expandedKey[(round * 16) + 10];
        buf2 = rsbox[state[6]] ^ expandedKey[(round * 16) + 14];
        state[2] = rsbox[state[10]] ^ expandedKey[(round * 16) + 2];
        state[6] = rsbox[state[14]] ^ expandedKey[(round * 16) + 6];
        state[10] = buf1;
        state[14] = buf2;
        // row 3
        buf1 = rsbox[state[3]] ^ expandedKey[(round * 16) + 15];
        state[3] = rsbox[state[7]] ^ expandedKey[(round * 16) + 3];
        state[7] = rsbox[state[11]] ^ expandedKey[(round * 16) + 7];
        state[11] = rsbox[state[15]] ^ expandedKey[(round * 16) + 11];
        state[15] = buf1;
    }
}

// encrypt
void aes128_encrypt(uint8_t state[], uint8_t key[])
{
    uint8_t expandedKey[176];
    expandKey(expandedKey, key);       // expand the key into 176 bytes
    aes_encr(state, expandedKey);
}
void aes128_encrypt___lsbyte_to_msbyte(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[])
{
    uint8_t state_mso_to_lso[16];//MSbyte to LSbyte
    uint8_t key_mso_to_lso[16];  //MSbyte to LSbyte
    uint8_t i;
    uint8_t expandedKey[176];
    for(i=0; i<16; i++) {
        state_mso_to_lso[i] = state_lso_to_mso[15-i];
    }
    for(i=0; i<16; i++) {
        key_mso_to_lso[i]   = key_lso_to_mso[15-i];
    }
    expandKey(expandedKey, key_mso_to_lso);       // expand the key into 176 bytes
    aes_encr(state_mso_to_lso, expandedKey);
    //
    for(i=0; i<16; i++) {
        state_lso_to_mso[i] = state_mso_to_lso[15-i];
    }
}
// decrypt
void aes128_decrypt(uint8_t state[], uint8_t key[])
{
    uint8_t expandedKey[176];
    expandKey(expandedKey, key);       // expand the key into 176 bytes
    aes_decr(state, expandedKey);
}
void aes128_decrypt___lsbyte_to_msbyte(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[])
{
    uint8_t state_mso_to_lso[16];//MSbyte to LSbyte
    uint8_t key_mso_to_lso[16];  //MSbyte to LSbyte
    uint8_t i;
    uint8_t expandedKey[176];
    for(i=0; i<16; i++) {
        state_mso_to_lso[i] = state_lso_to_mso[15-i];
    }
    for(i=0; i<16; i++) {
        key_mso_to_lso[i]   = key_lso_to_mso[15-i];
    }
    expandKey(expandedKey, key_mso_to_lso);       // expand the key into 176 bytes
    aes_decr(state_mso_to_lso, expandedKey);
    //
    for(i=0; i<16; i++) {
        state_lso_to_mso[i] = state_mso_to_lso[15-i];
    }
}


//====================================================================================================
#define RAM_START_ADDR_AES128_ENCRYPT   0x0000
#define RAM_START_ADDR_AES128_DECRYPT   0x0100
void aes128_Encrypt_use_32f87251_engine(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[])
{
    uint32_t *pU32;
    
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0000, *(pU32 = (uint32_t *)(key_lso_to_mso+0))   );  // Key
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0004, *(pU32 = (uint32_t *)(key_lso_to_mso+4))   );
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0008, *(pU32 = (uint32_t *)(key_lso_to_mso+8))   );
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x000C, *(pU32 = (uint32_t *)(key_lso_to_mso+12))  );
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0010, *(pU32 = (uint32_t *)(state_lso_to_mso+0)) );  // Plain Text
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0014, *(pU32 = (uint32_t *)(state_lso_to_mso+4)) );
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x0018, *(pU32 = (uint32_t *)(state_lso_to_mso+8)) );
    RAM_WT32( RAM_START_ADDR_AES128_ENCRYPT+0x001C, *(pU32 = (uint32_t *)(state_lso_to_mso+12)));
    // PDMA Transfer on CH4
    PDMA_CH4_DT (0x00000000,                                        // IRQ
                 0x00030000+RAM_START_ADDR_AES128_ENCRYPT,          // SRC_ADDR
                 0x00040000+0x00,                                   // DES_ADDR     00h:DMA_AESW[31:0]
                 0x00080004,                                        // Data_Size
                 0x00000729);                                       // Enable 
    // PDMA Transfer on CH5
    PDMA_CH5_DT (0x00000000,                                        // IRQ
                 0x00040000+0x04,                                   // SRC_ADDR     04h:DMA_AESR[31:0]
                 0x00030000+RAM_START_ADDR_AES128_ENCRYPT+0x0110,   // DES_ADDR
                 0x00080004,                                        // Data_Size
                 0x00000789);                                       // Enable
//RF_WT08(0x100, 0x1);//debug_RFLA_pulse_to_trigger_LA()  debug
    #if 0
    {
        AES_WT08(0x08,0x63);
        delay_unit625us(2);//avoid dma bus conflict, HW take about 12us to Encrypt/Decrypt
        while ( (AES_RD08(0x08) & 0x01) == 0x01 ) //08h:[0]AES_EN¡GAES-128 enable. When encryption or decryption complete, it clear automatically. 0h¡GDisable, 1h¡GEnable
        {
        } // wait 
    }
    #else
    {
        AES_WT08(0x08,0x73);
                                            //08h [0]  AES_EN    AES-128 enable. When encryption or decryption complete, it clear automatically. 0h¡GDisable, 1h¡GEnabl
                                            //    [1]  CRYP_SEL  Crypto mode selection. 0h¡GDecryption, 1h¡GEncryption
                                            //    [3:2]-
                                            //    [4]  IRQ_EN¡GInterrupt service request (IRQ) enable. 0h¡GDisable, 1h¡GEnable
                                            //    [5]  IRQ_CLR_EN¡GIRQ auto clear after read enable. 0h¡GDisable, 1h¡GEnable
                                            //    [6]  IRQ_ACT_SEL¡GIRQ active selection. 0h¡GLow active, 1h¡GHigh active
                                            //    [7]  -
        while ( aes128_irq_occur == 0 ) {} // wait :=1 at BC5602_IRQ1_ISR()
                aes128_irq_occur = 0;      // clear flag
    }
    #endif
//RF_WT08(0x100, 0x1);//debug_RFLA_pulse_to_trigger_LA()  debug
        *(pU32 = (uint32_t *)(state_lso_to_mso+0)) = RAM_RD32( RAM_START_ADDR_AES128_ENCRYPT+0x0110+0 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+4)) = RAM_RD32( RAM_START_ADDR_AES128_ENCRYPT+0x0110+4 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+8)) = RAM_RD32( RAM_START_ADDR_AES128_ENCRYPT+0x0110+8 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+12))= RAM_RD32( RAM_START_ADDR_AES128_ENCRYPT+0x0110+12);
    
 // debug_print_16_bytes_inverse(state_lso_to_mso);//debug
}

void aes128_Decrypt_use_32f87251_engine(uint8_t state_lso_to_mso[], uint8_t key_lso_to_mso[])
{
    uint32_t *pU32;
    
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0000, *(pU32 = (uint32_t *)(key_lso_to_mso+0))   );  // Key
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0004, *(pU32 = (uint32_t *)(key_lso_to_mso+4))   );
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0008, *(pU32 = (uint32_t *)(key_lso_to_mso+8))   );
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x000C, *(pU32 = (uint32_t *)(key_lso_to_mso+12))  );
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0010, *(pU32 = (uint32_t *)(state_lso_to_mso+0)) );  // Cipher Text
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0014, *(pU32 = (uint32_t *)(state_lso_to_mso+4)) );
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x0018, *(pU32 = (uint32_t *)(state_lso_to_mso+8)) );
    RAM_WT32( RAM_START_ADDR_AES128_DECRYPT+0x001C, *(pU32 = (uint32_t *)(state_lso_to_mso+12)));
    // PDMA Transfer on CH4
    PDMA_CH4_DT (0x00000000,                                        // IRQ
                 0x00030000+RAM_START_ADDR_AES128_DECRYPT,          // SRC_ADDR
                 0x00040000+0x00,                                   // DES_ADDR     00h:DMA_AESW[31:0]
                 0x00080004,                                        // Data_Size
                 0x00000729);                                       // Enable 
    // PDMA Transfer on CH5
    PDMA_CH5_DT (0x00000000,                                        // IRQ
                 0x00040000+0x04,                                   // SRC_ADDR     04h:DMA_AESR[31:0]
                 0x00030000+RAM_START_ADDR_AES128_DECRYPT+0x0110,   // DES_ADDR
                 0x00080004,                                        // Data_Size
                 0x00000789);                                       // Enable
//RF_WT08(0x100, 0x1);//debug_RFLA_pulse_to_trigger_LA()  debug
    #if 0
    {
        AES_WT08(0x08,0x61);
        delay_unit625us(2);//avoid dma bus conflict, HW take about 12us to Encrypt/Decrypt
        while ( (AES_RD08(0x08) & 0x01) == 0x01 ) //08h:[0]AES_EN¡GAES-128 enable. When encryption or decryption complete, it clear automatically. 0h¡GDisable, 1h¡GEnable
        {
        } // wait 
    }
    #else
    {
        AES_WT08(0x08,0x71);
                                            //08h [0]  AES_EN    AES-128 enable. When encryption or decryption complete, it clear automatically. 0h¡GDisable, 1h¡GEnabl
                                            //    [1]  CRYP_SEL  Crypto mode selection. 0h¡GDecryption, 1h¡GEncryption
                                            //    [3:2]-
                                            //    [4]  IRQ_EN¡GInterrupt service request (IRQ) enable. 0h¡GDisable, 1h¡GEnable
                                            //    [5]  IRQ_CLR_EN¡GIRQ auto clear after read enable. 0h¡GDisable, 1h¡GEnable
                                            //    [6]  IRQ_ACT_SEL¡GIRQ active selection. 0h¡GLow active, 1h¡GHigh active
                                            //    [7]  -
        while ( aes128_irq_occur == 0 ) {} // wait :=1 at BC5602_IRQ1_ISR()
                aes128_irq_occur = 0;      // clear flag
    }
    #endif
//RF_WT08(0x100, 0x1);//debug_RFLA_pulse_to_trigger_LA()  debug
        *(pU32 = (uint32_t *)(state_lso_to_mso+0)) = RAM_RD32( RAM_START_ADDR_AES128_DECRYPT+0x0110+0 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+4)) = RAM_RD32( RAM_START_ADDR_AES128_DECRYPT+0x0110+4 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+8)) = RAM_RD32( RAM_START_ADDR_AES128_DECRYPT+0x0110+8 );
        *(pU32 = (uint32_t *)(state_lso_to_mso+12))= RAM_RD32( RAM_START_ADDR_AES128_DECRYPT+0x0110+12);
    
    debug_print_16_bytes_inverse(state_lso_to_mso);//debug
}


//====================================================================================================
#ifdef _CODE_TEST_AESAVS_PATTERN_
uint8_t set_sample_1_plaintxt[2][16] = { //MSbyte to LSbyte & msbit to lsbit
    { 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF },   //fips-197 page   39 C.1 AES-128
    { 0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13 }    //vol-6 part-C 1.1 hci_le_encrypt command
};
uint8_t set_sample_1_key[2][16] = {      //MSbyte to LSbyte & msbit to lsbit
    { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F },   //fips-197 page   39 C.1 AES-128
    { 0x4C,0x68,0x38,0x41,0x39,0xF5,0x74,0xD8,0x36,0xBC,0xF3,0x4E,0x9D,0xFB,0x01,0xBF }    //vol-6 part-C 1.1 hci_le_encrypt command
      // LTK = 0x4C68384139F574D836BCF34E9DFB01BF (MSO to LSO)
};
uint8_t set_sample_aesavs_B_1_plaintxt[7][16] = {  //aesavs page 16 appendix B.1 keysize=128
    { 0xf3,0x44,0x81,0xec,0x3c,0xc6,0x27,0xba,0xcd,0x5d,0xc3,0xfb,0x08,0xf2,0x73,0xe6 },
    { 0x97,0x98,0xc4,0x64,0x0b,0xad,0x75,0xc7,0xc3,0x22,0x7d,0xb9,0x10,0x17,0x4e,0x72 },
    { 0x96,0xab,0x5c,0x2f,0xf6,0x12,0xd9,0xdf,0xaa,0xe8,0xc3,0x1f,0x30,0xc4,0x21,0x68 },
    { 0x6a,0x11,0x8a,0x87,0x45,0x19,0xe6,0x4e,0x99,0x63,0x79,0x8a,0x50,0x3f,0x1d,0x35 },
    { 0xcb,0x9f,0xce,0xec,0x81,0x28,0x6c,0xa3,0xe9,0x89,0xbd,0x97,0x9b,0x0c,0xb2,0x84 },
    { 0xb2,0x6a,0xeb,0x18,0x74,0xe4,0x7c,0xa8,0x35,0x8f,0xf2,0x23,0x78,0xf0,0x91,0x44 },
    { 0x58,0xc8,0xe0,0x0b,0x26,0x31,0x68,0x6d,0x54,0xea,0xb8,0x4b,0x91,0xf0,0xac,0xa1 }
};
uint8_t set_sample_aesavs_C_1_key[21][16] = {      //aesavs page 17 appendix C.1 keysize=128
    { 0x10,0xa5,0x88,0x69,0xd7,0x4b,0xe5,0xa3,0x74,0xcf,0x86,0x7c,0xfb,0x47,0x38,0x59 },
    { 0xca,0xea,0x65,0xcd,0xbb,0x75,0xe9,0x16,0x9e,0xcd,0x22,0xeb,0xe6,0xe5,0x46,0x75 },
    { 0xa2,0xe2,0xfa,0x9b,0xaf,0x7d,0x20,0x82,0x2c,0xa9,0xf0,0x54,0x2f,0x76,0x4a,0x41 },
    { 0xb6,0x36,0x4a,0xc4,0xe1,0xde,0x1e,0x28,0x5e,0xaf,0x14,0x4a,0x24,0x15,0xf7,0xa0 },
    { 0x64,0xcf,0x9c,0x7a,0xbc,0x50,0xb8,0x88,0xaf,0x65,0xf4,0x9d,0x52,0x19,0x44,0xb2 },
    { 0x47,0xd6,0x74,0x2e,0xef,0xcc,0x04,0x65,0xdc,0x96,0x35,0x5e,0x85,0x1b,0x64,0xd9 },
    { 0x3e,0xb3,0x97,0x90,0x67,0x8c,0x56,0xbe,0xe3,0x4b,0xbc,0xde,0xcc,0xf6,0xcd,0xb5 },
    { 0x64,0x11,0x0a,0x92,0x4f,0x07,0x43,0xd5,0x00,0xcc,0xad,0xae,0x72,0xc1,0x34,0x27 },
    { 0x18,0xd8,0x12,0x65,0x16,0xf8,0xa1,0x2a,0xb1,0xa3,0x6d,0x9f,0x04,0xd6,0x8e,0x51 },
    { 0xf5,0x30,0x35,0x79,0x68,0x57,0x84,0x80,0xb3,0x98,0xa3,0xc2,0x51,0xcd,0x10,0x93 },
    { 0xda,0x84,0x36,0x7f,0x32,0x5d,0x42,0xd6,0x01,0xb4,0x32,0x69,0x64,0x80,0x2e,0x8e },
    { 0xe3,0x7b,0x1c,0x6a,0xa2,0x84,0x6f,0x6f,0xdb,0x41,0x3f,0x23,0x8b,0x08,0x9f,0x23 },
    { 0x6c,0x00,0x2b,0x68,0x24,0x83,0xe0,0xca,0xbc,0xc7,0x31,0xc2,0x53,0xbe,0x56,0x74 },
    { 0x14,0x3a,0xe8,0xed,0x65,0x55,0xab,0xa9,0x61,0x10,0xab,0x58,0x89,0x3a,0x8a,0xe1 },
    { 0xb6,0x94,0x18,0xa8,0x53,0x32,0x24,0x0d,0xc8,0x24,0x92,0x35,0x39,0x56,0xae,0x0c },
    { 0x71,0xb5,0xc0,0x8a,0x19,0x93,0xe1,0x36,0x2e,0x4d,0x0c,0xe9,0xb2,0x2b,0x78,0xd5 },
    { 0xe2,0x34,0xcd,0xca,0x26,0x06,0xb8,0x1f,0x29,0x40,0x8d,0x5f,0x6d,0xa2,0x12,0x06 },
    { 0x13,0x23,0x7c,0x49,0x07,0x4a,0x3d,0xa0,0x78,0xdc,0x1d,0x82,0x8b,0xb7,0x8c,0x6f },
    { 0x30,0x71,0xa2,0xa4,0x8f,0xe6,0xcb,0xd0,0x4f,0x1a,0x12,0x90,0x98,0xe3,0x08,0xf8 },
    { 0x90,0xf4,0x2e,0xc0,0xf6,0x83,0x85,0xf2,0xff,0xc5,0xdf,0xc0,0x3a,0x65,0x4d,0xce },
    { 0xfe,0xbd,0x9a,0x24,0xd8,0xb6,0x5c,0x1c,0x78,0x7d,0x50,0xa4,0xed,0x36,0x19,0xa9 }
};
void test_32f87251_engine_aes128_____patterns_aesavs(void)
{
    uint8_t m, i;
    uint8_t state[16]; //plaintext, ciphertext
    uint8_t key[16];
    uint8_t temp[16];

    RF_WT08(0x200,0x0);  //wirte (RF_BASS+ 0x200,0x0) switch AES128 GIO
    AES_WT08(0x10,0x81); //01h GIOTM_O = {AES_IRQ_CLR (out), AES_IRQ (out), AESR_CMP (out), AESR_EN (out), AESW_CMP (out), AESW_EN (out), AES_CMP (out), AES_EN (out), CRYP_SEL (out), AES_CS [2:0] (out), HCLK (out)}

//while(1)//debug keep...
  {
    for (m=0; m<2; m++) {
        for (i=0; i<16; i++) { state[i] = set_sample_1_plaintxt[m][15-i]; }
        for (i=0; i<16; i++) {   key[i] = set_sample_1_key[m][15-i]; }
        aes128_Encrypt_use_32f87251_engine(state, key);
        aes128_Decrypt_use_32f87251_engine(state, key);
    }
    for (m=0; m<7; m++) {     //aesavs page 16 appendix B.1 keysize=128
        for (i=0; i<16; i++) { state[i] = set_sample_aesavs_B_1_plaintxt[m][15-i]; }
        for (i=0; i<16; i++) {   key[i] = 0x00; }
        aes128_Encrypt_use_32f87251_engine(state, key);
        aes128_Decrypt_use_32f87251_engine(state, key);
    }
    for (m=0; m<21; m++) {    //aesavs page 17 appendix C.1 keysize=128
        for (i=0; i<16; i++) { state[i] = 0x00; }
        for (i=0; i<16; i++) {   key[i] = set_sample_aesavs_C_1_key[m][15-i]; }
        aes128_Encrypt_use_32f87251_engine(state, key);
        aes128_Decrypt_use_32f87251_engine(state, key);
    }
    memset(temp + 0,0x00, 16);
    for (m=0; m<128; m++) {    //aesavs page 20 appendix D.1 keysize=128
        for (i=0; i <= 14; i++) {
            temp[15 - i] >>= 1;
            if (temp[14 - i] & 0x01) { temp[15 - i] |= 0x80; }
        }
        temp[0] = (temp[0] >> 1) | 0x80;
        //
        for (i=0; i<16; i++) { state[i] = temp[15-i]; }
        for (i=0; i<16; i++) {   key[i] = 0x00; }
        aes128_Encrypt_use_32f87251_engine(state, key);
        aes128_Decrypt_use_32f87251_engine(state, key);
    }
    memset(temp + 0,0x00, 16);
    for (m=0; m<128; m++) {    //aesavs page 29 appendix E.1 keysize=128
        for (i=0; i <= 14; i++) {
            temp[15 - i] >>= 1;
            if (temp[14 - i] & 0x01) { temp[15 - i] |= 0x80; }
        }
        temp[0] = (temp[0] >> 1) | 0x80;
        //
        for (i=0; i<16; i++) { state[i] = 0x00; }
        for (i=0; i<16; i++) {   key[i] = temp[15-i]; }
        aes128_Encrypt_use_32f87251_engine(state, key);
        aes128_Decrypt_use_32f87251_engine(state, key);
    }
  }//end while(1)
}
#endif


//====================================================================================================
uint8_t vol6_partc_1_LTK_mso_to_lso[16] = {
                                                    //  LTK = 0x4C68384139F574D836BCF34E9DFB01BF (MSO to LSO)
    0x4C,0x68,0x38,0x41, 0x39,0xF5,0x74,0xD8, 0x36,0xBC,0xF3,0x4E, 0x9D,0xFB,0x01,0xBF    //MSbyte to LSbyte & msbit to lsbit
  //4C:68:38:41:39:F5:74:D8:36:BC:F3:4E:9D:FB:01:BF    //MSbyte to LSbyte & msbit to lsbit
  //BF:01:FB:9D:4E:F3:BC:36:D8:74:F5:39:41:38:68:4C    //LSbyte to MSbyte
};
uint8_t vol6_partc_1_LTK_lso_to_mso[16] = {
                                                    //  LTK = 0x4C68384139F574D836BCF34E9DFB01BF (MSO to LSO)
    0xBF,0x01,0xFB,0x9D, 0x4E,0xF3,0xBC,0x36, 0xD8,0x74,0xF5,0x39, 0x41,0x38,0x68,0x4C    //LSbyte to MSbyte & msbit to lsbit
  //4C:68:38:41:39:F5:74:D8:36:BC:F3:4E:9D:FB:01:BF    //MSbyte to LSbyte & msbit to lsbit
  //BF:01:FB:9D:4E:F3:BC:36:D8:74:F5:39:41:38:68:4C    //LSbyte to MSbyte
};

uint8_t vol6_partc_1_SKD_mso_to_lso[16]= {  //1. SKD
    0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,   //SKDs = 0x0213243546576879 (MSO to LSO)
    0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
  //0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC,   //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
  //0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};
uint8_t vol6_partc_1_SKD_lso_to_mso[16]= {  //1. SKD
  //0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79,   //SKDs = 0x0213243546576879 (MSO to LSO)
  //0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
    0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC,   //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
    0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};
  //99ad1b5226a37e3e058e3b8e27c2c666 ==== vol6_partc_1_2_SK
  //0213243546576879acbdcedfe0f10213
uint8_t  vol6_partc_1_SKDm_lso_to_mso[8]= { //1. SKD
  //0xAC,0xBD,0xCE,0xDF,0xE0,0xF1,0x02,0x13    //SKDm = 0xACBDCEDFE0F10213 (MSO to LSO)
    0x13,0x02,0xF1,0xE0,0xDF,0xCE,0xBD,0xAC    //SKDm (LSO to MSO)    :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
};
uint8_t  vol6_partc_1_SKDs_lso_to_mso[8]= { //1. SKD
  //0x02,0x13,0x24,0x35,0x46,0x57,0x68,0x79    //SKDs = 0x0213243546576879 (MSO to LSO)
    0x79,0x68,0x57,0x46,0x35,0x24,0x13,0x02    //SKDs (LSO to MSO)    :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
};

void test_32f87251_engine_aes128_____pattern_vol6_partc_1(void)
{
    uint16_t i;
    uint32_t *pU32;

    RF_WT08(0x200, 0x0);  //wirte (RF_BASS+ 0x200, 0x0) switch AES128 GIO
    AES_WT08(0x10, 0x81); //01h GIOTM_O = {AES_IRQ_CLR (out), AES_IRQ (out), AESR_CMP (out), AESR_EN (out), AESW_CMP (out), AESW_EN (out), AES_CMP (out), AES_EN (out), CRYP_SEL (out), AES_CS [2:0] (out), HCLK (out)}
  //
    RAM_WT32(0x0000, *(pU32 = (uint32_t *)(vol6_partc_1_LTK_lso_to_mso+0)));  // Key
    RAM_WT32(0x0004, *(pU32 = (uint32_t *)(vol6_partc_1_LTK_lso_to_mso+4)));
    RAM_WT32(0x0008, *(pU32 = (uint32_t *)(vol6_partc_1_LTK_lso_to_mso+8)));
    RAM_WT32(0x000C, *(pU32 = (uint32_t *)(vol6_partc_1_LTK_lso_to_mso+12)));
    RAM_WT32(0x0010, *(pU32 = (uint32_t *)(vol6_partc_1_SKD_lso_to_mso+0)));  // Plain Text
    RAM_WT32(0x0014, *(pU32 = (uint32_t *)(vol6_partc_1_SKD_lso_to_mso+4)));
    RAM_WT32(0x0018, *(pU32 = (uint32_t *)(vol6_partc_1_SKD_lso_to_mso+8)));
    RAM_WT32(0x001C, *(pU32 = (uint32_t *)(vol6_partc_1_SKD_lso_to_mso+12)));
  //
    // PDMA Transfer on CH4
    PDMA_CH4_DT (0x00000000,              // IRQ
                 0x00030000, 0x00040000,  // SRC_ADDR,  DES_ADDR
                 0x00080004, 0x00000729); // Data_Size, Enable 

    // PDMA Transfer on CH5
    PDMA_CH5_DT (0x00000000,              // IRQ
                 0x00040004, 0x00030110,  // SRC_ADDR,  DES_ADDR
                 0x00080004, 0x00000789); // Data_Size, Enable
      //
        AES_WT08(0x08, 0x73);
        while ( aes128_irq_occur == 0 ) {}
                aes128_irq_occur = 0;
      //
    for (i=0; i<16; i++) {
        uart_putu8( RAM_RD08(0x0110+i) );
        uart_puts(",");
    }
        uart_putchar_n('\n');
}

void test_aes128(void)
{
  //test software aes128
    #if 0
    {
        uint8_t i;
        uint8_t state[16];
        ///////////////////////////////
        for(i=0; i<16; i++) {
            state[i] = vol6_partc_1_SKD_mso_to_lso[i];
        }
        aes128_encrypt(state+0, vol6_partc_1_LTK_mso_to_lso+0);
        for(i=0; i<16; i++) {
            uart_putu8(state[i]); uart_puts(" ");
        }
            uart_putchar_n('\n');
        aes128_decrypt(state, vol6_partc_1_LTK_mso_to_lso+0);
        for(i=0; i<16; i++) {
            uart_putu8(state[i]); uart_puts(" ");
        }
            uart_putchar_n('\n');
        ///////////////////////////////
        for(i=0; i<16; i++) {
            state[i] = vol6_partc_1_SKD_lso_to_mso[i];
        }
        aes128_encrypt___lsbyte_to_msbyte(state+0, vol6_partc_1_LTK_lso_to_mso+0);
        for(i=0; i<16; i++) {
            uart_putu8(state[15-i]); uart_puts(" ");
        }
            uart_putchar_n('\n');
        aes128_decrypt___lsbyte_to_msbyte(state, vol6_partc_1_LTK_lso_to_mso+0);
        for(i=0; i<16; i++) {
            uart_putu8(state[15-i]); uart_puts(" ");
        }
            uart_putchar_n('\n');
    }
    #endif
  //test 32f87251 engine aes128
  //test_32f87251_engine_aes128_____pattern_vol6_partc_1();
    test_32f87251_engine_aes128_____patterns_aesavs();
}


/** @} */
