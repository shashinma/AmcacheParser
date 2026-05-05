#include "AmcacheOld.h"
#include "Helper.h"
#include "../utils/DateTimeUtils.h"
#include <algorithm>

namespace amcache {

AmcacheOld::ParseResult AmcacheOld::Parse(const RegistryHive& hive,
                                          bool includeLinkedFiles,
                                          const std::set<std::string>& whitelist,
                                          const std::set<std::string>& blacklist) {
    ParseResult result;

    try {
        result.ProgramsEntries = ParsePrograms(hive);
        auto allFiles = ParseFiles(hive);
        result.TotalFileEntries = static_cast<int>(allFiles.size());

        ApplyFilters(allFiles, whitelist, blacklist);

        LinkFilesToPrograms(result.ProgramsEntries, allFiles,
                           result.UnassociatedFileEntries,
                           result.AssociatedFileEntries);

        if (!includeLinkedFiles) {
            for (auto& program : result.ProgramsEntries) {
                program.FileEntries.clear();
            }
        }

        result.Success = true;
    } catch (const std::exception& e) {
        result.Success = false;
        result.ErrorMessage = e.what();
    }

    return result;
}

std::vector<ProgramsEntryOld> AmcacheOld::ParsePrograms(const RegistryHive& hive) {
    std::vector<ProgramsEntryOld> programs;

    auto programsKey = hive.GetKeyByPath(PROGRAMS_PATH);
    if (!programsKey.IsValid()) {
        return programs;
    }

    auto subkeys = programsKey.GetSubKeys();
    programs.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        try {
            auto entry = ParseProgramEntry(subkey);
            programs.push_back(std::move(entry));
        } catch (...) {
            // Skip malformed entries
        }
    }

    return programs;
}

ProgramsEntryOld AmcacheOld::ParseProgramEntry(const RegistryKey& key) {
    ProgramsEntryOld entry;

    entry.ProgramID = key.GetName();
    entry.LastWriteTimestamp = key.GetLastWriteTime();

    if (auto val = key.GetStringValue("0")) entry.ProgramName_0 = *val;
    if (auto val = key.GetStringValue("1")) entry.ProgramVersion_1 = *val;
    if (auto val = key.GetStringValue("2")) entry.VendorName_2 = *val;
    if (auto val = key.GetStringValue("3")) entry.LanguageCode_3 = *val;
    if (auto val = key.GetStringValue("6")) entry.InstallSource_6 = *val;
    if (auto val = key.GetStringValue("7")) entry.UninstallRegistryKey_7 = *val;
    if (auto val = key.GetStringValue("d")) entry.PathsList_d = *val;
    if (auto val = key.GetStringValue("f")) entry.UninstallGuid_f = *val;
    if (auto val = key.GetStringValue("10")) entry.UnknownGuid_10 = *val;
    if (auto val = key.GetStringValue("11")) entry.UninstallGuid_11 = *val;
    if (auto val = key.GetStringValue("12")) entry.UnknownGuid_12 = *val;

    if (auto val = key.GetInt32Value("5")) entry.UnknownDword_5 = *val;
    if (auto val = key.GetInt32Value("13")) entry.UnknownDword_13 = *val;
    if (auto val = key.GetInt32Value("14")) entry.UnknownDword_14 = *val;
    if (auto val = key.GetInt32Value("15")) entry.UnknownDword_15 = *val;
    if (auto val = key.GetInt32Value("18")) entry.UnknownDword_18 = *val;

    if (auto val = key.GetInt64Value("17")) entry.UnknownQWord_17 = *val;

    if (auto val = key.GetInt64Value("a")) {
        if (*val > 0) {
            entry.InstallDateEpoch_a = DateTimeUtils::FromUnixSeconds(*val);
        }
    }
    if (auto val = key.GetInt64Value("b")) {
        if (*val > 0) {
            entry.InstallDateEpoch_b = DateTimeUtils::FromUnixSeconds(*val);
        }
    }

    if (auto val = key.GetBinaryValue("16")) {
        entry.UnknownBytes_16 = *val;
    }

    if (auto val = key.GetStringValue("Files")) {
        entry.FilesLinks = ProgramsEntryOld::ParseFilesLinks(*val);
    }

    return entry;
}

std::vector<FileEntryOld> AmcacheOld::ParseFiles(const RegistryHive& hive) {
    std::vector<FileEntryOld> files;

    auto filesKey = hive.GetKeyByPath(FILES_PATH);
    if (!filesKey.IsValid()) {
        return files;
    }

    auto volumeKeys = filesKey.GetSubKeys();

    for (auto& volumeKey : volumeKeys) {
        std::string volumeId = volumeKey.GetName();
        Timestamp volumeLastWrite = volumeKey.GetLastWriteTime();

        auto fileKeys = volumeKey.GetSubKeys();

        for (auto& fileKey : fileKeys) {
            try {
                auto entry = ParseFileEntry(fileKey, volumeId, volumeLastWrite);
                if (!entry.FullPath.empty()) {
                    files.push_back(std::move(entry));
                }
            } catch (...) {
                // Skip malformed entries
            }
        }
    }

    return files;
}

FileEntryOld AmcacheOld::ParseFileEntry(const RegistryKey& key,
                                         const std::string& volumeId,
                                         const Timestamp& volumeLastWrite) {
    FileEntryOld entry;

    std::string keyName = key.GetName();
    entry.FileID = keyName;
    entry.VolumeID = volumeId;
    entry.VolumeIDLastWriteTimestamp = volumeLastWrite;
    entry.FileIDLastWriteTimestamp = key.GetLastWriteTime();

    FileEntryOld::ParseMFTEntry(keyName, entry.MFTEntryNumber, entry.MFTSequenceNumber);

    if (auto val = key.GetStringValue("0")) entry.ProductName = *val;
    if (auto val = key.GetStringValue("1")) entry.CompanyName = *val;
    if (auto val = key.GetStringValue("2")) entry.FileVersionNumber = *val;
    if (auto val = key.GetStringValue("4")) entry.SwitchBackContext = *val;
    if (auto val = key.GetStringValue("5")) entry.FileVersionString = *val;
    if (auto val = key.GetStringValue("8")) entry.PEHeaderHash = *val;
    if (auto val = key.GetStringValue("c")) entry.FileDescription = *val;
    if (auto val = key.GetStringValue("15")) {
        entry.FullPath = *val;
        entry.FileExtension = FileEntryOld::ExtractFileExtension(*val);
    }
    if (auto val = key.GetStringValue("100")) entry.ProgramID = *val;
    if (auto val = key.GetStringValue("101")) {
        entry.SHA1 = FileEntryOld::ProcessSha1(*val);
    }

    if (auto val = key.GetInt32Value("3")) entry.LanguageID = *val;
    if (auto val = key.GetInt32Value("6")) entry.FileSize = *val;
    if (auto val = key.GetInt32Value("7")) entry.SizeOfImage = *val;
    if (auto val = key.GetUInt32Value("9")) entry.PEHeaderChecksum = *val;
    if (auto val = key.GetInt32Value("d")) entry.LinkerVersion = *val;
    if (auto val = key.GetInt32Value("10")) entry.BinaryType = *val;
    if (auto val = key.GetInt32Value("16")) entry.IsLocal = *val;
    if (auto val = key.GetInt32Value("106")) entry.GuessProgramID = *val;

    if (auto val = key.GetInt64Value("a")) entry.BinProductVersion = *val;
    if (auto val = key.GetUInt64Value("b")) entry.BinFileVersion = *val;

    if (auto val = key.GetInt64Value("f")) {
        if (*val > 0) {
            entry.LinkDate = DateTimeUtils::FromUnixSeconds(*val);
        }
    }

    if (auto val = key.GetUInt64Value("11")) {
        entry.LastModified = DateTimeUtils::FromFileTimeSafe(*val);
    }
    if (auto val = key.GetUInt64Value("12")) {
        entry.Created = DateTimeUtils::FromFileTimeSafe(*val);
    }
    if (auto val = key.GetUInt64Value("17")) {
        entry.LastModifiedStore = DateTimeUtils::FromFileTimeSafe(*val);
    }

    return entry;
}

void AmcacheOld::LinkFilesToPrograms(std::vector<ProgramsEntryOld>& programs,
                                      std::vector<FileEntryOld>& files,
                                      std::vector<FileEntryOld>& unassociated,
                                      std::vector<FileEntryOld>& associated) {
    std::map<std::string, size_t> programMap;
    for (size_t i = 0; i < programs.size(); ++i) {
        programMap[programs[i].ProgramID] = i;
    }

    for (auto& file : files) {
        if (!file.ProgramID.empty()) {
            auto it = programMap.find(file.ProgramID);
            if (it != programMap.end()) {
                file.ProgramName = programs[it->second].ProgramName_0;
                programs[it->second].FileEntries.push_back(file);
                associated.push_back(std::move(file));
                continue;
            }
        }
        file.ProgramName = "Unassociated";
        unassociated.push_back(std::move(file));
    }
}

void AmcacheOld::ApplyFilters(std::vector<FileEntryOld>& files,
                              const std::set<std::string>& whitelist,
                              const std::set<std::string>& blacklist) {
    if (whitelist.empty() && blacklist.empty()) {
        return;
    }

    files.erase(
        std::remove_if(files.begin(), files.end(),
            [&whitelist, &blacklist](const FileEntryOld& file) {
                if (Helper::IsInBlacklist(file.SHA1, blacklist)) {
                    return true;
                }
                if (!whitelist.empty() && !Helper::IsInWhitelist(file.SHA1, whitelist)) {
                    return true;
                }
                return false;
            }),
        files.end());
}

} // namespace amcache
