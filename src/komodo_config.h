#pragma once
/******************************************************************************
 * Copyright © 2021 Komodo Core developers                                    *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo software, including this file may be copied, modified, propagated   *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#ifdef __cplusplus

#include <boost/filesystem.hpp>
#include <string>
#include <map>
#include <vector>

class ConfigFile
{
public:
    /****
     * default ctor
     */
    ConfigFile();
    /****
     * Read a configuation file
     * @param path the full path to the file
     * @throws std::ioexception if path is invalid or the file cannot be read
     */
    ConfigFile(const boost::filesystem::path& path);
    /****
     * @param key the key
     * @returns true if the key was found in the file
     */
    bool Has(const std::string& key) const;
    /******
     * @param key the key
     * @returns the first value that matches the key (empty string if not found)
     */
    std::string Value(const std::string& key) const;
    /**********
     * @param key the key
     * @returns all values for that key (empty vector if not found)
     */
    std::vector<std::string> Values(const std::string& key) const;
    /******
     * @returns all entries found in the file
     */
    std::multimap<std::string, std::string> Entries() const;
    /*****
     * @param in the collection to replace the existing entries
     */
    void SetEntries(const std::multimap<std::string, std::string>& in);
    /*******
     * @brief writes entries to a file
     * @param path the path to the file
     * @param overwrite true to overwrite existing file
     */
    bool Save(const boost::filesystem::path& path, bool overwrite) const;
private:
    std::multimap<std::string, std::string> entries;
};

#endif
