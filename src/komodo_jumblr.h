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
#include "uthash.h" // UT_hash_handle
#include "komodo_cJSON.h"
#include "komodo_defs.h"

#ifdef _WIN32
#include <wincrypt.h>
#endif

#define JUMBLR_ADDR "RGhxXpXSSBTBm9EvNsXnTQczthMCxHX91t"
#define JUMBLR_BTCADDR "18RmTJe9qMech8siuhYfMtHo8RtcN1obC6"
#define JUMBLR_MAXSECRETADDRS 777
#define JUMBLR_SYNCHRONIZED_BLOCKS 10
#define JUMBLR_INCR 9.965
#define JUMBLR_FEE 0.001
#define JUMBLR_TXFEE 0.01
#define SMALLVAL 0.000000000000001

#define JUMBLR_ERROR_DUPLICATEDEPOSIT -1
#define JUMBLR_ERROR_SECRETCANTBEDEPOSIT -2
#define JUMBLR_ERROR_TOOMANYSECRETS -3
#define JUMBLR_ERROR_NOTINWALLET -4

struct jumblr_item
{
    UT_hash_handle hh; // a hash table of stored jumblr items
    int64_t amount;
    int64_t fee;
    int64_t txfee; // fee and txfee not really used (yet)
    uint32_t spent;
    uint32_t pad;
    char opid[66];
    char src[128];
    char dest[128];
    char status;
};

class Jumblr
{
public:
    Jumblr(const std::string& userpass, int port) : userpass(userpass), port(port) {}
    bool pause = true;

private:
    std::string userpass;
    uint16_t port;

public:
    /****
     * @brief add a secret address
     * @note these are external
     * @param secretaddr the address to add
     * @returns the index of the added address if successful. Otherwise an error code ( < 0 )
     */
    int32_t secretaddradd(char *secretaddr);
    /****
     * @brief add a deposit address
     * @note these are external
     * @param depositaddr the address to add
     * @returns the index of the added address if successful. Otherwise an error code ( < 0 )
     */
    int32_t depositaddradd(char *depositaddr);

    void iteration();

private:
    /****
     * @brief make the importaddress RPC call
     * @param address the address to import
     * @returns the results in JSON format
     */
    char *importaddress(char *address);
    /*****
     * @brief check the validity of an address
     * @param addr the address
     * @returns the results in JSON format
     */
    char *validateaddress(char *addr);
    /****
     * @brief find the index location of a particular secret address
     * @param searchaddr what to look for
     * @returns the index location or -1
     */
    int32_t secretaddrfind(char *searchaddr);
    /*****
     * @brief generate a secret address
     * @pre Requires secret addresses already stored
     * @param[out] secretaddr where to store the result
     * @returns the index of the address selected
     */
    int32_t secretaddr(char *secretaddr);
    /****
     * @brief determine the address type
     * @param addr the address to examine
     * @returns 'z', 't', or -1 on error
     */
    int32_t addresstype(char *addr);
    /*****
     * @brief search for a jumblr item by opid
     * @param opid
     * @returns the item (or nullptr)
     */
    jumblr_item *opidfind(char *opid);
    /****
     * @brief add (or find existing) opid to the hashtable
     * @param opid the opid to add
     * @returns the full entry
     */
    jumblr_item *opidadd(char *opid);

    char *zgetnewaddress();

    char *zlistoperationids();

    /*****
     * @brief Retrieve result and status of an operation which has finished, and then remove the operation from memory
     * @note makes an RPC call
     * @param opid the operation id
     * @returns an operation result
     */
    char *zgetoperationresult(char *opid);

    char *zgetoperationstatus(char *opid);

    char *sendt_to_z(char *taddr,char *zaddr,double amount);

    char *sendz_to_z(char *zaddrS,char *zaddrD,double amount);

    char *sendz_to_t(char *zaddr,char *taddr,double amount);

    char *zlistaddresses();

    char *zlistreceivedbyaddress(char *addr);

    char *getreceivedbyaddress(char *addr);

    char *importprivkey(char *wifstr);

    char *zgetbalance(char *addr);

    char *listunspent(char *coinaddr);

    char *gettransaction(char *txidstr);

    int32_t numvins(bits256 txid);

    int64_t receivedby(char *addr);

    int64_t balance(char *addr);

    int32_t itemset(jumblr_item *ptr,cJSON *item,char *status);

    void opidupdate(jumblr_item *ptr);

    void prune(jumblr_item *ptr);

    void zaddrinit(char *zaddr);

    void opidsupdate();

    uint64_t increment(uint8_t r,int32_t height,uint64_t total,uint64_t biggest,uint64_t medium, uint64_t smallest);

    /*****
     * @brief make an RPC style method call
     * @param method the method to call
     * @param params the method parameters (nullptr ok)
     * @returns the results in JSON format
     */
    char *issuemethod(const std::string& method, const std::string& params);

};

