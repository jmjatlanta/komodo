/****
 * Test the configuration
 */
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "komodo_config.h"

std::string komodo_statefname(const std::string& symbol, const std::string& str);
void ClearDatadirCache();
boost::filesystem::path GetDefaultDataDir();
uint16_t komodo_userpass(char *userpass, const std::string& symbol);
extern char ASSETCHAINS_SYMBOL[65];
extern std::map<std::string, std::string> mapArgs;

namespace TestConfig
{

TEST(TestConfig, userpass)
{
    char userpass[512];
    std::string token = "J_M_J";
    strcpy(ASSETCHAINS_SYMBOL, token.c_str());

    boost::filesystem::path jmj_path = boost::filesystem::temp_directory_path();
    jmj_path /= token;
    boost::filesystem::create_directories(jmj_path);
    mapArgs["-datadir"] = jmj_path.string();
    ClearDatadirCache();
    boost::filesystem::path config_file = jmj_path / (token + ".conf");
    {
        std::ofstream out;
        out.exceptions(std::ios_base::badbit | std::ios::failbit);
        out.open(config_file.string());
        out << "rpcuser=abc\nrpcpassword=123\nrpcport=456\n";
    }
    uint16_t port = komodo_userpass(userpass, token);
    ASSERT_EQ(port, 456);
    ASSERT_EQ(std::string(userpass), "abc:123");
}

TEST(TestConfig, DefaultDataDir)
{
#ifdef __linux__
    ASSETCHAINS_SYMBOL[0] = 0;
    std::string home_dir = getenv("HOME");
    std::string expected = home_dir + "/.komodo";
    ASSERT_EQ(GetDefaultDataDir().string(), expected);
    strcpy(ASSETCHAINS_SYMBOL, "JMJ");
    expected = home_dir + "/.komodo/JMJ";
    ASSERT_EQ(GetDefaultDataDir().string(), expected);

#endif
}

/***
 * Test the functinality of the komodo_statefname
 * Depends on 
 *  -datadir command line parameter, 
 *  ASSETCHAINS_SYMBOL global, 
 *  HOME environment variable (if -datadir not set)
 *  running platform (komodo.conf, default path)
 */    
TEST(TestConfig, statefname)
{
    mapArgs.clear();
    ASSETCHAINS_SYMBOL[0] = 0;
    ClearDatadirCache();
#ifdef __linux__
    std::string fname;
    // gather the expected results
    char* home_dir = getenv("HOME");
    char symbol[32];
    symbol[0] = 0;
    char str[32];
    str[0] = 0;
    // defaults
    std::string expected(home_dir);
    expected += "/.komodo/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with str filled in
    strcpy(str, "blah.txt");
    expected = home_dir;
    expected += "/.komodo/blah.txt";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with ASSETCHAINS_SYMBOL filled in with unexpected value
    str[0] = 0;
    strcpy(ASSETCHAINS_SYMBOL, "JMJ");
    expected = home_dir;
    expected += "/.komodo";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "REGTEST"
    strcpy(symbol, "REGTEST");
    expected = home_dir;
    expected += "/.komodo";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "KMD"
    strcpy(symbol, "KMD");
    expected = home_dir;
    expected += "/.komodo";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL empty and symbol filled in with "KMD"
    ASSETCHAINS_SYMBOL[0] = 0;
    strcpy(symbol, "KMD");
    expected = home_dir;
    expected += "/.komodo/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));

    // with -datadir set
    ClearDatadirCache();
    mapArgs["-datadir"] = boost::filesystem::temp_directory_path().string();
    expected = "/tmp/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with str filled in
    strcpy(str, "blah.txt");
    expected = "/tmp/blah.txt";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with ASSETCHAINS_SYMBOL filled in with unexpected value
    str[0] = 0;
    strcpy(ASSETCHAINS_SYMBOL, "JMJ");
    expected = "/tmp";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "REGTEST"
    strcpy(symbol, "REGTEST");
    expected = "/tmp/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "KMD"
    strcpy(symbol, "KMD");
    expected = "/tmp";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL empty and symbol filled in with "KMD"
    ASSETCHAINS_SYMBOL[0] = 0;
    strcpy(symbol, "KMD");
    expected = "/tmp/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));

    // with -datadir set to end with ASSETCHAINS_SYMBOL
    strcpy(ASSETCHAINS_SYMBOL, "J_M_J");
    boost::filesystem::path jmj_path = boost::filesystem::temp_directory_path();
    jmj_path /= "J_M_J";
    boost::filesystem::create_directories(jmj_path);
    mapArgs["-datadir"] = jmj_path.string();
    ClearDatadirCache();
    expected = "/tmp/J_M_J";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with str filled in
    strcpy(str, "blah.txt");
    expected = "/tmp/J_M_Jblah.txt";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, expected);
    // with ASSETCHAINS_SYMBOL filled in with unexpected value
    str[0] = 0;
    strcpy(ASSETCHAINS_SYMBOL, "JMJ");
    expected = "/tmp/J_M_J";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "REGTEST"
    strcpy(symbol, "REGTEST");
    expected = "/tmp/J_M_J/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL filled in with unexpected value and symbol filled in with "KMD"
    strcpy(symbol, "KMD");
    expected = "/tmp/J_M_J";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));
    // with ASSETCHAIMS_SYMBOL empty and symbol filled in with "KMD"
    ASSETCHAINS_SYMBOL[0] = 0;
    strcpy(symbol, "KMD");
    expected = "/tmp/J_M_J/";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));

    // typical use case
    mapArgs["-datadir"] = jmj_path.string() + boost::filesystem::path::preferred_separator;
    ClearDatadirCache();
    strcpy(ASSETCHAINS_SYMBOL, "J_M_J");
    strcpy(str, "komodostate");
    expected = "/tmp/J_M_J/komodostate";
    fname = komodo_statefname(symbol, str);
    ASSERT_EQ(fname, std::string(expected));

#endif
}

TEST(TestConfig, configFile)
{
#ifdef __linux__
    boost::filesystem::path badpath("/root/.jmjcoin");
    boost::filesystem::path noexist_path = boost::filesystem::temp_directory_path() / "jmjBlahBlah.txt";
    boost::filesystem::path good_path = boost::filesystem::temp_directory_path() / "jmjGoodFile.conf";
    if ( boost::filesystem::exists(noexist_path) )
        boost::filesystem::remove( noexist_path );
    if ( boost::filesystem::exists( good_path ) )
        boost::filesystem::remove( good_path );
    
    // try to read a file we probably can't get to
    try
    {
        ConfigFile test(badpath);
        ASSERT_TRUE(false);
    }
    catch(const std::ios_base::failure&)
    {
    }
    // try to read a file that does not exist
    try
    {
        ConfigFile not_exist(noexist_path);
        ASSERT_TRUE(false);
    }
    catch(const std::ios_base::failure&)
    {
    }
    // try to create a good config file
    try
    {
        ConfigFile good;
        ASSERT_FALSE( good.Has("rpcport") );
        std::multimap<std::string, std::string> entries;
        entries.emplace("rpcport", "123");
        entries.emplace("rpcuser", "abc");
        entries.emplace("rpcpassword", "def");
        entries.emplace("Hello", "World!");
        entries.emplace("Hello", "Again!");
        good.SetEntries(entries);
        ASSERT_TRUE( good.Save(good_path, false) );
    }
    catch(const std::ios_base::failure& ex)
    {
        ASSERT_EQ(std::string(), std::string(ex.what()));
        ASSERT_TRUE(false);
    }
    // now try to read the file we just created
    ConfigFile good( good_path );
    ASSERT_TRUE( good.Has("rpcport") );
    ASSERT_EQ( good.Value("rpcport"), "123");
    ASSERT_EQ( good.Value("rpcuser"), "abc" );
    ASSERT_EQ( good.Value("rpcpassword"), "def");
    ASSERT_EQ( good.Value("Hello"), "World!");
    std::vector<std::string> values = good.Values("Hello");
    ASSERT_EQ( values.size(), 2 );
    ASSERT_EQ( good.Entries().size(), 5 );
    // now clean up the mess
    boost::filesystem::remove( good_path );
#endif
}

} // namespace TestConfig
