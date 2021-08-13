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

#include "cc/eval.h" // TxProof

enum CrosschainType {
    CROSSCHAIN_KOMODO = 1,
    CROSSCHAIN_TXSCL = 2,
    CROSSCHAIN_STAKED = 3
};

typedef struct CrosschainAuthority {
    uint8_t notaries[64][33];
    int8_t size;
    int8_t requiredSigs;
} CrosschainAuthority;

/***
 * @brief determine the chain type
 * @param symbol the symbol for the chain
 * @returns the chain type
 */
CrosschainType GetSymbolAuthority(const char* symbol);

/*****
 * @brief Verify the tx notarization is valid
 * @param tx the transaction
 * @param auth the authorities for the chain
 * @returns true on success
 */
bool CheckTxAuthority(const CTransaction &tx, CrosschainAuthority auth);

/***********************************************************************************
 * The following methods are used on the asset chain (as opposed to the KMD chain) *
 ***********************************************************************************/

/***************
 * @brief given a transaction, find its proof
 * @param hash the transaction
 * @param burnTx not used
 * @returns the proof
 */
TxProof GetAssetchainProof(uint256 hash,CTransaction burnTx);

/***************
 * @brief check a notarization
 * @param kmdNotarisationHash the hash
 * @param momom The Merkle Root of Merkle Root of Merkle Root
 * @returns true on success
 */
bool CheckMoMoM(uint256 kmdNotarisationHash, uint256 momom);

/**************************
* @brief Check notaries approvals for the txoutproofs of burn tx
* (alternate check if MoMoM check has failed)
* @param burntxid txid of burn tx on the source chain
* @param notaryTxidsarray collection of txids of notaries' proofs
* @returns true on success
*/
bool CheckNotariesApproval(uint256 burntxid, const std::vector<uint256> & notaryTxids);

/***********************************************************************************
 * The following methods are used on the KMD chain (as opposed to the asset chain) *
 ***********************************************************************************/

/*********************
 * @brief calculate the proof root of a given asset chain
 * @param[in] symbol the asset chain
 * @param[in] targetCCid the target ccid
 * @param[in] kmdHeight the upper limit of blocks to scan
 * @param[out] moms the MoMs found
 * @param[out] destNotarisationTxid the notarization from the asset chain
 * @returns the merkle root
 */
uint256 CalculateProofRoot(const char* symbol, uint32_t targetCCid, int kmdHeight,
        std::vector<uint256> &moms, uint256 &destNotarisationTxid);

/****************
 * @brief given a proof from an assetchain A, find the proof root on B that includes A
 * @param txid
 * @param targetSymbol asset chain symbol
 * @param targetCCid
 * @param assetChainProof
 * @param offset
 * @returns the proof root
 */
TxProof GetCrossChainProof(const uint256 txid, const char* targetSymbol, uint32_t targetCCid,
        const TxProof assetChainProof,int32_t offset);

/***************************
 * @brief Takes an importTx that has proof leading to assetchain root
 * and extends proof to cross chain root
 * @param importTx
 * @param offset
 */
void CompleteImportTransaction(CTransaction &importTx,int32_t offset);
