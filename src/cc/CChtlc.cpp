/******************************************************************************
 * Copyright © 2021 The Komodo Core Developers.                               *
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
 ******************************************************************************/
#include "cc/CChtlc.h"

/***
 * Run the script to determine if the conditions are met
 * @param eval the return value
 * @param tx the transaction
 * @param nIn not used
 */
bool HTLC::Validate(Eval *eval, const CTransaction &tx, uint32_t nIn)
{
    eval->Valid();
    return true;
}

bool HTLC::IsMyVin(const CScript &script)
{
    CC *cond;
    if (!(cond = GetCryptoCondition(script)))
        return false;
    // Recurse the CC tree to find asset condition
    auto findEval = [] (CC *cond, CCVisitor visitor) {
        CCcontract_info *contract = static_cast<CCcontract_info*>(visitor.context);
        bool r = cc_typeId(cond) == CC_Eval && cond->codeLength == 1 && cond->code[0] == contract->evalcode;
        // false for a match, true for continue
        return r ? 0 : 1;
    };
    CCVisitor visitor = {findEval, (uint8_t*)"", 0, this};
    bool out =! cc_visit(cond, visitor);
    cc_free(cond);
    return out;
}

/****
 * Run validation on a transaction
 * @param cp the HTLC CryptoCondition contract
 * @param eval stores the results
 * @param tx the transaction to validate
 * @param nIn not used
 */
bool HTLCValidate(CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
    return static_cast<HTLC*>(cp)->Validate(eval, tx, nIn);
}

/***
 * Validate that the script is an HTLC
 */
bool HTLCIsMyVin(const CScript &script)
{
    return HTLC().IsMyVin(script);
}

UniValue HTLC::list()
{
	UniValue result(UniValue::VARR);

	uint256 txid, hashBlock;
	CTransaction vintx; std::vector<uint8_t> origpubkey;
	std::string name, description;

    auto addTokenId = [&](uint256 txid) {
        if (myGetTransaction(txid, vintx, hashBlock) != 0) {
            if (vintx.vout.size() > 0 && DecodeTokenCreateOpRet(vintx.vout[vintx.vout.size() - 1].scriptPubKey, origpubkey, name, description) != 0) {
                result.push_back(txid.GetHex());
            }
        }
    };

	std::vector<uint256> txids;
	SetCCtxids(txids, normaladdr,false, evalcode,zeroid,'c');                      // find by old normal addr marker
   	for (std::vector<uint256>::const_iterator it = txids.begin(); it != txids.end(); it++) 	{
        addTokenId(*it);
	}

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressIndexCCMarker;
    SetCCunspents(addressIndexCCMarker, unspendableCCaddr,true);    // find by burnable validated cc addr marker
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressIndexCCMarker.begin(); it != addressIndexCCMarker.end(); it++) {
        addTokenId(it->first.txhash);
    }

	return(result);
}
