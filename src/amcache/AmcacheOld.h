#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "../classes/FileEntryOld.h"
#include "../classes/ProgramsEntryOld.h"
#include "../registry/RegistryWrapper.h"

namespace amcache {

class AmcacheOld {
public:
    struct ParseResult {
        std::vector<ProgramsEntryOld> ProgramsEntries;
        std::vector<FileEntryOld> UnassociatedFileEntries;
        std::vector<FileEntryOld> AssociatedFileEntries;
        int TotalFileEntries = 0;
        bool Success = false;
        std::string ErrorMessage;
    };

    static ParseResult Parse(const RegistryHive& hive,
                            bool includeLinkedFiles,
                            const std::set<std::string>& whitelist,
                            const std::set<std::string>& blacklist);

private:
    static std::vector<ProgramsEntryOld> ParsePrograms(const RegistryHive& hive);
    static ProgramsEntryOld ParseProgramEntry(const RegistryKey& key);
    static std::vector<FileEntryOld> ParseFiles(const RegistryHive& hive);
    static FileEntryOld ParseFileEntry(const RegistryKey& key,
                                        const std::string& volumeId,
                                        const Timestamp& volumeLastWrite);
    static void LinkFilesToPrograms(std::vector<ProgramsEntryOld>& programs,
                                    std::vector<FileEntryOld>& files,
                                    std::vector<FileEntryOld>& unassociated,
                                    std::vector<FileEntryOld>& associated);
    static void ApplyFilters(std::vector<FileEntryOld>& files,
                            const std::set<std::string>& whitelist,
                            const std::set<std::string>& blacklist);

    static constexpr const char* PROGRAMS_PATH = "Root\\Programs";
    static constexpr const char* FILES_PATH = "Root\\File";
};

} // namespace amcache
