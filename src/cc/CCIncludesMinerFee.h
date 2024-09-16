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

/***
 * This contract verifies the transaction contains a fee
 */
struct IncludesMinerFee : public CCcontract_info
{
    /***
     * Set up a raw CCcontract_info struct to be this contract
     * @param in the struct to fill
     */ 
    static void SetValues(CCcontract_info* in);

    /**
     * ctor
     */
    IncludesMinerFee();

    /***
     * Validate
     * @param eval the return value
     * @param tx the incoming transaction
     * @param nIn not used
     * @returns true if validation passes
     */
    bool Validate(Eval *eval, const CTransaction &tx, uint32_t nIn);

    /***
     * Check if this script references an HTLC
     * @param script the script
     * @returns true if this is references an HTLC
     */
    bool IsMyVin(const CScript &script);

};

