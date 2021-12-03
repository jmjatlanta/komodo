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

#ifndef CROSSCHAIN_H
#define CROSSCHAIN_H

#include "uint256.h"
#include "serialize.h"
#include "primitives/transaction.h"

#include <cstdint>
#include <vector>

const int CROSSCHAIN_KOMODO = 1;
const int CROSSCHAIN_TXSCL = 2;
const int CROSSCHAIN_STAKED = 3;

typedef struct CrosschainAuthority {
    uint8_t notaries[64][33];
    int8_t size;
    int8_t requiredSigs;
} CrosschainAuthority;

/*
 * Merkle stuff
 */
uint256 SafeCheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex);
uint256 GetMerkleRoot(const std::vector<uint256>& vLeaves);

class MerkleBranch
{
public:
    int nIndex;
    std::vector<uint256> branch;

    MerkleBranch() {}
    MerkleBranch(int i, std::vector<uint256> b) : nIndex(i), branch(b) {}
    uint256 Exec(uint256 hash) const { return SafeCheckMerkleBranch(hash, branch, nIndex); }

    MerkleBranch& operator<<(MerkleBranch append)
    {
        nIndex += append.nIndex << branch.size();
        branch.insert(branch.end(), append.branch.begin(), append.branch.end());
        return *this;
    }

    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(nIndex));
        READWRITE(branch);
    }
};

typedef std::pair<uint256,MerkleBranch> TxProof;

int GetSymbolAuthority(const char* symbol);

/* On assetchain */
TxProof GetAssetchainProof(uint256 hash,CTransaction burnTx);

/* On KMD */
uint256 CalculateProofRoot(const char* symbol, uint32_t targetCCid, int kmdHeight,
        std::vector<uint256> &moms, uint256 &destNotarisationTxid);
TxProof GetCrossChainProof(const uint256 txid, const char* targetSymbol, uint32_t targetCCid,
        const TxProof assetChainProof,int32_t offset);
void CompleteImportTransaction(CTransaction &importTx,int32_t offset);

/* On assetchain */
bool CheckMoMoM(uint256 kmdNotarisationHash, uint256 momom);
bool CheckNotariesApproval(uint256 burntxid, const std::vector<uint256> & notaryTxids);

#endif /* CROSSCHAIN_H */
