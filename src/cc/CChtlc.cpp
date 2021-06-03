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
 * Validate that the contract's conditions have been met
 * @param eval the return value
 * @param tx the transaction
 * @param nIn not used
 */
bool HTLC::Validate(Eval *eval, const CTransaction &tx, uint32_t nIn)
{
    std::vector<CTxIn> ins = tx.vin;
    for(auto in : tx.vin)
    {
        // Do we have the preimage?
        // Has time expired?
    }
    eval->Valid();
    return true;
}

bool HTLC::IsMyVin(const CScript &script)
{
    CC *cond = GetCryptoCondition(script);
    if (!cond)
        return false;
    // Recurse the CC tree to find condition
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

void HTLC::SetValues(CCcontract_info* in)
{
    in->evalcode = EVAL_HTLC;
    in->ismyvin = HTLCIsMyVin;
    in->validate = HTLCValidate;
}

HTLC::HTLC() : CCcontract_info()
{
    evalcode = EVAL_HTLC;
    /*
    strcpy(this->unspendableCCaddr, "REGFHStKZrUQLT3G3AnJYiiC3Cc19uZVXp");
    strcpy(this->normaladdr, "RNCkPLHQ79fXnwrfrW5cfNkAxXztKrveRB");
    strcpy(this->CChexstr, "0264b45b5f8ce902491a332a2a5652502a9f5db4b3529792759be7f58e34985640");
    uint8_t HTLCCCpriv[32] = {0x15, 0x8c, 0x13, 0xa6, 0xdf, 0x1d, 0x62, 0x9a, 0x8b, 0x4d, 0xc3, 
                    0xcb, 0xea, 0x54, 0xfa, 0xf0, 0xa7, 0x4f, 0x9b, 0x3f, 0xe0, 0xd1, 0x24, 0xf5,
                    0x71, 0xf3, 0xc9, 0x77, 0x93, 0x8c, 0xec, 0xe1 };
    memcpy(this->CCpriv, HTLCCCpriv, 32);
    */
    ismyvin = HTLCIsMyVin;
    validate = HTLCValidate;    
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
