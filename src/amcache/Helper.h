#pragma once

#include <string>
#include <vector>
#include <set>
#include "../registry/RegistryWrapper.h"

namespace amcache {

enum class AmcacheFormat {
    Unknown,
    Old,
    New
};

class Helper {
public:
    static bool IsNewFormat(const RegistryHive& hive);
    static AmcacheFormat GetFormat(const RegistryHive& hive);
    static std::set<std::string> LoadHashList(const std::string& path);
    static bool IsInWhitelist(const std::string& sha1, const std::set<std::string>& whitelist);
    static bool IsInBlacklist(const std::string& sha1, const std::set<std::string>& blacklist);
    static std::vector<std::string> FindTransactionLogs(const std::string& hivePath);
    static std::string GenerateOutputFilename(const std::string& timestamp,
                                               const std::string& hiveName,
                                               const std::string& suffix,
                                               const std::string& extension = ".csv");
    static std::string GetBaseName(const std::string& path);
    static std::string GetTimestampString();

private:
    static constexpr const char* OLD_FORMAT_PATH = "Root\\File";
    static constexpr const char* NEW_FORMAT_PATH = "Root\\InventoryApplicationFile";
};

} // namespace amcache
