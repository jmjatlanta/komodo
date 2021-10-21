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

/**
 * Store key/value pairs on an assetchain via OP_RETURN
 * See https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/blockchain.html#kvupdate
 */
#include "komodo_defs.h"
#include "uthash.h"
#include "bits256.h"

#include <cstdint>
#include <mutex>

struct komodo_kv 
{ 
    UT_hash_handle hh; 
    bits256 pubkey; 
    uint8_t *key;
    uint8_t *value; 
    int32_t height; 
    uint32_t flags; 
    uint16_t keylen;
    uint16_t valuesize; 
};

class KV
{
public:
    /*****
     * @brief search hash table for key
     * @param[out] pubkeyp the public key of the found value
     * @param[in] current_height current chain height (to check expiry)
     * @param[out] flagsp the flags of the found value
     * @param[out] heightp the height of the found value
     * @param[out] value the value
     * @param[in] key the key to search for
     * @param[in] keylen the key length
     * @returns -1 if not found, otherwise the size of `value`
     */
    int32_t search(uint256 *pubkeyp,int32_t current_height,uint32_t *flagsp,int32_t *heightp,uint8_t value[IGUANA_MAXSCRIPTSIZE],uint8_t *key,int32_t keylen);

    /****
     * @brief add or update a kv entry
     * @param opretbuf the entry to update
     * @param opretlen the length of `opretbuf`
     * @param value the fee
     */
    void update(uint8_t *opretbuf,int32_t opretlen,uint64_t value);

    /***
     * @brief sign data with a private key
     * @param[in] buf the data to be signed
     * @param[in] len the length of the data in `buf`
     * @param[in] _privkey the private key
     * @returns the signature
     */
    uint256 sig(uint8_t *buf,int32_t len,uint256 _privkey);

    /*****
     * @brief verify signature
     * @param buf the data that was signed
     * @param len the length of `buf`
     * @param _pubkey the signer's public key
     * @param sig the given signature
     * @returns 0 if signature matches, -1 otherwise
     */
    bool sigverify(uint8_t *buf,int32_t len,uint256 _pubkey,uint256 sig);

    /****
     * @brief get duration via flags and block counts
     * @param flags where number of days is stored
     * @returns the duration
     */
    int32_t duration(uint32_t flags);

    /*****
     * @brief calculate fee
     * @param flags to calculate number of days
     * @param opretlen the size of the data
     * @param keylen the key length
     * @returns the fee 
     */
    uint64_t fee(uint32_t flags,int32_t opretlen,int32_t keylen);

    /****
     * @brief get public/private keys based on passed in information
     * @param[out] pubkeyp the public key
     * @param[in] passphrase the passphrase
     * @returns the private key
     */
    uint256 privkey(uint256 *pubkeyp,char *passphrase);

private:
    /***
     * Compare two values
     * @param refvalue value A
     * @param refvaluesize the length of value A
     * @param value value B
     * @param valuesize the length of value B
     * @returns -1 if size mismatch or one of the values is null, 0 if both values are equal, < 0 if A < B, >0 if A > B
     */
    int32_t cmp(uint8_t *refvalue,uint16_t refvaluesize,uint8_t *value,uint16_t valuesize);

    /***
     * Retrieve number of days stored in flags
     * @param flags where the number of days is stored (using bitwise manipulation)
     * @returns number of days (max 365)
     */
    int32_t numdays(uint32_t flags);

private:
    std::mutex kv_mutex;
    komodo_kv *KOMODO_KV;
};
