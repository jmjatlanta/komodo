/******************************************************************************
 * Copyright © 2014-2021 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#pragma once
#include <stdint.h>

/******************************
 * These are utilities that are used by both C (i.e. cJSON, gmp) and C++
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _BITS256
#define _BITS256
    union _bits256 { uint8_t bytes[32]; uint16_t ushorts[16]; uint32_t uints[8]; uint64_t ulongs[4]; uint64_t txid; };
    typedef union _bits256 bits256;
#endif

/***
 * Copy a null-terminated string
 * @param dest the destination
 * @param src the source
 * @param len the length to copy
 * @returns 1 on success, -1 on failure
 */
int32_t safecopy(char *dest,char *src,long len);

/***
 * Copy a string
 * @param str the incoming string
 * @returns a pointer to allocated memory containing a copy of str
 */
char *clonestr(char *str);

unsigned char _decode_hex(char *hex);

int32_t decode_hex(uint8_t *bytes,int32_t n,char *hex);

/****
 * Checks if the string contains hex symbols
 * @param str the string
 * @param n length to check (set to zero to run until null terminator)
 * @returns non-zero if str has hex symbols
 */
int32_t is_hexstr(char *str,int32_t n);

int32_t unhex(char c);

char *bits256_str(char hexstr[65],bits256 x);

char hexbyte(int32_t c);

int32_t init_hexbytes_noT(char *hexbytes,unsigned char *message,long len);

long _stripwhite(char *buf,int accept);

#ifdef __cplusplus
}
#endif
