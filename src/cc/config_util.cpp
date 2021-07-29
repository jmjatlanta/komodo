#include <string.h>
#include "config_util.h"
#include "komodo_config.h"

#ifndef KOMODO_ASSETCHAIN_MAXLEN
#define KOMODO_ASSETCHAIN_MAXLEN 65
#endif

static void copy_value(char* dest, const ConfigFile& values, const std::string& key, size_t max_len)
{
    std::string temp = values.Value(key);
    if (!temp.empty())
        strncpy( dest, temp.c_str(), max_len - 1);
}

/****
 * @brief Read the RPC info from a config file
 * @note if symbols is "KMD" the file "komodo.conf" will be read
 * @param[out] results where to store the results
 * @param[in] symbol the symbol of the chain (file [symbol].conf should exist)
 * @returns 1 on success, 0 on failure
 */
uint8_t komodo_rpc_info(struct rpc_info* results, const char* symbol)
{
    if (!results)
        return 0;

    memcpy(results, 0, sizeof(results));

    try
    {
        ConfigFile configFile(GetConfigFile(symbol));
        std::string temp = configFile.Value("rpcport");
        if (!temp.empty())
            results->port = std::stol(temp);
        copy_value(results->username, configFile, "rpcuser", 512);
        copy_value(results->password, configFile, "rpcpassword", 512);
        copy_value(results->ipaddress, configFile, "ipaddress", 100);
        return 1;
    } catch ( const std::ios_base::failure&)
    {
    }
    return 0;
}