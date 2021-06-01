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
    static void SetValues(CCcontract_info* in)
    {
        in->evalcode = EVAL_HTLC;
        in->ismyvin = HTLCIsMyVin;
        in->validate = HTLCValidate;
    }
    HTLC() : CCcontract_info()
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
    }
    bool Validate(Eval *eval, const CTransaction &tx, uint32_t nIn);
    bool IsMyVin(const CScript &script);
    UniValue list();
    UniValue info(uint256 tokenid) 
    { 
        return UniValue(UniValue::VNULL); 
    }
};

