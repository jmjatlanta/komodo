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

#ifndef BITCOIN_TXMEMPOOL_H
#define BITCOIN_TXMEMPOOL_H

#include <list>

#include "addressindex.h"
#include "spentindex.h"
#include "amount.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "sync.h"

#undef foreach
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/ordered_index.hpp"

class CAutoFile;

inline double AllowFreeThreshold()
{
    return COIN * 144 / 250;
}

inline bool AllowFree(double dPriority)
{
    // Large (in bytes) low-priority (new, small-coin) transactions
    // need a fee.
    return dPriority > AllowFreeThreshold();
}

/** Fake height value used in CCoins to signify they are only in the memory pool (since 0.8) */
static const unsigned int MEMPOOL_HEIGHT = 0x7FFFFFFF;

/**
 * CTxMemPool stores these:
 */
class CTxMemPoolEntry
{
private:
    CTransaction tx;
    CAmount nFee; //! Cached to avoid expensive parent-transaction lookups
    size_t nTxSize; //! ... and avoid recomputing tx size
    size_t nModSize; //! ... and modified size for priority
    size_t nUsageSize; //! ... and total memory usage
    CFeeRate feeRate; //! ... and fee per kB
    int64_t nTime; //! Local time when entering the mempool
    double dPriority; //! Priority when entering the mempool
    unsigned int nHeight; //! Chain height when entering the mempool
    bool hadNoDependencies; //! Not dependent on any other txs when it entered the mempool
    bool spendsCoinbase; //! keep track of transactions that spend a coinbase
    uint32_t nBranchId; //! Branch ID this transaction is known to commit to, cached for efficiency

public:
    CTxMemPoolEntry(const CTransaction& _tx, const CAmount& _nFee,
                    int64_t _nTime, double _dPriority, unsigned int _nHeight,
                    bool poolHasNoInputsOf, bool spendsCoinbase, uint32_t nBranchId);
    CTxMemPoolEntry();
    CTxMemPoolEntry(const CTxMemPoolEntry& other);

    const CTransaction& GetTx() const { return this->tx; }
    double GetPriority(unsigned int currentHeight) const;
    CAmount GetFee() const { return nFee; }
    CFeeRate GetFeeRate() const { return feeRate; }
    size_t GetTxSize() const { return nTxSize; }
    int64_t GetTime() const { return nTime; }
    unsigned int GetHeight() const { return nHeight; }
    bool WasClearAtEntry() const { return hadNoDependencies; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }

    bool GetSpendsCoinbase() const { return spendsCoinbase; }
    uint32_t GetValidatedBranchId() const { return nBranchId; }
};

// extracts a TxMemPoolEntry's transaction hash
struct mempoolentry_txid
{
    typedef uint256 result_type;
    result_type operator() (const CTxMemPoolEntry &entry) const
    {
        return entry.GetTx().GetHash();
    }
};

class CompareTxMemPoolEntryByFee
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b)
    {
        if (a.GetFeeRate() == b.GetFeeRate())
            return a.GetTime() < b.GetTime();
        return a.GetFeeRate() > b.GetFeeRate();
    }
};

class CBlockPolicyEstimator;

/** An inpoint - a combination of a transaction and an index n into its vin */
class CInPoint
{
public:
    const CTransaction* ptx;
    uint32_t n;

    CInPoint() { SetNull(); }
    CInPoint(const CTransaction* ptxIn, uint32_t nIn) { ptx = ptxIn; n = nIn; }
    void SetNull() { ptx = NULL; n = (uint32_t) -1; }
    bool IsNull() const { return (ptx == NULL && n == (uint32_t) -1); }
    size_t DynamicMemoryUsage() const { return 0; }
};

/**
 * CTxMemPool stores valid-according-to-the-current-best-chain
 * transactions that may be included in the next block.
 *
 * Transactions are added when they are seen on the network
 * (or created by the local node), but not all transactions seen
 * are added to the pool: if a new transaction double-spends
 * an input of a transaction in the pool, it is dropped,
 * as are non-standard transactions.
 */
class CTxMemPool
{
private:
    uint32_t nCheckFrequency; //! Value n means that n times in 2^32 we check.
    unsigned int nTransactionsUpdated;
    CBlockPolicyEstimator* minerPolicyEstimator;

    uint64_t totalTxSize = 0; //! sum of all mempool tx' byte sizes
    uint64_t cachedInnerUsage; //! sum of dynamic memory usage of all the map elements (NOT the maps themselves)

    std::map<uint256, const CTransaction*> mapRecentlyAddedTx;
    uint64_t nRecentlyAddedSequence = 0;
    uint64_t nNotifiedSequence = 0;

    std::map<uint256, const CTransaction*> mapSproutNullifiers;
    std::map<uint256, const CTransaction*> mapSaplingNullifiers;

    /*****
     * @brief check internal collections for validity
     * @param type which collection to check (sprout or sapling)
     */
    void checkNullifiers(ShieldedType type) const;
    
public:
    typedef boost::multi_index_container<
        CTxMemPoolEntry,
        boost::multi_index::indexed_by<
            // sorted by txid
            boost::multi_index::ordered_unique<mempoolentry_txid>,
            // sorted by fee rate
            boost::multi_index::ordered_non_unique<
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByFee
            >
        >
    > indexed_transaction_set;

    mutable CCriticalSection cs;
    indexed_transaction_set mapTx;

private:
    typedef std::map<CMempoolAddressDeltaKey, CMempoolAddressDelta, CMempoolAddressDeltaKeyCompare> addressDeltaMap;
    addressDeltaMap mapAddress;

    typedef std::map<uint256, std::vector<CMempoolAddressDeltaKey> > addressDeltaMapInserted;
    addressDeltaMapInserted mapAddressInserted;

    typedef std::map<CSpentIndexKey, CSpentIndexValue, CSpentIndexKeyCompare> mapSpentIndex;
    mapSpentIndex mapSpent;

    typedef std::map<uint256, std::vector<CSpentIndexKey> > mapSpentIndexInserted;
    mapSpentIndexInserted mapSpentInserted;

public:
    std::map<COutPoint, CInPoint> mapNextTx;
    std::map<uint256, std::pair<double, CAmount> > mapDeltas;

    CTxMemPool(const CFeeRate& _minRelayFee);
    ~CTxMemPool();

    /**
     * @brief Makes sure the pool is consistent (does not contain 
     * two transactions that spend the same inputs, all inputs are in the mapNextTx array).
     * @note If sanity-checking is turned off, this does nothing.
     * @param pcoins
     */
    void check(const CCoinsViewCache *pcoins) const;
    /*****
     * @brief set the frequency of the sanity check
     * @param dFrequency the new frequency (0 = never)
     */
    void setSanityCheck(double dFrequency = 1.0) { nCheckFrequency = static_cast<uint32_t>(dFrequency * 4294967295.0); }

    /****
     * @brief add a tx to the memory pool without checks
     * @param hash the tx hash
     * @param entry the tx
     * @param fCurrentEstimate true to run fee estimate logic during processing
     * @returns true always
     */
    bool addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry, bool fCurrentEstimate = true);
    /****
     * @brief add to address index
     * @param entry what to add
     * @param view the current view (to get the previous outputs of tx's inputs)
     */
    void addAddressIndex(const CTxMemPoolEntry &entry, const CCoinsViewCache &view);
    /*****
     * @brief retrieve address indexes
     * @param addresses the search criteria
     * @param results the results
     * @returns always true
     */
    bool getAddressIndex(std::vector<std::pair<uint160, int> > &addresses,
                         std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > &results);
    /*****
     * @brief remove an address index
     * @param txhash the hash to search for
     * @returns always true
     */
    bool removeAddressIndex(const uint256 txhash);
    /****
     * @brief add an entry to the spent index
     * @param entry what to add
     * @param view the view
     */
    void addSpentIndex(const CTxMemPoolEntry &entry, const CCoinsViewCache &view);
    /****
     * @brief Retrieve an entry from the spent index
     * @param key what to search for
     * @param value the results
     * @returns true if key found
     */
    bool getSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value);
    /****
     * @brief remove an entry from the spent index
     * @param txhash what to remove
     * @returns always true
     */
    bool removeSpentIndex(const uint256 txhash);
    /******
     * @brief remove a transaction from the memory pool
     * @param origTx the transaction to remove
     * @param removed what was removed (origTx could have had children)
     * @param fRecursive true to look for children
     */
    void remove(const CTransaction &tx, std::list<CTransaction>& removed, bool fRecursive = false);
    /******
     * @brief remove a transaction and all related txs
     * @param invalidRoot the tx to remove + txs with this as anchor
     * @param type SPROUT or SAPLING
     */
    void removeWithAnchor(const uint256 &invalidRoot, ShieldedType type);
    /*****
     * @brief Remove transactions due to a reorg
     * @param pcoins
     * @param nMemPoolHeight
     * @param flags
     */
    void removeForReorg(const CCoinsViewCache *pcoins, unsigned int nMemPoolHeight, int flags);
    /*****
     * @brief remove txs that conflict with a tx
     * @param tx the transaction
     * @param[out] removed what was removed
     */
    void removeConflicts(const CTransaction &tx, std::list<CTransaction>& removed);
    /****
     * @brief remove entries that have expired
     * @param nBlockHeight the chain height
     */
    void removeExpired(unsigned int nBlockHeight);
    /****
     * @brief Remove txs due to a block being connected
     * @note Also updates the miner fee estimator.
     * @param vtx
     * @param nBlockHeight
     * @param conflicts
     * @param fCurrentEstimate
     */
    void removeForBlock(const std::vector<CTransaction>& vtx, unsigned int nBlockHeight,
                        std::list<CTransaction>& conflicts, bool fCurrentEstimate = true);
    /****
     * @brief Removes transactions which don't commit to
     * the given branch ID from the mempool.
     * @note Called whenever the tip changes. 
     * @param nMemPoolBranchId the branch id to keep
     */
    void removeWithoutBranchId(uint32_t nMemPoolBranchId);
    /****
     * @brief cleans out this mempool
     */
    void clear();
    /****
     * @brief Retrieve a collection of tx hashes in the mempool
     * @param[out] vtxid the results
     */
    void queryHashes(std::vector<uint256>& vtxid);
    /****
     * @brief "spend" coins related to hash
     * @param hashTx the hash to look for
     * @param coins the coins db to "spend" from
     */
    void pruneSpent(const uint256& hash, CCoins &coins);
    /****
     * @returns number of transactions updated
     */
    unsigned int GetTransactionsUpdated() const;
    /*****
     * @brief adds to the number of transactions updated
     * @param n the number to add
     */
    void AddTransactionsUpdated(unsigned int n);
    /**
     * Check that none of this transactions inputs are in the mempool, and thus
     * the tx is not dependent on other mempool transactions to be included in a block.
     */
    bool HasNoInputsOf(const CTransaction& tx) const;

    /******
     * @brief Affect CreateNewBlock prioritisation of transactions 
     * @param hash the transaction
     * @param strHash the hash as a string (used for debugging message)
     * @param dPriorityDelta the priority delta
     * @param nFeeDelta the fee delta
     */
    void PrioritiseTransaction(const uint256 hash, const std::string strHash, double dPriorityDelta, 
            const CAmount& nFeeDelta);
    /*****
     * @brief add priority to an existing transaction
     * @param hash the tx to look for
     * @param dPriorityDelta the priority delta
     * @param nFeeDelta the fee delta
     */
    void ApplyDeltas(const uint256 hash, double &dPriorityDelta, CAmount &nFeeDelta);
    /****
     * @brief remove prioritization data from transaction
     * @param hash the tx to look for
     */
    void ClearPrioritisation(const uint256 hash);
    /****
     * @brief determine if a nullifier exists
     * @param nullifier what to look for
     * @param type SPROUT or SAPLING
     * @returns true if found
     */
    bool nullifierExists(const uint256& nullifier, ShieldedType type) const;
    /*****
     * @brief notify wallets of recently added transactions
     */
    void NotifyRecentlyAdded();
    /****
     * @returns true if wallets have been notified (REGTEST only)
     */
    bool IsFullyNotified();
    /****
     * @returns number of items in mempool
     */
    unsigned long size()
    {
        LOCK(cs);
        return mapTx.size();
    }

    /****
     * @returns total transaction size of all txs in this pool
     */
    uint64_t GetTotalTxSize()
    {
        LOCK(cs);
        return totalTxSize;
    }

    /****
     * @param hash what to look for
     * @returns true if tx exists in mempool
     */
    bool exists(uint256 hash) const
    {
        LOCK(cs);
        return (mapTx.count(hash) != 0);
    }

    /******
     * @brief find a transaction by its hash
     * @param hash what to look for
     * @param result the result
     * @returns true if found
     */
    bool lookup(uint256 hash, CTransaction& result) const;

    /**
     * @brief Estimate fee needed to get into next nBlocks blocks
     * @param nBlocks number of blocks
     * @returns fee estimate to get into next nBlocks blocks
     */
    CFeeRate estimateFee(int nBlocks) const;

    /** 
     * @brief Estimate priority needed to get into the next nBlocks 
     * @param nBlocks number of blocks
     * @returns priority estimate to get into the next nBlocks blocks
     */
    double estimatePriority(int nBlocks) const;
    
    /** 
     * @brief Write estimates to disk 
     * @param fileout the file
     * @returns true
     */
    bool WriteFeeEstimates(CAutoFile& fileout) const;
    /****
     * @brief read estimates from disk
     * @param filein the file
     * @returns true
     */
    bool ReadFeeEstimates(CAutoFile& filein);

    /****
     * @returns the estimated overhead of this pool
     */
    size_t DynamicMemoryUsage() const;

    /** 
     * @returns current check frequency setting 
     */
    uint32_t GetCheckFrequency() const {
        return nCheckFrequency;
    }
};

/** 
 * CCoinsView that brings transactions from a memorypool into view.
 * It does not check for spendings by memory pool transactions.
 */
class CCoinsViewMemPool : public CCoinsViewBacked
{
protected:
    CTxMemPool &mempool;

public:
    CCoinsViewMemPool(CCoinsView *baseIn, CTxMemPool &mempoolIn);
    bool GetNullifier(const uint256 &txid, ShieldedType type) const;
    bool GetCoins(const uint256 &txid, CCoins &coins) const;
    bool HaveCoins(const uint256 &txid) const;
};

#endif // BITCOIN_TXMEMPOOL_H
