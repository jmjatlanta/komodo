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
#include "komodo_structs.h"

extern std::mutex komodo_mutex;
extern pthread_mutex_t staked_mutex;

extern pax_transaction *PAX;
extern int32_t NUM_PRICES; 
extern uint32_t *PVALS;
extern knotaries_entry *Pubkeys;

extern komodo_state KOMODO_STATES[34]; // state objects. NOTE: order matches CURRENCIES[N-1], with KOMODO_STATES[0] reserved for ASSETCHAINS_SYMBOL.
extern const uint32_t nStakedDecemberHardforkTimestamp; //December 2019 hardfork 12/20/2019 @ 11:06am (UTC)
extern const int32_t nDecemberHardforkHeight;   //December 2019 hardfork

extern const uint32_t nS4Timestamp; //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 03:00:00 PM UTC
extern const int32_t nS4HardforkHeight;   //dPoW Season 4 2020 hardfork Sunday, June 14th, 2020 

extern int COINBASE_MATURITY;
extern unsigned int WITNESS_CACHE_SIZE;
extern uint256 KOMODO_EARLYTXID;

extern int32_t KOMODO_NSPV;
extern int32_t KOMODO_NSPV;
extern int32_t KOMODO_MININGTHREADS;
extern int32_t IS_KOMODO_NOTARY;
extern int32_t IS_STAKED_NOTARY;
extern int32_t USE_EXTERNAL_PUBKEY;
extern int32_t KOMODO_CHOSEN_ONE;
extern int32_t ASSETCHAINS_SEED;
extern int32_t KOMODO_ON_DEMAND;
extern int32_t KOMODO_EXTERNAL_NOTARIES;
extern int32_t KOMODO_PASSPORT_INITDONE;
extern int32_t KOMODO_PAX;
extern int32_t KOMODO_EXCHANGEWALLET;
extern int32_t KOMODO_REWIND;
extern int32_t STAKED_ERA;
extern int32_t KOMODO_CONNECTING;
extern int32_t KOMODO_DEALERNODE;
extern int32_t KOMODO_EXTRASATOSHI;
extern int32_t ASSETCHAINS_FOUNDERS;
extern int32_t ASSETCHAINS_CBMATURITY;
extern int32_t KOMODO_INSYNC;
extern int32_t KOMODO_LASTMINED;
extern int32_t prevKOMODO_LASTMINED;
extern int32_t KOMODO_CCACTIVATE;
extern int32_t KOMODO_DEX_P2P;
extern int32_t JUMBLR_PAUSE;
extern std::string NOTARY_PUBKEY;
extern std::string ASSETCHAINS_NOTARIES;
extern std::string ASSETCHAINS_OVERRIDE_PUBKEY;
extern std::string DONATION_PUBKEY;
extern std::string ASSETCHAINS_SCRIPTPUB;
extern std::string NOTARY_ADDRESS;
extern std::string ASSETCHAINS_SELFIMPORT;
extern std::string ASSETCHAINS_CCLIB;
extern uint8_t NOTARY_PUBKEY33[33];
extern uint8_t ASSETCHAINS_OVERRIDE_PUBKEY33[33];
extern uint8_t ASSETCHAINS_OVERRIDE_PUBKEYHASH[20];
extern uint8_t ASSETCHAINS_PUBLIC;
extern uint8_t ASSETCHAINS_PRIVATE;
extern uint8_t ASSETCHAINS_TXPOW;
extern int8_t ASSETCHAINS_ADAPTIVEPOW;
extern bool VERUS_MINTBLOCKS;
extern std::vector<uint8_t> Mineropret;
extern std::vector<std::string> vWhiteListAddress;
extern char NOTARYADDRS[64][64];
extern char NOTARY_ADDRESSES[NUM_KMD_SEASONS][64][64];
extern char ASSETCHAINS_SYMBOL[KOMODO_ASSETCHAIN_MAXLEN];
extern char ASSETCHAINS_USERPASS[4096];
extern uint16_t ASSETCHAINS_P2PPORT;
extern uint16_t ASSETCHAINS_RPCPORT;
extern uint16_t ASSETCHAINS_BEAMPORT;
extern uint16_t ASSETCHAINS_CODAPORT;
extern uint32_t ASSETCHAIN_INIT;
extern uint32_t ASSETCHAINS_CC;
extern uint32_t KOMODO_STOPAT;
extern uint32_t KOMODO_DPOWCONFS;
extern uint32_t STAKING_MIN_DIFF;
extern uint32_t ASSETCHAINS_MAGIC;
extern int64_t ASSETCHAINS_GENESISTXVAL;
extern int64_t MAX_MONEY;

// consensus variables for coinbase timelock control and timelock transaction support
// time locks are specified enough to enable their use initially to lock specific coinbase transactions for emission control
// to be verifiable, timelocks require additional data that enables them to be validated and their ownership and
// release time determined from the blockchain. to do this, every time locked output according to this
// spec will use an op_return with CLTV at front and anything after |OP_RETURN|PUSH of rest|OPRETTYPE_TIMELOCK|script|
#define _ASSETCHAINS_TIMELOCKOFF 0xffffffffffffffff
extern uint64_t ASSETCHAINS_TIMELOCKGTE;
extern uint64_t ASSETCHAINS_TIMEUNLOCKFROM;
extern uint64_t ASSETCHAINS_TIMEUNLOCKTO;
extern uint64_t ASSETCHAINS_CBOPRET;

extern uint64_t ASSETCHAINS_LASTERA;
extern uint64_t ASSETCHAINS_ENDSUBSIDY[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_REWARD[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_HALVING[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_DECAY[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_NOTARY_PAY[ASSETCHAINS_MAX_ERAS+1];
extern uint64_t ASSETCHAINS_PEGSCCPARAMS[3];
extern uint8_t ASSETCHAINS_CCDISABLES[256];
extern uint8_t ASSETCHAINS_CCZEROTXFEE[256];

extern uint32_t ASSETCHAINS_NUMALGOS;
extern uint32_t ASSETCHAINS_EQUIHASH;
extern uint32_t ASSETCHAINS_VERUSHASH;
extern uint32_t ASSETCHAINS_VERUSHASHV1_1;
extern const char *ASSETCHAINS_ALGORITHMS[3];
extern uint64_t ASSETCHAINS_NONCEMASK[3];
extern uint32_t ASSETCHAINS_NONCESHIFT[3];
extern uint32_t ASSETCHAINS_HASHESPERROUND[3];
extern uint32_t ASSETCHAINS_ALGO;
// min diff returned from GetNextWorkRequired needs to be added here for each algo, so they can work with ac_staked.
extern uint32_t ASSETCHAINS_MINDIFF[3];
                                            // ^ wrong!
// Verus proof of stake controls
extern int32_t ASSETCHAINS_LWMAPOS;        // percentage of blocks should be PoS
extern int32_t VERUS_BLOCK_POSUNITS;    // one block is 1000 units
extern int32_t VERUS_MIN_STAKEAGE;       // 1/2 this should also be a cap on the POS averaging window, or startup could be too easy
extern int32_t VERUS_CONSECUTIVE_POS_THRESHOLD;
extern int32_t VERUS_NOPOS_THRESHHOLD;   // if we have no POS blocks in this many blocks, set to default difficulty

extern int32_t ASSETCHAINS_SAPLING;
extern int32_t ASSETCHAINS_OVERWINTER;

extern uint64_t KOMODO_INTERESTSUM;
extern uint64_t KOMODO_WALLETBALANCE;
extern int32_t ASSETCHAINS_STAKED;
extern uint64_t ASSETCHAINS_COMMISSION;
extern uint64_t ASSETCHAINS_SUPPLY;
extern uint64_t ASSETCHAINS_FOUNDERS_REWARD;

extern uint32_t KOMODO_INITDONE;
extern char KMDUSERPASS[8192+512+1];
extern char BTCUSERPASS[8192]; 
extern uint16_t KMD_PORT;
extern uint16_t BITCOIND_RPCPORT;
extern uint64_t PENDING_KOMODO_TX;
extern int32_t KOMODO_LOADINGBLOCKS;
extern unsigned int MAX_BLOCK_SIGOPS;

extern int32_t KOMODO_TESTNODE, KOMODO_SNAPSHOT_INTERVAL; 
extern CScript KOMODO_EARLYTXID_SCRIPTPUB;
extern int32_t ASSETCHAINS_EARLYTXIDCONTRACT;
extern int32_t ASSETCHAINS_STAKED_SPLIT_PERCENTAGE;

extern std::map <std::int8_t, int32_t> mapHeightEvalActivate;

extern komodo_kv *KOMODO_KV;
extern pthread_mutex_t KOMODO_KV_mutex;
extern pthread_mutex_t KOMODO_CC_mutex;

#define MAX_CURRENCIES 32
extern char CURRENCIES[33][8]; // 33 currencies, KMD at index 32 

/****
 * Get the index of the currency
 * NOTE: This is a linear search
 * @param origbase the currency to look for
 * @returns the index of the currency in the CURRENCIES array, or -1 if not found
 */
int32_t komodo_baseid(char *origbase);

uint64_t komodo_current_supply(uint32_t nHeight);