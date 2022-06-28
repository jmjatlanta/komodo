/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
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
#ifndef OS_PORTABLEH
#define OS_PORTABLEH

// iguana_OS has functions that invoke system calls. Whenever possible stdio and 
// similar functions are use and most functions are fully portable and in this file. 
// For things that require OS specific, the call is routed to iguana_OS_portable_*  
// Usually, all but one OS can be handled with the same code, so iguana_OS_portable.c 
// has most of this shared logic and an #ifdef iguana_OS_nonportable.c

#include "bits256.h"

#include <stdint.h>

#ifdef _WIN32
#define sleep(x) Sleep(1000*(x))
#include "../OSlibs/win/mingw.h"
#include "../OSlibs/win/mman.h"
#define PTW32_STATIC_LIB
#include "../OSlibs/win/pthread.h"

#ifndef NATIVE_WINDOWS
#define EADDRINUSE WSAEADDRINUSE
#endif

#endif

#ifndef MIN
#define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif

#define SATOSHIDEN ((uint64_t)100000000L)
#define dstr(x) ((double)(x) / SATOSHIDEN)

#define SMALLVAL 0.000000000000001

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

void OS_randombytes(uint8_t *x,long xlen);

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen);

int32_t safecopy(char *dest,const char *src,long len);

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp);
int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp);
#define bits256_nonz(a) (((a).ulongs[0] | (a).ulongs[1] | (a).ulongs[2] | (a).ulongs[3]) != 0)
bits256 bits256_doublesha256(char *hashstr,uint8_t *data,int32_t datalen);
char *bits256_str(char hexstr[65],bits256 x);

#endif
