#pragma once

#include "Common.h"
#include <algorithm>
#include <cctype>

namespace amcache {

struct FileEntryNew {
    std::string BinaryType;
    std::string BinFileVersion;
    std::string BinProductVersion;
    std::string SHA1;
    bool IsOsComponent = false;
    bool IsPeFile = false;
    int32_t Language = 0;
    OptionalTimestamp LinkDate;
    std::string LongPathHash;
    std::string FullPath;
    std::string Name;
    std::string ApplicationName;
    std::string ProductName;
    std::string ProductVersion;
    std::string ProgramId;
    std::string Publisher;
    int64_t Size = 0;
    uint64_t Usn = 0;
    std::string Version;
    std::string FileExtension;
    std::string Description;
    std::string OriginalFileName;
    Timestamp FileKeyLastWriteTimestamp;

    FileEntryNew() = default;

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
            // Make sure the dot is part of the filename, not a directory path
            auto lastSep = path.find_last_of("/\\");
            if (lastSep == std::string::npos || pos > lastSep) {
                return path.substr(pos);
            }
        }
        return "";
    }
};

} // namespace amcache
