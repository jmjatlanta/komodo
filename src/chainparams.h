// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#ifndef BITCOIN_CHAINPARAMS_H
#define BITCOIN_CHAINPARAMS_H

#include "chainparamsbase.h"
#include "consensus/params.h"
#include "primitives/block.h"
#include "protocol.h"

#include <vector>

struct CDNSSeedData {
    std::string name, host;
    CDNSSeedData(const std::string &strName, const std::string &strHost) : name(strName), host(strHost) {}
};

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Bitcoin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        ZCPAYMENT_ADDRRESS,
        ZCSPENDING_KEY,
        ZCVIEWING_KEY,

        MAX_BASE58_TYPES
    };
    struct CCheckpointData {
        MapCheckpoints mapCheckpoints;
        int64_t nTimeLastCheckpoint;
        int64_t nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    enum Bech32Type {
        SAPLING_PAYMENT_ADDRESS,
        SAPLING_FULL_VIEWING_KEY,
        SAPLING_INCOMING_VIEWING_KEY,
        SAPLING_EXTENDED_SPEND_KEY,

        MAX_BECH32_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    const std::vector<unsigned char>& AlertKey() const { return vAlertPubKey; }
    int GetDefaultPort() const { return nDefaultPort; }

    const CBlock& GenesisBlock() const { return genesis; }
    /** Make miner wait to have peers to avoid wasting work */
    bool MiningRequiresPeers() const { return fMiningRequiresPeers; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    int64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    unsigned int EquihashN() const { return nEquihashN; }
    unsigned int EquihashK() const { return nEquihashK; }
    std::string CurrencyUnits() const { return strCurrencyUnits; }
    uint32_t BIP44CoinType() const { return bip44CoinType; }
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** In the future use NetworkIDString() for RPC fields */
    bool TestnetToBeDeprecatedFieldRPC() const { return fTestnetToBeDeprecatedFieldRPC; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP(Bech32Type type) const { return bech32HRPs[type]; }
    const std::vector<SeedSpec6>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    /** Return the founder's reward address and script for a given block height */
    std::string GetFoundersRewardAddressAtHeight(int height) const;
    CScript GetFoundersRewardScriptAtHeight(int height) const;
    std::string GetFoundersRewardAddressAtIndex(int i) const;
    /** Enforce coinbase consensus rule in regtest mode */
    void SetRegTestCoinbaseMustBeProtected() { consensus.fCoinbaseMustBeProtected = true; }

    void SetDefaultPort(uint16_t port) { nDefaultPort = port; }
    void SetCheckpointData(CCheckpointData checkpointData);
    void SetNValue(uint64_t n) { nEquihashN = n; }
    void SetKValue(uint64_t k) { nEquihashK = k; }
    void SetMiningRequiresPeers(bool flag) { fMiningRequiresPeers = flag; }
    uint32_t CoinbaseMaturity() const { return coinbaseMaturity; }
    void SetCoinbaseMaturity(uint32_t in) const { coinbaseMaturity = in; }
    void ResetCoinbaseMaturity() const { coinbaseMaturity = originalCoinbaseMaturity; }

    //void setnonce(uint32_t nonce) { memcpy(&genesis.nNonce,&nonce,sizeof(nonce)); }
    //void settimestamp(uint32_t timestamp) { genesis.nTime = timestamp; }
    //void setgenesis(CBlock &block) { genesis = block; }
    //void recalc_genesis(uint32_t nonce) { genesis = CreateGenesisBlock(ASSETCHAINS_TIMESTAMP, nonce, GENESIS_NBITS, 1, COIN); };
    CMessageHeader::MessageStartChars pchMessageStart; // jl777 moved
    Consensus::Params consensus;

protected:
    CChainParams() {}

     //! Raw pub key bytes for the broadcast alert signing key.
    std::vector<unsigned char> vAlertPubKey;
    int nMinerThreads = 0;
    long nMaxTipAge = 0;
    int nDefaultPort = 0;
    uint64_t nPruneAfterHeight = 0;
    unsigned int nEquihashN = 0;
    unsigned int nEquihashK = 0;
    std::vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string bech32HRPs[MAX_BECH32_TYPES];
    std::string strNetworkID;
    std::string strCurrencyUnits;
    uint32_t bip44CoinType;
    CBlock genesis;
    std::vector<SeedSpec6> vFixedSeeds;
    bool fMiningRequiresPeers = false;
    bool fDefaultConsistencyChecks = false;
    bool fRequireStandard = false;
    bool fMineBlocksOnDemand = false;
    bool fTestnetToBeDeprecatedFieldRPC = false;
    CCheckpointData checkpointData;
    std::vector<std::string> vFoundersRewardAddress;
    mutable uint32_t coinbaseMaturity = 100;
    uint32_t originalCoinbaseMaturity = 100;
};

class ChainParams : public CChainParams
{
public:
    /***
     * @brief height at which hard-coded genesis notaries are no longer used
     */
    virtual uint32_t KomodoNotariesHardcoded() const = 0;
    /****
     * @brief start (height) of season 1
     */
    virtual int32_t S1HardforkHeight() const = 0;
    /****
     * @brief start (height) of season 2
     */
    virtual int32_t S2HardforkHeight() const = 0;
    virtual int32_t DecemberHardforkHeight() const = 0;
    virtual uint32_t StakedDecemberHardforkTimestamp() const = 0;
    virtual uint32_t S4Timestamp() const = 0;
    virtual int32_t S4HardforkHeight() const = 0;
    virtual uint32_t S5Timestamp() const = 0;
    virtual int32_t S5HardforkHeight() const = 0;
    /****
     * @returns the height when notaries receive the privilege of mining at lower difficulty
     */
    virtual int32_t NotaryLowerDifficultyStartHeight() const = 0;
    /***
     * @return the height at which notaries are no longer able to mine multiple blocks per round
     */
    virtual int32_t NotaryLimitRepeatHeight() const = 0;
    /**
     * @return the height for using special notary logic
     */
    virtual int32_t NotarySpecialStartHeight() const = 0;
    /****
     * @returns height at which komodo_is_special could return -2
     */
    virtual int32_t NotarySpecialTimeTooShortHeight() const = 0;
    /****
     * @returns the height where number of notaries moved from 64 to 66
     */
    virtual int32_t NotaryMovedTo66() const = 0;
    /***
     * @note: NotaryLowerDifficultyStartHeight overrides some of the logic that relies on ths value
     * @returns height at which notaries were not able to mine more than 1 block recently
     */
    virtual int32_t NotaryOncePerCycle() const = 0;
    /***
     * @return the height when Special and Special 2 start to be evaluated
     */
    virtual int32_t NotarySAndS2StartHeight() const = 0;
    /***
     * @return the height when Special stops and Special 2 starts to be evaluated
     */
    virtual int32_t NotaryS2StartHeight() const = 0;
    /****
     * @return the height when flag is set based on special 2, taking into account election gap
     */
    virtual int32_t NotaryS2IncludeElectionGapHeight() const = 0;
    /****
     * @return height when election gap no longer affects flag
     */
    virtual int32_t NotaryElectionGapOverrideHeight() const = 0;
    /****
     * @return a special height where flag is set regardless of special values
     */
    virtual int32_t NotarySpecialFlagHeight() const = 0;
};

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const ChainParams &Params();

/** Return parameters for the given network. */
ChainParams &Params(CBaseChainParams::Network network);

/** Sets the params returned by Params() to those for the given network. */
void SelectParams(CBaseChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectParamsFromCommandLine();

/**
 * Allows modifying the network upgrade regtest parameters.
 */
void UpdateNetworkUpgradeParameters(Consensus::UpgradeIndex idx, int nActivationHeight);

#endif // BITCOIN_CHAINPARAMS_H
