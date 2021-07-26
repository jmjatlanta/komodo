#include <fstream>
#include <stdexcept>
#include "komodo_config.h"

ConfigFile::ConfigFile()
{
}

ConfigFile::ConfigFile(const boost::filesystem::path& path)
{
    std::ifstream in;
    in.exceptions( std::ifstream::failbit | std::ifstream::badbit);
    in.open(path.string());
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
        return (*itr).second;
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
