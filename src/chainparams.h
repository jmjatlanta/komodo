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

#define KOMODO_MINDIFF_NBITS 0x200f0f0f

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
};

/*****
 * @brief Handle hardfork triggers (heights/timestamps)
 * @see komodo_hardfork.h
 */
class ChainParams : public CChainParams
{
public:
    virtual ~ChainParams() {}
    /***
     * The following return the height/times at which Seasons start
     * NOTE: KMD is based on height, asset chains use Timestamp
     */
    virtual uint32_t Season1StartHeight() const = 0;
    virtual uint32_t Season1StartTimestamp() const = 0;
    virtual uint32_t Season2StartHeight() const = 0;
    virtual uint32_t Season2StartTimestamp() const = 0;
    virtual uint32_t Season3StartHeight() const = 0;
    virtual uint32_t Season3StartTimestamp() const = 0;
    virtual uint32_t Season4StartHeight() const = 0;
    virtual uint32_t Season4StartTimestamp() const = 0;
    virtual uint32_t Season5StartHeight() const = 0;
    virtual uint32_t Season5StartTimestamp() const = 0;
    virtual uint32_t Season6StartHeight() const = 0;
    virtual uint32_t Season6StartTimestamp() const = 0;
    /****
     * @returns the height when notaries can begin mining at lower difficulty
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
     * @note: Also determines point where PAX uses seed
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
    /*****
     * @return height where komodo_eligiblenotary returns 1 (true) even if the notary has mined in the last 66 blks
     */
    virtual uint32_t NotaryEligibleEvenIfMinedRecently() const = 0;
    /****
     * @return height where komodo_validateinterest begins to work
     */
    virtual uint32_t EnableValidateInterestHeight() const = 0;
    /****
     * @returns below this height, computations within komodo_validateinterest reduce time by 16000
     */
    virtual uint32_t ValidateInterestTrueTimeHeight() const = 0;
    /****
     * @returns height where AdaptivePoW begins to work (see komodo_adaptivepow_target)
     */
    virtual uint32_t AdaptivePoWMinHeight() const = 0;
    /****
     * @returns height at which transactions are considered "Early"
     * @note Notes seem to refer to the HEMP chain, and a scheme to "lock in" a rule prior to this height.
     * @note also seems to be used with a repayment plan
     * @see GetKomodoEarlytxidScriptPub()
     * @see src/cc/prices.cpp
     * @see src/cc/hempcoin_notes.txt
     */
    virtual uint32_t KomodoEarlyTXIDHeight() const = 0;
    /****
     * @returns minimum height to calculate PoW
     * @see komodo_checkPOW()
     */
    virtual uint32_t EnableCheckPoWHeight() const  = 0;
    /*****
     * @returns the minimum height for PoW calc on PoS chain
     * @note (requires a small number of blocks before chain PoW can be calculated)
     */
    virtual uint32_t MinPoWHeight() const = 0;
    /****
     * @returns lower limit where blocks on non-KMD chains should check for the appropriate notary
     * @note this works in conjuntion with KomodoNotaryUpperLimitHeight. There is a gap between these
     * two heights where checking the notary should not be done.
     * @note this is also the height where some komodo_gateway.komodo_check_deposit() logic is activated
     * @see BitcoinMiner()
     * @see komodo_check_deposit()
     */
    virtual uint32_t KomodoNotaryLowerLimitHeight() const = 0;
    /****
     * @returns the upper limit where blocks on non-KMD chains should check fo rthe appropriate notary
     * @see KomodoNotaryLowerLImitHeight() above.
     * @note this is also the height where the komodo gateway is activated
    */
    virtual uint32_t KomodoNotaryUpperLimitHeight() const = 0;
    /*****
     * @returns lower limit in PAX
     */
    virtual uint32_t KomodoPaxLowerLimitHeight() const = 0;
    /****
     * @returns height at which non-z message will be logged
     * @see komodo_gateway.komodo_check_deposit()
     */
    virtual uint32_t KomodoPrintNonZMessageHeight() const = 0;
    /****
     * @return height at which komodo_gateway.komodo_check_deposit() rejects strange vouts
     */
    virtual uint32_t KomodoGatewayRejectStrangeoutHeight() const = 0;
    virtual uint32_t KomodoGatewayPaxHeight() const = 0;
    virtual uint32_t KomodoGatewayPaxMessageLowerHeight() const = 0;
    virtual uint32_t KomodoGatewayPaxMessageUpperHeight() const = 0;
    /*****
     * @return height at which notaries in Notaries_genesis should no longer be used
     */
    virtual uint32_t KomodoNotariesHardcodedHeight() const = 0;
    /***
     * @return height where values are swapped
     * @note fields got switched around due to legacy issues and approves
     * @see komodo_gateway.cpp->komodo_opreturn
     */
    virtual uint32_t KomodoOpReturnSwapHeight() const = 0;
    /*****
     * @returns height where the signed mask changed
     * @see komodo.cpp->komodo_connectblock()
     */
    virtual uint32_t KomodoSignedMaskChangeHeight() const = 0;
    /****
     * @returns height at which minimum to ratify changes from 7 to 11
     */
    virtual uint32_t KomodoMinRatifyHeight() const = 0;
    /***
     * @returns height where interest is calculated
     */
    virtual uint32_t KomodoInterestMinSpendHeight() const = 0;
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
