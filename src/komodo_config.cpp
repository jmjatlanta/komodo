#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sys/time.h> // gettimeofday
#include "util.h" // GetDataDir()
#include "crc.h"
#include "komodo_config.h"

extern std::map<std::string, std::string> mapArgs;
extern uint16_t BITCOIND_RPCPORT;

ConfigFile::ConfigFile()
{
}

ConfigFile::ConfigFile(const boost::filesystem::path& path)
{
    std::ifstream in;
    in.exceptions(std::ifstream::badbit);
    in.open(path.string());
    if (in.fail())
        throw std::ios_base::failure("Unable to open " + path.string());
    std::string line;
    while ( std::getline(in, line) )
    {
        if (line.find('#') == 0)
            continue;
        size_t pos = line.find('=');
        if (pos != std::string::npos && pos <= line.size() - 1)
        {
            entries.emplace( line.substr(0,pos), line.substr(pos+1));
        }
    }
}

bool ConfigFile::Has(const std::string& key) const
{
    return entries.find(key) != entries.end();
}

std::string ConfigFile::Value(const std::string& key) const
{
    auto itr = entries.find(key);
    if (itr != entries.end())
    {
        return (*itr).second;
    }
    return "";
}

std::vector<std::string> ConfigFile::Values(const std::string& key) const
{
    std::vector<std::string> retval;
    auto range = entries.equal_range(key);
    for( auto itr = range.first; itr != range.second; ++itr)
        retval.push_back( (*itr).second );
    return retval;
}

std::multimap<std::string, std::string> ConfigFile::Entries() const
{
    return entries;
}

void ConfigFile::SetEntries(const std::multimap<std::string, std::string>& in)
{
    entries = in;
}

bool ConfigFile::Save(const boost::filesystem::path& path, bool overwrite) const
{
    if (overwrite && boost::filesystem::exists(path) || !boost::filesystem::exists(path))
    {
        std::ofstream out;
        out.exceptions(std::fstream::failbit | std::fstream::badbit);
        try
        {
            out.open(path.string());
            for(auto itr = entries.begin(); itr != entries.end(); ++itr)
            {
                out << (*itr).first << "=" << (*itr).second << "\n";
            }
            return true;
        } 
        catch (const std::ofstream::failure&)
        {
        }
    }
    return false;
}


/*****
 * Get the full path to the config file
 * @note if symbol is empty, looks for komodo.conf
 * @note the `-conf` command line parameter overrides most logic
 * @param symbol the symbol to use
 * @returns the full path to the config file
 */
boost::filesystem::path GetConfigFile(const std::string& symbol)
{
    boost::filesystem::path retval;
    if (mapArgs.count("-conf"))
        retval = mapArgs["-conf"];
    else
    {
        if (symbol.empty())
        {
#ifdef __linux__
            std::string komodo_conf("komodo.conf");
#else
            std::string komodo_conf("Komodo.conf");
#endif
            retval /= komodo_conf;
        }
        else
            retval /= (symbol + ".conf");
    }
    if (!retval.is_complete())
    {
        // try prefixing with GetDataDir
        retval = GetDataDir(false) / retval;
    }
    return retval;
}

double current_milliseconds()
{
    struct timeval tv; double millis;
    gettimeofday(&tv,NULL);
    millis = ((double)tv.tv_sec * 1000. + (double)tv.tv_usec / 1000.);
    return(millis);
}

#ifndef _WIN32
void randombytes_buf(unsigned char *x,long xlen)
{
    static int fd = -1;
    int32_t i;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom",O_RDONLY);
            if (fd != -1) break;
            sleep(1);
        }
    }
    while (xlen > 0) {
        if (xlen < 1048576) i = (int32_t)xlen; else i = 1048576;
        i = (int32_t)read(fd,x,i);
        if (i < 1) {
            sleep(1);
            continue;
        }
        if ( 0 )
        {
            int32_t j;
            for (j=0; j<i; j++)
                printf("%02x ",x[j]);
            printf("-> %p\n",x);
        }
        x += i;
        xlen -= i;
    }
}
#endif

/******
 * Generate a random username and password
 * @param[in] salt to help generate randomization
 * @param[out] user the resultant username
 * @param[out] pw the resultant password
 */
void generate_random_user_pw(const std::string& salt, std::string& user, std::string& pw)
{
    char buf[128];
    uint8_t buf2[33]; 
    uint32_t r = (uint32_t)time(NULL);
    uint32_t r2 = current_milliseconds();
    memcpy(buf,&r,sizeof(r));
    memcpy(&buf[sizeof(r)],&r2,sizeof(r2));
    memcpy(&buf[sizeof(r)+sizeof(r2)],salt.c_str(),salt.size());
    uint32_t crc = calc_crc32(0,(uint8_t *)buf,(int32_t)(sizeof(r)+sizeof(r2)+salt.size()));
    randombytes_buf(buf2,sizeof(buf2));
    char password[8192];
    uint32_t i;
    for (uint32_t i=0; i<sizeof(buf2); i++)
        sprintf(&password[i*2],"%02x",buf2[i]);
    password[i*2] = 0;
    pw = std::string("pass") + password;
    user = "user" + std::to_string(crc);
    return;
}

/***
 * Read or create a [symbol].conf file. Also sets the globals
 *    BITCOIND_RPCPORT (based on passed in value)
 *    KMD_PORT (based on komodo.conf)
 *    KMDUSERPASS (based on komodo.conf)
 * and the mapArgs (randomly generated)
 *    -rpcusername
 *    -rpcpassword
 * if symbol or rpcport are not passed, only komodo.conf is read
 * @note only seems to be called if ASSETCHAINS_SYMBOL is set, so only on KMD subchains
 * @param[in] symbol the symbol for the asset chain
 * @param[in] rpcport the rpc port for the asset chain
 */
void komodo_configfile(char *symbol,uint16_t rpcport)
{
    if ( symbol != 0 && rpcport != 0 )
    {
        BITCOIND_RPCPORT = rpcport;
        try
        {
            ConfigFile config_f(GetConfigFile(symbol));
            mapArgs["-rpcuser"] = config_f.Value("rpcuser");
            mapArgs["-rpcpassword"] = config_f.Value("rpcpassword");
        }
        catch (const std::ios_base::failure& ex)
        {
            // attempt to create a config file
            LogPrintf("Unable to read %s, attempting to create.\n", GetConfigFile(symbol).c_str());
            ConfigFile config_f;
            std::multimap<std::string, std::string> entries;
            std::string user;
            std::string pw;
            generate_random_user_pw(symbol, user, pw);
            entries.emplace("rpcuser", user);
            entries.emplace("rpcpassword", pw);
            entries.emplace("rpcport", std::to_string(rpcport));
            entries.emplace("server", "1");
            entries.emplace("txindex", "1");
            entries.emplace("rpcworkqueue", "256");
            entries.emplace("rpcallowip", "127.0.0.1");
            entries.emplace("rpcbind", "127.0.0.1");
            config_f.SetEntries(entries);
            if (!config_f.Save(GetConfigFile(symbol), false))
            {
                LogPrintf("Unable to create config file [%s]\n", GetConfigFile(symbol).c_str());
            }
            mapArgs["-rpcuser"] = config_f.Value("rpcuser");
            mapArgs["-rpcpassword"] = config_f.Value("rpcpassword");
        }
    }
}
