#include <iostream>
#include "komodo_config.h"

ConfigFile::ConfigFile(const boost::filesystem::path& file)
{
    std::ifstream in(file.string());
    std::string line;
    while(std::getline(in, line))
    {
        if (line.find('#') == 0)
            continue;
        size_t pos = line.find('=');
        if (pos != std::string::npos && pos != line.size()-1)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos+1);
            entries.emplace(key, value);
        }
    }
}

std::multimap<std::string, std::string> ConfigFile::GetEntries()
{
    return entries;
}

std::string ConfigFile::GetFirstEntry(const std::string& key)
{
    auto itr = entries.find(key);
    if (itr != entries.end())
        return (*itr).second;
    return "";
}

std::vector<std::string> ConfigFile::GetEntries(const std::string& key)
{
    std::vector<std::string> retval;
    for( auto itr = entries.find(key); itr != entries.end(); ++itr)
    {
        retval.push_back( (*itr).second );
    }
    return retval;
}

void ConfigFile::SetEntries(const std::multimap<std::string, std::string>& in)
{
    entries = in;
}

bool ConfigFile::Write(const boost::filesystem::path& file, bool overwrite)
{
    if (!overwrite && boost::filesystem::exists(file))
        return false;
    std::ofstream out(file.string(), std::ofstream::trunc);
    if (!out)
        return false;
    for(auto itr = entries.begin(); itr != entries.end(); ++itr)
    {
        out << (*itr).first << "=" << (*itr).second << "\n";
    }
    return true;
}