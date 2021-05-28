/****
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
 ******/
#pragma once
#include "CCinclude.h"

/****
 * Run validation on a transaction
 * @param cp the HTLC CryptoCondition contract
 * @param eval stores the results
 * @param tx the transactoin
 * @param nIn not used
 */
bool HTLCValidate(CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn);
bool HTLCIsMyVin(const CScript &script);

/***
 * This contract wraps HTLC functionality into a CryptoCondition
 */
struct HTLC : public CCcontract_info
{
    HTLC() : CCcontract_info()
    {
        // NOTE: This is never called (yet, refactoring in the future)
    }
    bool Validate(Eval *eval, const CTransaction &tx, uint32_t nIn);
    bool IsMyVin(const CScript &script);
    UniValue list();
    UniValue info(uint256 tokenid) 
    { 
        return UniValue(UniValue::VNULL); 
    }
};

