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
#pragma once

#include "komodo_defs.h"
#include "mini-gmp.h"
#include "hex.h"
#include "key_io.h"
#include "cc/CCinclude.h"
#include <string.h>

#ifdef _WIN32
#include <sodium.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#endif

#define SATOSHIDEN ((uint64_t)100000000L)
#define dstr(x) ((double)(x) / SATOSHIDEN)

struct rmd160_vstate { uint64_t length; uint8_t buf[64]; uint32_t curlen, state[5]; };

void vcalc_sha256(char deprecated[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len);

bits256 bits256_doublesha256(char *deprecated,uint8_t *data,int32_t datalen);

/**
 Process a block of memory though the hash
 @param md     The hash state
 @param in     The data to hash
 @param inlen  The length of the data (octets)
 @return 0 if successful
 */
int rmd160_vprocess (struct rmd160_vstate * md, const unsigned char *in, unsigned long inlen);

uint32_t calc_crc32(uint32_t crc,const void *buf,size_t size);

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen);

int32_t bitcoin_addr2rmd160(uint8_t *addrtypep,uint8_t rmd160[20],char *coinaddr);

char *bitcoin_address(char *coinaddr,uint8_t addrtype,uint8_t *pubkey_or_rmd160,int32_t len);

int32_t komodo_is_issuer();

int32_t bitweight(uint64_t x);

char *bits256_str(char hexstr[65],bits256 x);

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp);

int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp);

int32_t komodo_opreturnscript(uint8_t *script,uint8_t type,uint8_t *opret,int32_t opretlen);

// get a pseudo random number that is the same for each block individually at all times and different
// from all other blocks. the sequence is extremely likely, but not guaranteed to be unique for each block chain
uint64_t komodo_block_prg(uint32_t nHeight);

// given a block height, this returns the unlock time for that block height, derived from
// the ASSETCHAINS_MAGIC number as well as the block height, providing different random numbers
// for corresponding blocks across chains, but the same sequence in each chain
int64_t komodo_block_unlocktime(uint32_t nHeight);

char *parse_conf_line(char *line,char *field);

double OS_milliseconds();

void komodo_statefname(char *fname,char *symbol,char *str);

int32_t komodo_whoami(char *pubkeystr,int32_t height,uint32_t timestamp);

uint64_t komodo_ac_block_subsidy(int nHeight);

void komodo_nameset(char *symbol,char *dest,char *source);

struct komodo_state *komodo_stateptrget(char *base);

struct komodo_state *komodo_stateptr(char *symbol,char *dest);

void komodo_prefetch(FILE *fp);

// check if block timestamp is more than S5 activation time
// this function is to activate the ExtractDestination fix 
bool komodo_is_vSolutionsFixActive();
