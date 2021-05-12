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

#include <pthread.h>
#include <cstdint>
#include "komodo_structs.h"
#include "komodo_cutils.h"

typedef struct queue
{
	struct queueitem *list;
	pthread_mutex_t mutex;
    char name[64],initflag;
} queue_t; // also defined in cc/OS_portable.h

/***
 * These 4 defines are not used by komodo_utils.cpp, but do serve
 * to define them for other .h files. These should be moved (or migrated to std::mutex).
 */
#define portable_mutex_t pthread_mutex_t
#define portable_mutex_init(ptr) pthread_mutex_init(ptr,NULL)
#define portable_mutex_lock pthread_mutex_lock
#define portable_mutex_unlock pthread_mutex_unlock

#define CRYPTO777_PUBSECPSTR "020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9"
#define CRYPTO777_KMDADDR "RXL3YXG2ceaB6C5hfJcN4fvmLH2C34knhA"
#define CRYPTO777_RMD160STR "f1dce4182fce875748c4986b240ff7d7bc3fffb0"

#define KOMODO_PUBTYPE 60

void vcalc_sha256(char deprecated[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len);

bits256 bits256_doublesha256(char *deprecated,uint8_t *data,int32_t datalen);

uint32_t calc_crc32(uint32_t crc,const void *buf,size_t size);

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen);

int32_t bitcoin_addr2rmd160(uint8_t *addrtypep,uint8_t rmd160[20],char *coinaddr);

char *bitcoin_address(char *coinaddr,uint8_t addrtype,uint8_t *pubkey_or_rmd160,int32_t len);

int32_t komodo_is_issuer();

int32_t bitweight(uint64_t x);

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp);

int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp);

int32_t komodo_scriptitemlen(int32_t *opretlenp,uint8_t *script);

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

#ifndef _WIN32
void OS_randombytes(unsigned char *x,long xlen);
#endif

void lock_queue(queue_t *queue);

void queue_enqueue(char *name,queue_t *queue,struct queueitem *item);

struct queueitem *queue_dequeue(queue_t *queue);

void *queue_delete(queue_t *queue,struct queueitem *copy,int32_t copysize);

void *queue_free(queue_t *queue);

void *queue_clone(queue_t *clone,queue_t *queue,int32_t size);

int32_t queue_size(queue_t *queue);

void iguana_initQ(queue_t *Q,char *name);

uint16_t _komodo_userpass(char *username,char *password,FILE *fp);

void komodo_statefname(char *fname,char *symbol,char *str);

void komodo_configfile(char *symbol,uint16_t rpcport);

uint16_t komodo_userpass(char *userpass,char *symbol);

uint32_t komodo_assetmagic(char *symbol,uint64_t supply,uint8_t *extraptr,int32_t extralen);

uint16_t komodo_assetport(uint32_t magic,int32_t extralen);

uint16_t komodo_port(char *symbol,uint64_t supply,uint32_t *magicp,uint8_t *extraptr,int32_t extralen);

/***
 * Find the notary id and pubkey for the given information
 * @param pubkeystr where the pubkey will be placed
 * @param height
 * @param timestamp
 * @returns the notary id
 */
int32_t komodo_whoami(char *pubkeystr,int32_t height,uint32_t timestamp);

uint64_t komodo_max_money();

uint64_t komodo_ac_block_subsidy(int nHeight);

int8_t equihash_params_possible(uint64_t n, uint64_t k);

void komodo_args(char *argv0);

void komodo_nameset(char *symbol,char *dest,char *source);

/****
 * Find the appropriate state object
 * @param base the symbol to look for (nullptr will return state object for KMD)
 * @returns the correct state object, KOMODO_STATES[0] if not found in CURRENCIES
 */
struct komodo_state *komodo_stateptrget(char *base);

/****
 * Retrieve a pointer to the state object
 * @param symbol the symbol (will be filled in with "KMD" or ASSETCHAINS_SYMBOL
 * @param dest the dest (will filled in with either "BTC" or "KMD" depending on if ASSETCHAINS_SYMBOL is filled in)
 * @returns the appropriate state object that matches symbol
 */
struct komodo_state *komodo_stateptr(char *symbol,char *dest);

/***
 * prefetch an entire file, but do nothing with what was read.
 * @param fp the file
 */
void komodo_prefetch(FILE *fp);
