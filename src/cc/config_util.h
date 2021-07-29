/******************************************************************************
 * Copyright © 2021 Komodo Core developers                                    *
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
#ifndef __CC_CONFIG_UTIL__
#define __CC_CONFIG_UTIL__

/************
 * A few CC .c routines use this (rogue and dapp), so now they're in one place
 * Uses the C++ utilities for reading the config file into a c-compatible struct
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpc_info
{
    char username[512];
    char password[512];
    char ipaddress[100];
    uint32_t port;
};

/****
 * @brief Read the RPC info from a config file
 * @note if symbols is "KMD" the file "komodo.conf" will be read
 * @param[out] results where to store the results
 * @param[in] symbol the symbol of the chain (file [symbol].conf should exist)
 * @returns 1 on success, 0 on failure
 */
uint8_t komodo_rpc_info(struct rpc_info* results, const char* symbol);

#ifdef __cplusplus
}
#endif
#endif