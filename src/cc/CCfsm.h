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

#include "CCinclude.h"

std::string FSMList();
std::string FSMInfo(uint256 fsmtxid);
std::string FSMCreate(uint64_t txfee,std::string name,std::string states);

struct CCFSMContract_info : public CCcontract_info
{
    CCFSMContract_info() : CCcontract_info()
    {
        evalcode = EVAL_FSM;
        strcpy(unspendableCCaddr, "RUKTbLBeKgHkm3Ss4hKZP3ikuLW1xx7B2x");
        strcpy(normaladdr, "RWSHRbxnJYLvDjpcQ2i8MekgP6h2ctTKaj");
        strcpy(CChexstr, "039b52d294b413b07f3643c1a28c5467901a76562d8b39a785910ae0a0f3043810");
        uint8_t FSMCCpriv[32] = { 0x11, 0xe1, 0xea, 0x3e, 0xdb, 0x36, 0xf0, 0xa8, 0xc6, 0x34, 
                0xe1, 0x21, 0xb8, 0x02, 0xb9, 0x4b, 0x12, 0x37, 0x8f, 0xa0, 0x86, 0x23, 0x50, 
                0xb2, 0x5f, 0xe4, 0xe7, 0x36, 0x0f, 0xda, 0xae, 0xfc };
        memcpy(CCpriv, FSMCCpriv, 32); 
    }
    virtual bool validate(Eval* eval, const CTransaction &tx, uint32_t nIn) override;
};
