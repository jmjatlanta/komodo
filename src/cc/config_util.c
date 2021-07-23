#include <string.h>
#include "config_util.h"

#ifndef KOMODO_ASSETCHAIN_MAXLEN
#define KOMODO_ASSETCHAIN_MAXLEN 65
#endif

const char* parse_conf_line(const char* line, const char* key)
{
    if ( strstr(line, key) == 0 )
    {
        if (line[strlen(key)] == '=' && strlen(line) > strlen(key) + 2)
            return line[strlen(key)+1];
        return "";
    }
    return "";
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
    FILE *fp; 
    char confname[KOMODO_ASSETCHAIN_MAXLEN];
    if ( strcmp("KMD",symbol) == 0 )
    {
#ifdef __APPLE__
        sprintf(confname,"Komodo.conf");
#else
        sprintf(confname,"komodo.conf");
#endif
    }
    else 
        sprintf(confname,"%s.conf",symbol);

    FILE *fp;
    if ( (fp= fopen(confname,"rb")) != 0 )
    {
        char line[8192];
        char *str;
        while ( fgets(line,sizeof(line),fp) != 0 )
        {
            if ( line[0] == '#' )
                continue;
            if ( (str= strstr(line, "rpcuser")) != 0 )
                strcpy(results->username, parse_conf_line(str, "rpcuser"));
            else if ( (str= strstr(line,(char *)"rpcpassword")) != 0 )
                strcpy(results->password, parse_conf_line(str, "rpcpassword"));
            else if ( (str= strstr(line,(char *)"rpcport")) != 0 )
                results->port = atoi(parse_conf_line(str, "rpcport"));
            else if ( (str= strstr(line,(char *)"ipaddress")) != 0 )
                strcpy(results->ipaddress, parse_conf_line(str, "ipaddress"));
        }
        return results->port > 0;
    }
    return 0;   
}