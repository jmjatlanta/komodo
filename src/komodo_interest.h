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

#include "komodo_defs.h"

#define KOMODO_ENDOFERA 7777777
#define KOMODO_INTEREST ((uint64_t)5000000) //((uint64_t)(0.05 * COIN))   // 5%

uint64_t komodo_interestnew(int32_t txheight,uint64_t nValue,uint32_t nLockTime,uint32_t tiptime);

uint64_t komodo_interest(int32_t txheight,uint64_t nValue,uint32_t nLockTime,uint32_t tiptime);
