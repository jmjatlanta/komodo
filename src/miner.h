// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
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

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"
#include "solver.h"
#include "chainparams.h"
#include <boost/optional.hpp>
#include <stdint.h>

class CBlockIndex;
class CScript;
#ifdef ENABLE_WALLET
class CReserveKey;
class CWallet;
#endif
namespace Consensus { struct Params; };

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOps;
};
#define KOMODO_MAXGPUCOUNT 65

/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(CPubKey _pk,const CScript& scriptPubKeyIn, int32_t gpucount, bool isStake = false);
#ifdef ENABLE_WALLET
boost::optional<CScript> GetMinerScriptPubKey(CReserveKey& reservekey);
CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey, int32_t nHeight, int32_t gpucount, bool isStake = false);
#else
boost::optional<CScript> GetMinerScriptPubKey();
CBlockTemplate* CreateNewBlockWithKey();
#endif

#ifdef ENABLE_MINING

/***
 * Possible return values from mining
 */
enum BlockCreateResult
{
    BCR_CONTINUE, // loop should be restarted
    BCR_BREAK, // loop should be exited
    BCR_RETURN, // mining should be stopped
    BCR_BLOCK_CREATED // mining successful
};

/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);

#ifdef ENABLE_WALLET
/** Run the miner threads */
void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads);

/******
 * @brief attempt to mine one block
 * @param chainparams the chain parameters
 * @param reservekey the reserve key
 * @param gpucount gpu count
 * @param notaryid the notary id
 * @param nExtraNonce nonce
 * @param solver the solver in use
 * @param n
 * @param k
 * @param m_cs protects cancelSolver
 * @param cancelSolver whether the solver process should be cancelled
 * @param pwallet the wallet used for mining
 */
BlockCreateResult MineOneBlock(const CChainParams &chainparams, 
        int32_t &gpucount, int32_t &notaryid,
        unsigned int &nExtraNonce, std::shared_ptr<BlockSolver> solver,
        unsigned int n, unsigned int k, std::mutex &m_cs, bool &cancelSolver
        ,CWallet *pwallet, CReserveKey &reservekey);

#else // (!ENABLE_WALLET)

/** Run the miner threads */
void GenerateBitcoins(bool fGenerate, int nThreads);

/******
 * @brief attempt to mine one block
 * @param chainparams the chain parameters
 * @param reservekey the reserve key
 * @param gpucount gpu count
 * @param notaryid the notary id
 * @param nExtraNonce nonce
 * @param solver the solver in use
 * @param n
 * @param k
 * @param m_cs protects cancelSolver
 * @param cancelSolver whether the solver process should be cancelled
 * @param pwallet the wallet used for mining
 */
BlockCreateResult MineOneBlock(const CChainParams &chainparams, 
        int32_t &gpucount, int32_t &notaryid,
        unsigned int &nExtraNonce, std::shared_ptr<BlockSolver> solver,
        unsigned int n, unsigned int k, std::mutex &m_cs, bool &cancelSolver);
#endif // ENABLE_WALLET
#endif // ENABLE_MINING

void UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

#endif // BITCOIN_MINER_H
