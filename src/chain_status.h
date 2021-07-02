#pragma once
/******************************************************************************
 * Copyright © 2021 The Komodo Core Developers.                               *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo software, including this file may be copied, modified, propagated   *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#include "chain.h"

class CChainParams;

class ChainStatus
{
public:
    ChainStatus(CChain* activeChain, const CChainParams& chainParams);
    /***
     * Get the next protocol version based on current chain height
     * iif the upgrade is soon
     * @param preferredUpgradePeriod period (in blocks) to prefer clients that have upgraded
     * @returns the next protocol version or 0
     */
    int NextProtocolVersion(uint32_t preferredUpgradePeriod);
    const CChainParams& Params();
private:
    CChain* activeChain = nullptr;
    const CChainParams& chainParams;
};
