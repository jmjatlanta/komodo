/****
 * Test the configuration
 */
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

std::string komodo_statefname(const std::string& symbol, const std::string& str);
void ClearDatadirCache();
boost::filesystem::path GetDefaultDataDir();
extern char ASSETCHAINS_SYMBOL[65];
extern std::map<std::string, std::string> mapArgs;

namespace TestConfig
{

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
#ifdef __linux__
    std::string fname;
    // gather the expected results
    char* home_dir = getenv("HOME");
    char symbol[32];
    symbol[0] = 0;
    char str[32];
    str[0] = 0;
    ASSETCHAINS_SYMBOL[0] = 0;
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

} // namespace TestConfig
