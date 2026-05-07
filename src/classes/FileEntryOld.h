#pragma once

#include "Common.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace amcache {

struct FileEntryOld {
    int32_t MFTEntryNumber = 0;
    int32_t MFTSequenceNumber = 0;
    std::string ProductName;
    std::string CompanyName;
    std::string FileVersionString;
    std::string FileVersionNumber;
    std::string FileDescription;
    std::string FullPath;
    std::string FileExtension;
    std::string PEHeaderHash;
    std::string ProgramID;
    std::string SHA1;
    std::string FileID;
    std::string VolumeID;
    std::string SwitchBackContext;
    std::string ProgramName;
    int64_t BinProductVersion = 0;
    uint64_t BinFileVersion = 0;
    int32_t LinkerVersion = 0;
    int32_t BinaryType = 0;
    int32_t IsLocal = 0;
    int32_t GuessProgramID = 0;
    std::optional<int32_t> LanguageID;
    std::optional<int32_t> FileSize;
    std::optional<int32_t> SizeOfImage;
    std::optional<uint32_t> PEHeaderChecksum;
    Timestamp VolumeIDLastWriteTimestamp;
    Timestamp FileIDLastWriteTimestamp;
    OptionalTimestamp LinkDate;
    OptionalTimestamp LastModified;
    OptionalTimestamp LastModifiedStore;
    OptionalTimestamp Created;

    FileEntryOld() = default;

    static void ParseMFTEntry(const std::string& keyName, int32_t& entryNumber, int32_t& sequenceNumber) {
        // Strip dashes and trim whitespace (matches original .NET behavior)
        std::string cleaned;
        for (char c : keyName) {
            if (c != '-' && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                cleaned += c;
            }
        }

        if (cleaned.length() >= 8) {
            try {
                std::string padded = cleaned;
                while (padded.length() < 8) {
                    padded = "0" + padded;
                }

                std::string seqStr = padded.substr(0, 4);
                std::string entryStr = padded.substr(padded.length() - 4);

                sequenceNumber = static_cast<int32_t>(std::stoul(seqStr, nullptr, 16));
                entryNumber = static_cast<int32_t>(std::stoul(entryStr, nullptr, 16));
            } catch (...) {
                entryNumber = 0;
                sequenceNumber = 0;
            }
        }
    }

    static std::string ProcessSha1(const std::string& sha1) {
        if (sha1.length() > 4) {
            std::string result = sha1.substr(4);
            std::transform(result.begin(), result.end(), result.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return result;
        }
        return sha1;
    }

    static std::string ExtractFileExtension(const std::string& path) {
        auto pos = path.rfind('.');
        if (pos != std::string::npos && pos < path.length() - 1) {
            return path.substr(pos);
        }
        return "";
    }
};

} // namespace amcache
