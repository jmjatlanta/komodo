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
#include "chainparams.h"
#include "sync.h" // LOCK()
#include "consensus/upgrades.h" // NextEpoch()
#include "chain_status.h"

extern CCriticalSection cs_main;

/***
 * ctor
 */
ChainStatus::ChainStatus(CChain* activeChain, const CChainParams& chainParams)
        : activeChain(activeChain), chainParams(chainParams)
{
}

const CChainParams& ChainStatus::Params()
{
    return chainParams;
}

/***
 * Get the next protocol version based on current chain height
 * iif the upgrade is soon
 * @returns the next protocol version or 0
 */
int ChainStatus::NextProtocolVersion(uint32_t preferredUpgradePeriod)
{
    int height;
    {
        LOCK(cs_main);
        height = activeChain->Height();
    }

    const Consensus::Params& params = Params().GetConsensus();
    auto nextEpoch = NextEpoch(height, params);
    if (nextEpoch) {
        auto idx = nextEpoch.get();
        int nActivationHeight = params.vUpgrades[idx].nActivationHeight;

        if (nActivationHeight > 0 &&
            height < nActivationHeight &&
            height >= nActivationHeight - preferredUpgradePeriod)
        {
            return params.vUpgrades[idx].nProtocolVersion;
        }
    }
    return 0;
}