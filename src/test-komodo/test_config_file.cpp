#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include "komodo_config.h"

namespace ConfigFileTest
{

TEST(ConfigFileTest, open_file)
{
    // file does not exist
    boost::filesystem::path not_exist = boost::filesystem::temp_directory_path() / "should_not_exist.jmj";
    ASSERT_FALSE( boost::filesystem::exists(not_exist) );
    ConfigFile nada(not_exist);
    ASSERT_EQ( nada.GetEntries().size(), 0 );
    // create file
    std::multimap<std::string, std::string> input;
    input.emplace("E", "MCsquared");
    input.emplace("Hello", "World!");
    input.emplace("Hello", "Again");
    nada.SetEntries(input);
    boost::filesystem::path created = boost::filesystem::temp_directory_path() / "created.jmj";
    nada.Write(created, true);
    // read file
    ConfigFile exists(created);
    auto results = exists.GetEntries();
    ASSERT_EQ( results.size(), 3);
    ASSERT_EQ( exists.GetFirstEntry("Hello"), "World!" );
    auto coll = exists.GetEntries("Hello");
    ASSERT_EQ( coll.size(), 2);
    boost::filesystem::remove(created);
}

} // namespace ConfigFileTest