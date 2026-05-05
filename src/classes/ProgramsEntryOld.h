#pragma once

#include "Common.h"
#include "FileEntryOld.h"
#include <vector>
#include <sstream>

namespace amcache {

struct FilesProgramEntry {
    std::string SourceGuid;
    std::string FileEntry;
};

struct ProgramsEntryOld {
    std::string ProgramName_0;
    std::string ProgramVersion_1;
    std::string VendorName_2;
    std::string LanguageCode_3;
    std::string InstallSource_6;
    std::string UninstallRegistryKey_7;
    std::string UnknownGuid_10;
    std::string UnknownGuid_12;
    std::string UninstallGuid_11;
    int32_t UnknownDword_5 = 0;
    int32_t UnknownDword_13 = 0;
    int32_t UnknownDword_14 = 0;
    int32_t UnknownDword_15 = 0;
    std::vector<uint8_t> UnknownBytes_16;
    int64_t UnknownQWord_17 = 0;
    int32_t UnknownDword_18 = 0;
    OptionalTimestamp InstallDateEpoch_a;
    OptionalTimestamp InstallDateEpoch_b;
    std::string PathsList_d;
    std::string UninstallGuid_f;
    std::vector<FilesProgramEntry> FilesLinks;
    std::string ProgramID;
    Timestamp LastWriteTimestamp;
    std::vector<FileEntryOld> FileEntries;

    ProgramsEntryOld() = default;

    static std::vector<FilesProgramEntry> ParseFilesLinks(const std::string& rawFilesList) {
        std::vector<FilesProgramEntry> result;
        if (rawFilesList.empty()) {
            return result;
        }

        std::istringstream iss(rawFilesList);
        std::string token;

        while (iss >> token) {
            auto atPos = token.find('@');
            if (atPos != std::string::npos) {
                FilesProgramEntry entry;
                entry.SourceGuid = token.substr(0, atPos);
                entry.FileEntry = token.substr(atPos + 1);
                result.push_back(std::move(entry));
            }
        }

        return result;
    }
};

} // namespace amcache
