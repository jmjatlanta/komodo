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
#include "cc/CCIncludesMinerFee.h"

/***
 * Validate that the contract's conditions have been met
 * @param eval the return value
 * @param tx the transaction
 * @param nIn not used
 */
bool IncludesMinerFee::Validate(Eval *eval, const CTransaction &tx, uint32_t nIn)
{
    int64_t interest;
    CAmount total = pcoinsTip->GetValueIn(chainActive.LastTip()->GetHeight(),&interest,tx,chainActive.LastTip()->nTime);
    total -= tx.GetValueOut();
    if (total > 0)
    {
        eval->Valid();
        return true;
    }
    eval->Invalid("No fee");
    return false;
}

bool IncludesMinerFee::IsMyVin(const CScript &script)
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
 * @param cp the CryptoCondition contract
 * @param eval stores the results
 * @param tx the transaction to validate
 * @param nIn not used
 */
bool IncludesMinerFeeValidate(CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
    return static_cast<IncludesMinerFee*>(cp)->Validate(eval, tx, nIn);
}

/***
 * Validate that the script is truly this contract
 */
bool IncludesMinerFeeIsMyVin(const CScript &script)
{
    return IncludesMinerFee().IsMyVin(script);
}

void IncludesMinerFee::SetValues(CCcontract_info* in)
{
    in->evalcode = EVAL_INCLMINERFEE;
    in->ismyvin = IncludesMinerFeeIsMyVin;
    in->validate = IncludesMinerFeeValidate;
}

IncludesMinerFee::IncludesMinerFee() : CCcontract_info()
{
    evalcode = EVAL_INCLMINERFEE;
    ismyvin = IncludesMinerFeeIsMyVin;
    validate = IncludesMinerFeeValidate;    
}
