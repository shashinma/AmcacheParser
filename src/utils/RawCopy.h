#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace amcache {

struct LockedFileResult {
    std::vector<uint8_t> hiveData;
    std::vector<std::vector<uint8_t>> logData;
};

class RawCopy {
public:
    static std::optional<std::vector<uint8_t>> ReadLockedFile(const std::string& path);
    static std::optional<LockedFileResult> ReadLockedFileWithLogs(const std::string& path);
    static bool IsAdministrator();
    static bool IsFileLocked(const std::string& path);
    static bool CopyLockedFile(const std::string& sourcePath, const std::string& destPath);

#ifdef _WIN32
    static std::optional<std::vector<uint8_t>> ReadViaVSS(const std::string& path);
    static std::optional<std::vector<uint8_t>> ReadViaNTFS(const std::string& path);
    static std::optional<std::vector<uint8_t>> ReadViaVolumeDirect(const std::string& path);
    static std::optional<std::vector<uint8_t>> ReadWithBackupPrivileges(const std::string& path);
#endif
};

} // namespace amcache
