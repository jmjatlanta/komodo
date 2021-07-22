/******
 * Takes care of reading config files
 * files must be text files in key=value format
 * Comments are allowed if the line has # as the first character
 * Lines without = are ignored
 * Entries with matching keys are valid
 */
#pragma once
#include <boost/filesystem.hpp>
#include <map>
#include <string>

class ConfigFile
{
public:
    /****
     * Read an existing config file
     * @param file the file
     */
    ConfigFile(const boost::filesystem::path& file);
    /*****
     * @returns all entries
     */
    std::multimap<std::string, std::string> GetEntries();
    /*****
     * @param key the key
     * @returns the first entry that matches
     */
    std::string GetFirstEntry(const std::string& key);
    /******
     * @param key the key
     * @returns all entries that match the key
     */
    std::vector<std::string> GetEntries(const std::string& key);
    /****
     * Overwrite internal collection with this one
     * @param in the new collection
     */
    void SetEntries(const std::multimap<std::string, std::string>& in);
    /******
     * @brief write the values to a file
     * @param file the file
     * @param overwrite false will not permit overwriting an existing file
     * @returns true on success
     */
    bool Write(const boost::filesystem::path& file, bool overwrite);
private:
    std::multimap<std::string, std::string> entries;
};