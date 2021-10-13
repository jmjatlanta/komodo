#pragma once
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
#include "chainparams.h"
#include <string>

class assetchain
{
public:
    assetchain() : symbol_("") {}
    assetchain(const std::string& symbol) : symbol_(symbol)
    {
        if (symbol_.size() > 64)
            symbol_ = symbol_.substr(0, 64);
    }

    /******
     * Assetchain Symbol
     */

    /*****
     * @returns true if the chain is Komodo
     */
    bool isKMD() { return symbol_.empty(); }
    /****
     * @param in the symbol to compare
     * @returns true if this chain's symbol matches
     */
    bool isSymbol(const std::string& in) { return in == symbol_; }
    /****
     * @returns this chain's symbol (will be empty for KMD)
     */
    std::string symbol() { return symbol_; }
    /****
     * @returns this chain's symbol, "KMD" in the case of Komodo
     */
    std::string ToString() 
    { 
        if (symbol_.empty()) 
            return "KMD"; 
        return symbol_; 
    }
    bool SymbolStartsWith(const std::string& in) { return symbol_.find(in) == 0; }
    
    /******
     * ChainParams
     */

    /****
     * @note requires a previous call to SelectParams
     * @returns the parameters for this chain
     */
    const CChainParams &Params() { return *pCurrentParams; }

    /*****
     * Sets the params returned by Params() to those for the given network.
     * @param network the network to use as the default
     */
    void SelectParams(CBaseChainParams::Network network);

    /***
     * @brief Return parameters for a given network.
     * @param network the network params to retrieve
     * @returns the parameters
     */
    CChainParams &Params(CBaseChainParams::Network network);

    /****
     * @brief examine command line params and set appropriate CChainParams
     * @returns true on success, false if command line param combination was invalid
     */
    bool SelectParamsFromCommandLine();
    /*****
     * @brief initialize chain parameters
     * @param saplingActivationHeight from the command line
     * @param overwinterActivationHeight from the command line
     */
    void InitChainParams(int64_t saplingActivationHeight, int64_t overwinterActivationHeight);

    /****
     * @brief set sapling and overwinter heights
     * @param height
     */
    void SetActivation(int32_t height) { SetSaplingHeight(height); SetOverwinterHeight(height); }
    void SetSaplingHeight(int32_t height) { pCurrentParams->SetSaplingHeight(height); }
    void SetOverwinterHeight(int32_t height) { pCurrentParams->SetOverwinterHeight(height); }

    /****
     * @note only appicable to regtest
     * @param idx
     * @param nActivationHeight
     */
    void UpdateNetworkUpgradeParameters(Consensus::UpgradeIndex idx, int nActivationHeight)
    {
        regTestParams.UpdateNetworkUpgradeParameters(idx, nActivationHeight);
    }

    bool isInitialized() { return initialized; }

private:
    // symbol
    std::string symbol_;
    // chainparams
    CChainParams *pCurrentParams = nullptr;
    CMainParams mainParams;
    CTestNetParams testNetParams;
    CRegTestParams regTestParams;
    bool initialized = false;
};
