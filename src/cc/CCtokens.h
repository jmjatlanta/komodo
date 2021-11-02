/******************************************************************************
 * Copyright © 2014-2018 The SuperNET Developers.                             *
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

struct CCTokens : public CCcontract_info
{
public:
    CCTokens() : CCcontract_info(EVAL_TOKENS, 
            "RAMvUfoyURBRxAdVeTMHxn3giJZCFWeha2", 
            "RCNgAngYAdrfzujYyPgfbjCGNVQZzCgTad", 
            "03e6191c70c9c9a28f9fd87089b9488d0e6c02fb629df64979c9cdb6b2b4a68d95",
            (const uint8_t[]){ 0x1d, 0x0d, 0x0d, 0xce, 0x2d, 0xd2, 0xe1, 0x9d, 0xf5, 
                0xb6, 0x26, 0xd5, 0xad, 0xa0, 0xf0, 0x0a, 0xdd, 0x7a, 0x72, 0x7d, 0x17, 0x35, 
                0xb5, 0xe3, 0x2c, 0x6c, 0xa9, 0xa2, 0x03, 0x16, 0x4b, 0xcf })
    { }
    /*****
     * Validate a transaction
     * @param eval holds results
     * @param tx the transaction to validate
     * @param nIn not used
     * @returns true on success
     */    
    virtual bool validate(Eval* eval, const CTransaction &tx, uint32_t nIn) override;

    /******
     * @brief compares cc inputs vs cc outputs (to prevent feeding vouts from normal inputs)
     * @param goDeeper
     * @param inputs
     * @param outputs
     * @param eval
     * @param tx
     * @param reftokenid
     * @returns true if inputs equal outputs
     */
    bool AmountsExact(bool goDeeper, int64_t &inputs, int64_t &outputs, 
            Eval* eval, const CTransaction &tx, uint256 tokenid);

    /****
     * @param txfee
     * @param tokensupply
     * @param name
     * @param description
     * @returns token creation signed raw tx
     */
    static std::string CreateToken(int64_t txfee, int64_t assetsupply, const std::string& name, 
            const std::string& description, const std::vector<uint8_t>& nonfungibleData);

    UniValue TokenInfo(uint256 tokenid);
    UniValue TokenList();

    /****
     * @brief transfer tokens to another pubkey
     * @param txfee
     * @param tokenid
     * @param destpubkey
     * @param total
     * @returns the transaction
     */
    static std::string Transfer(int64_t txfee, uint256 assetid, 
            const std::vector<uint8_t>& destpubkey, int64_t total);

    bool IsTokenMarkerVout(CTxOut vout);

    /*****
     * @brief Checks if a transaction vout is true token vout
     * @note for this check pubkeys and eval code in token opreturn are used to recreate vout and 
     * compare it with the checked vout. Verifies that the transaction total token inputs value 
     * equals to total token outputs (that is, token balance is not changed in this transaction)
     * @param goDeeper also recursively checks the previous token transactions (or the creation transaction) and ensures token balance is not changed for them too
     * @param checkPubkeys always true
     * @param cp CCcontract_info structure initialized for EVAL_TOKENS eval code
     * @param eval could be NULL, if not NULL then the eval parameter is used to report validation error
     * @param tx transaction object to check
     * @param v vout number (starting from 0)
     * @param reftokenid id of the token. The vout is checked if it has this tokenid
     * @returns true if vout is true token with the reftokenid id
     */
    int64_t IsTokensvout(bool goDeeper, bool checkPubkeys, Eval* eval, const CTransaction& tx, int32_t v, uint256 reftokenid);

    /*****
     * @brief Adds token inputs to transaction object. 
     * If tokenid is a non-fungible token then the function will set additionalTokensEvalcode2 variable in 
     * the cp object to the eval code from NFT data to spend NFT outputs properly
     * @param[out] mtx mutable transaction object
     * @param pk pubkey for whose token inputs to add
     * @param tokenid id of token which inputs to add
     * @param total amount to add (if total==0 no inputs are added and all available amount is returned)
     * @param maxinputs maximum number of inputs to add. If 0 then CC_MAXVINS define is used
     * @returns
     */
    int64_t AddCCInputs(CMutableTransaction &mtx, CPubKey pk, uint256 tokenid, 
            int64_t total, int32_t maxinputs);

    /*****
     * @brief Adds token inputs to transaction object, also returns NFT data in vopretNonfungible parameter
     * If tokenid is a non-fungible token then the function will set additionalTokensEvalcode2 variable in 
     * the cp object to the eval code from NFT data to spend NFT outputs properly
     * @param[out] mtx mutable transaction object
     * @param pk pubkey for whose token inputs to add
     * @param tokenid id of token which inputs to add
     * @param total amount to add (if total==0 no inputs are added and all available amount is returned)
     * @param maxinputs maximum number of inputs to add. If 0 then CC_MAXVINS define is used
     * @param[out] vopretNonfungible the NFT data
     * @returns
     */
    int64_t AddCCInputs(CMutableTransaction &mtx, CPubKey pk, uint256 tokenid, 
            int64_t total, int32_t maxinputs, vscript_t &vopretNonfungible);

    int64_t GetTokenBalance(CPubKey pk, uint256 tokenid);

    /****
     * @brief Makes opreturn scriptPubKey for token creation transaction. 
     * Normally this function is called internally by the tokencreate rpc. 
     * You might need to call this function to create a customized token.
     * The total opreturn length should not exceed 10001 bytes
     * @param funcid should be set to 'c' character
     * @param origpubkey token creator pubkey as byte array
     * @param name token name (no more than 32 char)
     * @param description token description (no more than 4096 char)
     * @param vopretNonfungible NFT data, could be empty. If not empty, NFT will be created, the first byte if the NFT data should be set to the eval code of the contract validating this NFT data
     * @returns scriptPubKey with OP_RETURN script
     */
    static CScript EncodeCreateOpRet(uint8_t funcid, const std::vector<uint8_t>& origpubkey, 
            const std::string& name, const std::string& description, vscript_t vopretNonfungible);

    /*****
     * @brief Makes opreturn scriptPubKey for token creation transaction. 
     * Normally this function is called internally by the tokencreate rpc.
     * You might need to call it to create a customized token.
     * The total opreturn length should not exceed 10001 byte
     * @see opretid
     * @param funcid should be set to 'c' character
     * @param origpubkey token creator pubkey as byte array
     * @param name token name (no more than 32 char)
     * @param description token description (no more than 4096 char)
     * @param oprets vector of pairs of additional data added to the token opret. The first element in the pair is opretid enum, the second is the data as byte array
     * @returns scriptPubKey with OP_RETURN script
     */
    static CScript EncodeCreateOpRet(uint8_t funcid, const std::vector<uint8_t>& origpubkey, 
            const std::string& name, const std::string& description, 
            std::vector<std::pair<uint8_t, vscript_t>> oprets);

    /*****
     * @brief Makes opreturn scriptPubKey for token transaction. 
     * Normally this function is called internally by the token rpcs. 
     * You might call this function if your module should create a customized token.
     * The total opreturn length should not exceed 10001 byte
     * @param tokenid id of the token
     * @param voutPubkeys vector of pubkeys used to make the token vout in the same transaction that the created opreturn is for, the pubkeys are used for token vout validation
     * @param opretWithId a pair of additional opreturn data added to the token opret. Could be empty. The first element in the pair is opretid enum, the second is the data as byte array
     * @returns scriptPubKey with OP_RETURN script
     */
    static CScript EncodeTransactionOpRet(uint256 tokenid, const std::vector<CPubKey>& voutPubkeys, 
            std::pair<uint8_t, vscript_t> opretWithId);

    /****
     * @brief Makes opreturn scriptPubKey for token transaction.
     * An overload to make opreturn scriptPubKey for token transaction. 
     * Normally this function is called internally by the token rpcs. 
     * You might call this function if your module should create a customized token.
     * The total opreturn length should not exceed 10001 byte
     * @param tokenid id of the token
     * @param voutPubkeys vector of pubkeys used to make the token vout in the same transaction that the created opreturn is for, the pubkeys are used for token vout validation
     * @param oprets vector of pairs of additional opreturn data added to the token opret. Could be empty. The first element in the pair is opretid enum, the second is the data as byte array
     * @returns scriptPubKey with OP_RETURN script
     */
    static CScript EncodeTransactionOpRet(uint256 tokenid, const std::vector<CPubKey>& voutPubkeys, 
            std::vector<std::pair<uint8_t, vscript_t>> oprets);

    /******
     * @brief Decodes opreturn scriptPubKey of token creation transaction. 
     * Normally this function is called internally by the token rpcs.
     * You might call this function if your module should create a customized token.
     * @param scriptPubKey OP_RETURN script to decode
     * @param[out] origpubkey creator public key as a byte array
     * @param[out] name token name 
     * @param[out] description token description 
     * @returns funcid ('c') or NULL if errors
     */
    static uint8_t DecodeCreateOpRet(const CScript &scriptPubKey, 
            std::vector<uint8_t> &origpubkey, std::string &name, std::string &description);

    /****
     * @brief Decodes opreturn scriptPubKey of token creation transaction. 
     * @note Overload that decodes opreturn scriptPubKey of token creation transaction and also returns additional data blobs. 
     * Normally this function is called internally by the token rpcs.
     * You might want to call this function if your module should create a customized token.
     * @param scriptPubKey OP_RETURN script to decode
     * @param[out] origpubkey creator public key as a byte array
     * @param[out] name token name 
     * @param[out] description token description 
     * @param[out] oprets vector of pairs of additional opreturn data added to the token opret. Could be empty if not set. The first element in the pair is opretid enum, the second is the data as byte array
     * @returns funcid ('c') or NULL if errors
     */
    static uint8_t DecodeCreateOpRet(const CScript &scriptPubKey, std::vector<uint8_t> &origpubkey, 
            std::string &name, std::string &description, std::vector<std::pair<uint8_t, vscript_t>>  &oprets);

    /*****
     * @brief Decodes opreturn scriptPubKey of token transaction, also returns additional data blobs. 
     * Normally this function is called internally by different token rpc.
     * You might want to call if your module created a customized token.
     * @param scriptPubKey OP_RETURN script to decode
     * @param[out] evalCodeTokens should be EVAL_TOKENS
     * @param[out] tokenid id of token 
     * @param[out] voutPubkeys vector of token output validation pubkeys from the opreturn
     * @param[out] oprets vector of pairs of additional opreturn data added to the token opret. Could be empty if not set. The first element in the pair is opretid enum, the second is the data as byte array
     * @returns funcid ('c' if creation tx or 't' if token transfer tx) or NULL if errors
     */
    static uint8_t DecodeTransactionOpRet(const CScript scriptPubKey, uint8_t &evalCodeTokens, uint256 &tokenid, std::vector<CPubKey> &voutPubkeys, std::vector<std::pair<uint8_t, vscript_t>>  &oprets);


    /*****
     * @brief Creates a token cryptocondition that allows to spend it by one key
     * @note The resulting cc will have two eval codes (EVAL_TOKENS and evalcode parameter value).
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param pk pubkey to spend the cc
     * @returns cryptocondition object. Must be disposed with cc_free function when not used any more
     */
    static CC *MakeCCcond1(uint8_t evalcode, CPubKey pk);

    /*********
     * @brief Creates a token cryptocondition that allows to spend it by one key
     * @note The resulting cc will have two eval codes (EVAL_TOKENS and evalcode parameter value).
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)    /// @param evalcode2 yet another cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param pk pubkey to spend the cc
     * @returns cryptocondition object. Must be disposed with cc_free function when not used any more
     */
    static CC *MakeCCcond1(uint8_t evalcode, uint8_t evalcode2, CPubKey pk);

    /******
     * @brief Creates new 1of2 token cryptocondition that allows to spend it by either of two keys
     * @note Resulting vout will have three eval codes (EVAL_TOKENS, evalcode and evalcode2 parameter values).
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param pk1 one of two pubkeys to spend the cc
     * @param pk2 second of two pubkeys to spend the cc
     * @returns cryptocondition object. Must be disposed with cc_free function when not used any more
     */
    static CC *MakeCCcond1of2(uint8_t evalcode, CPubKey pk1, CPubKey pk2);

    /******
     * @brief Creates new 1of2 token cryptocondition that allows to spend it by either of two keys
     * @note The resulting cc will have two eval codes (EVAL_TOKENS and evalcode parameter value).
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param evalcode2 yet another cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param pk1 one of two pubkeys to spend the cc
     * @param pk2 second of two pubkeys to spend the cc
     * @returns cryptocondition object. Must be disposed with cc_free function when not used any more
     */
    static CC *MakeCCcond1of2(uint8_t evalcode, uint8_t evalcode2, CPubKey pk1, CPubKey pk2);

    /******
     * @brief Creates a token transaction output with a cryptocondition that allows to spend it by one key. 
     * @note The resulting vout will have two eval codes (EVAL_TOKENS and evalcode parameter value).
     * @note The returned output should be added to a transaction vout array.
     * @see CCinit
     * @see CCcontract_info
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param nValue value of the output in satoshi
     * @param pk pubkey to spend the cc
     * @returns vout object
     */
    static CTxOut MakeCC1vout(uint8_t evalcode, CAmount nValue, CPubKey pk);

    /*****
     * @brief Creates a token transaction output with a cryptocondition with two eval codes that allows to spend it by one key. 
     * @note Resulting vout will have three eval codes (EVAL_TOKENS, evalcode and evalcode2 parameter values).
     * @note The returned output should be added to a transaction vout array.
     * @see CCinit
     * @see CCcontract_info
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param evalcode2 yet another cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param nValue value of the output in satoshi
     * @param pk pubkey to spend the cc
     * @returns vout object
     */
    static CTxOut MakeCC1vout(uint8_t evalcode, uint8_t evalcode2, CAmount nValue, CPubKey pk);

    /*****
     * @brief creates a token transaction output with a 1of2 cryptocondition that allows to spend it by either of two keys. 
     * @note The resulting vout will have two eval codes (EVAL_TOKENS and evalcode parameter value).
     * @note The returned output should be added to a transaction vout array.
     * @see CCinit
     * @see CCcontract_info
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param nValue value of the output in satoshi
     * @param pk1 one of two pubkeys to spend the cc
     * @param pk2 second of two pubkeys to spend the cc
     * @returns vout object
     */
    static CTxOut MakeCC1of2vout(uint8_t evalcode, CAmount nValue, CPubKey pk1, CPubKey pk2);

    /******
     * @brief Creates a token transaction output with a 1of2 cryptocondition with two eval codes that allows to spend it by either of two keys. 
     * @note The resulting vout will have three eval codes (EVAL_TOKENS, evalcode and evalcode2 parameter values).
     * @note The returned output should be added to a transaction vout array.
     * @see CCinit
     * @see CCcontract_info
     * @param evalcode cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param evalcode2 yet another cryptocondition eval code (transactions with this eval code in cc inputs will be forwarded to the contract associated with this eval code)
     * @param nValue value of the output in satoshi
     * @param pk1 one of two pubkeys to spend the cc
     * @param pk2 second of two pubkeys to spend the cc
     * @returns vout object
     */
    static CTxOut MakeCC1of2vout(uint8_t evalcode, uint8_t evalcode2, CAmount nValue, CPubKey pk1, CPubKey pk2);

    /******
     * @breif Gets adddress for token cryptocondition vout
     * @param cp CCcontract_info structure (NOTE: not always EVAL_TOKENS, could be CCAsset)
     * @param[out] destaddr retrieved address
     * @param pk public key to create the cryptocondition
     */
    static bool GetCCaddress(CCcontract_info *cp, char *destaddr, CPubKey pk);

    /******
     * @brief Gets adddress for token 1of2 cc vout
     * @param cp CCcontract_info structure initialized with EVAL_TOKENS eval code
     * @param[out] destaddr retrieved address
     * @param pk first public key to create the cryptocondition
     * @param pk2 second public key to create the cryptocondition
     */
    static bool GetCCaddress1of2(CCcontract_info *cp, char *destaddr, CPubKey pk, CPubKey pk2);

    /******
     * @brief Sets pubkeys, private key and cc addr for spending from 1of2 token cryptocondition vout
     * @see CCTokens::GetCCaddress
     * @see CCcontract_info
     * @param cp contract info structure where the private key is set
     * @param pk1 one of the two public keys of the 1of2 cc
     * @param pk2 second of the two public keys of the 1of2 cc
     * @param priv private key for one of the two pubkeys
     * @param coinaddr the cc address obtained for this 1of2 token cc with GetTokensCCaddress1of2
     */
    static void CCaddr1of2set(CCcontract_info *cp, CPubKey pk1, CPubKey pk2, uint8_t *priv, char *coinaddr);

private: 
    /******
     * @brief checks if any token vouts are sent to 'dead' pubkey
     * @param eval
     * @param tx
     * @param reftokenid
     * @returns the burned amount
     */
    int64_t HasBurnedTokensvouts(Eval* eval, const CTransaction& tx, uint256 reftokenid);

    static bool GetCCaddress(char *destaddr, uint8_t evalcode, uint8_t evalcode2, CPubKey pk);

};
