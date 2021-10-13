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

#include "arith_uint256.h"
#define ASSETCHAINS_MAX_ERAS 7 // needed by chain.h
#include "chain.h"
#include "komodo_nk.h"

#define NUM_KMD_SEASONS 6
#define NUM_KMD_NOTARIES 64
#define KOMODO_EARLYTXID_HEIGHT 100
#define ASSETCHAINS_MINHEIGHT 128
#define KOMODO_ELECTION_GAP 2000
#define ROUNDROBIN_DELAY 61
#define KOMODO_ASSETCHAIN_MAXLEN 65
#define KOMODO_LIMITED_NETWORKSIZE 4
#define IGUANA_MAXSCRIPTSIZE 10001
#define KOMODO_MAXMEMPOOLTIME 3600 // affects consensus
#define CRYPTO777_PUBSECPSTR "020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9"
#define KOMODO_FIRSTFUNGIBLEID 100
#define KOMODO_SAPLING_ACTIVATION 1544832000 // Dec 15th, 2018
#define KOMODO_SAPLING_DEADLINE 1550188800 // Feb 15th, 2019
#define ASSETCHAINS_STAKED_BLOCK_FUTURE_MAX 57
#define ASSETCHAINS_STAKED_BLOCK_FUTURE_HALF 27
#define ASSETCHAINS_STAKED_MIN_POW_DIFF 536900000 // 537000000 537300000
#define _COINBASE_MATURITY 100
#define _ASSETCHAINS_TIMELOCKOFF 0xffffffffffffffff
#define MAX_CURRENCIES 32

// KMD Notary Seasons 
// 1: May 1st 2018 1530921600
// 2: July 15th 2019 1563148800 -> estimated height 1444000
// 3: 3rd season ending isnt known, so use very far times in future.
    // 1751328000 = dummy timestamp, 1 July 2025!
    // 7113400 = 5x current KMD blockheight. 
// to add 4th season, change NUM_KMD_SEASONS to 4, and add timestamp and height of activation to these arrays. 

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

#define KOMODO_MAXNVALUE (((uint64_t)1 << 63) - 1)
#define KOMODO_BIT63SET(x) ((x) & ((uint64_t)1 << 63))
#define KOMODO_VALUETOOBIG(x) ((x) > (uint64_t)10000000001*COIN)
#define PRICES_DAYWINDOW ((3600*24/ASSETCHAINS_BLOCKTIME) + 1)

#define IGUANA_MAXSCRIPTSIZE 10001
#define KOMODO_KVDURATION 1440
#define KOMODO_KVBINARY 2
#define PRICES_SMOOTHWIDTH 1
#define PRICES_MAXDATAPOINTS 8

#ifndef KOMODO_NSPV_FULLNODE
#define KOMODO_NSPV_FULLNODE (KOMODO_NSPV <= 0)
#endif // !KOMODO_NSPV_FULLNODE
#ifndef KOMODO_NSPV_SUPERLITE
#define KOMODO_NSPV_SUPERLITE (KOMODO_NSPV > 0)
#endif // !KOMODO_NSPV_SUPERLITE

extern uint8_t ASSETCHAINS_TXPOW;
extern uint8_t ASSETCHAINS_PUBLIC;
extern int8_t ASSETCHAINS_ADAPTIVEPOW;
extern uint16_t ASSETCHAINS_P2PPORT;
extern uint16_t ASSETCHAINS_RPCPORT;
extern uint32_t ASSETCHAINS_MAGIC;
extern int32_t VERUS_BLOCK_POSUNITS;
extern int32_t ASSETCHAINS_LWMAPOS;
extern int32_t ASSETCHAINS_BLOCKTIME;
extern uint64_t ASSETCHAINS_SUPPLY;
extern uint64_t ASSETCHAINS_FOUNDERS_REWARD;
extern uint32_t ASSETCHAINS_ALGO;
extern uint32_t ASSETCHAINS_VERUSHASH;
extern uint32_t ASSETCHAINS_EQUIHASH;
extern uint32_t KOMODO_INITDONE;
extern bool IS_KOMODO_NOTARY;
extern int32_t KOMODO_MININGTHREADS;
extern int32_t KOMODO_LONGESTCHAIN;
extern int32_t ASSETCHAINS_SEED;
extern int32_t KOMODO_CHOSEN_ONE;
extern int32_t KOMODO_ON_DEMAND;
extern int32_t KOMODO_PASSPORT_INITDONE;
extern int32_t ASSETCHAINS_STAKED;
extern int32_t KOMODO_NSPV;
extern uint64_t ASSETCHAINS_COMMISSION;
extern uint64_t ASSETCHAINS_LASTERA;
extern uint64_t ASSETCHAINS_CBOPRET;
extern bool VERUS_MINTBLOCKS;
extern uint64_t ASSETCHAINS_REWARD[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_NOTARY_PAY[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_NONCEMASK[];
extern uint64_t ASSETCHAINS_NK[2];
extern const char *ASSETCHAINS_ALGORITHMS[];
extern int32_t VERUS_MIN_STAKEAGE;
extern uint32_t ASSETCHAINS_VERUSHASH;
extern uint32_t ASSETCHAINS_VERUSHASHV1_1;
extern uint32_t ASSETCHAINS_NONCESHIFT[];
extern uint32_t ASSETCHAINS_HASHESPERROUND[];
extern std::string NOTARY_PUBKEY;
extern std::string ASSETCHAINS_OVERRIDE_PUBKEY;
extern std::string ASSETCHAINS_SCRIPTPUB;
extern uint8_t NOTARY_PUBKEY33[33];
extern uint8_t ASSETCHAINS_OVERRIDE_PUBKEY33[33];
extern std::vector<std::string> ASSETCHAINS_PRICES;
extern std::vector<std::string> ASSETCHAINS_STOCKS;

extern int32_t VERUS_BLOCK_POSUNITS;
extern int32_t VERUS_CONSECUTIVE_POS_THRESHOLD;
extern int32_t VERUS_NOPOS_THRESHHOLD;
extern uint256 KOMODO_EARLYTXID;

extern bool IS_KOMODO_TESTNODE;
extern bool IS_KOMODO_DEALERNODE;
extern int32_t KOMODO_CONNECTING;
extern int32_t KOMODO_CCACTIVATE;
extern uint32_t ASSETCHAINS_CC;
extern std::string CCerror;
extern std::string ASSETCHAINS_CCLIB;
extern uint8_t ASSETCHAINS_CCDISABLES[256];

extern std::string NOTARY_PUBKEY;
extern std::string NOTARY_ADDRESS;
extern bool IS_MODE_EXCHANGEWALLET;
extern int32_t VERUS_MIN_STAKEAGE;
extern std::string DONATION_PUBKEY;
extern uint8_t ASSETCHAINS_PRIVATE;
extern char NOTARYADDRS[64][64];
extern char NOTARY_ADDRESSES[NUM_KMD_SEASONS][64][64];
extern bool IS_KOMODO_TESTNODE;
extern int32_t KOMODO_SNAPSHOT_INTERVAL;
extern int32_t STAKED_NOTARY_ID;
extern int32_t STAKED_ERA;
extern int32_t USE_EXTERNAL_PUBKEY;
extern int32_t KOMODO_SNAPSHOT_INTERVAL,STAKED_NOTARY_ID,STAKED_ERA;
extern int32_t ASSETCHAINS_EARLYTXIDCONTRACT;
extern int32_t ASSETCHAINS_STAKED_SPLIT_PERCENTAGE;
extern std::vector<std::string> vWhiteListAddress;
extern std::map <std::int8_t, int32_t> mapHeightEvalActivate;

uint256 Parseuint256(const char *hexstr); // defined in cc/CCutilbits.cpp
void komodo_netevent(std::vector<uint8_t> payload);
