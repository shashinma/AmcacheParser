#include "RawCopy.h"
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace amcache {

static void TryReadLogs(const std::string& hivePath, std::vector<std::vector<uint8_t>>& logData) {
    namespace fs = std::filesystem;
    fs::path p(hivePath);
    fs::path dir = p.parent_path();
    if (dir.empty()) dir = ".";
    std::string base = p.stem().string();

    for (const auto& ext : {".LOG1", ".LOG2"}) {
        fs::path logPath = dir / (base + ext);
        std::ifstream file(logPath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(size));
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                logData.push_back(std::move(buffer));
            }
        }
    }
}

std::optional<std::vector<uint8_t>> RawCopy::ReadLockedFile(const std::string& path) {
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(static_cast<size_t>(size));
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                return buffer;
            }
        }
    }

#ifdef _WIN32
    if (!IsAdministrator()) {
        return std::nullopt;
    }

    auto result = ReadViaVSS(path);
    if (result.has_value()) {
        return result;
    }

    result = ReadViaNTFS(path);
    if (result.has_value()) {
        return result;
    }
#endif

    return std::nullopt;
}

std::optional<LockedFileResult> RawCopy::ReadLockedFileWithLogs(const std::string& path) {
    LockedFileResult result;

    // Try normal read for hive
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            result.hiveData.resize(static_cast<size_t>(size));
            if (file.read(reinterpret_cast<char*>(result.hiveData.data()), size)) {
                // Try reading logs normally too
                TryReadLogs(path, result.logData);
                return result;
            }
        }
    }

#ifdef _WIN32
    if (!IsAdministrator()) {
        return std::nullopt;
    }

    auto vssResult = ReadViaVSS(path);
    if (vssResult.has_value()) {
        result.hiveData = std::move(*vssResult);
        // For VSS, logs may not be accessible via the same snapshot path easily
        // Try normal read first, then same VSS method
        TryReadLogs(path, result.logData);
        return result;
    }

    auto ntfsResult = ReadViaNTFS(path);
    if (ntfsResult.has_value()) {
        result.hiveData = std::move(*ntfsResult);
        TryReadLogs(path, result.logData);
        return result;
    }

    auto backupResult = ReadWithBackupPrivileges(path);
    if (backupResult.has_value()) {
        result.hiveData = std::move(*backupResult);
        TryReadLogs(path, result.logData);
        return result;
    }
#endif

    return std::nullopt;
}

bool RawCopy::IsAdministrator() {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = nullptr;

    if (AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &AdministratorsGroup)) {
        CheckTokenMembership(nullptr, AdministratorsGroup, &isAdmin);
        FreeSid(AdministratorsGroup);
    }
    return isAdmin != FALSE;
#else
    return geteuid() == 0;
#endif
}

bool RawCopy::IsFileLocked(const std::string& path) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(
        path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION) {
            return true;
        }
    } else {
        CloseHandle(hFile);
    }
    return false;
#else
    return !std::filesystem::exists(path);
#endif
}

bool RawCopy::CopyLockedFile(const std::string& sourcePath, const std::string& destPath) {
    auto data = ReadLockedFile(sourcePath);
    if (!data.has_value()) {
        return false;
    }
    std::ofstream outFile(destPath, std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(data->data()),
                  static_cast<std::streamsize>(data->size()));
    return outFile.good();
}

#ifdef _WIN32

std::optional<std::vector<uint8_t>> RawCopy::ReadViaVSS(const std::string& path) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        CoUninitialize();
        return std::nullopt;
    }

    IVssBackupComponents* pBackup = nullptr;
    hr = CreateVssBackupComponents(&pBackup);
    if (FAILED(hr)) {
        CoUninitialize();
        return std::nullopt;
    }

    hr = pBackup->InitializeForBackup();
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    hr = pBackup->SetBackupState(true, false, VSS_BT_COPY, false);
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    IVssAsync* pAsync = nullptr;
    hr = pBackup->GatherWriterMetadata(&pAsync);
    if (SUCCEEDED(hr) && pAsync) {
        pAsync->Wait();
        pAsync->Release();
    }

    char volumePath[MAX_PATH];
    if (!GetVolumePathNameA(path.c_str(), volumePath, MAX_PATH)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, volumePath, -1, nullptr, 0);
    std::wstring wideVolume(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, volumePath, -1, &wideVolume[0], wideLen);

    VSS_ID snapshotSetId;
    hr = pBackup->StartSnapshotSet(&snapshotSetId);
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    VSS_ID snapshotId;
    hr = pBackup->AddToSnapshotSet(const_cast<wchar_t*>(wideVolume.c_str()), GUID_NULL, &snapshotId);
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    hr = pBackup->PrepareForBackup(&pAsync);
    if (SUCCEEDED(hr) && pAsync) {
        pAsync->Wait();
        pAsync->Release();
    }

    hr = pBackup->DoSnapshotSet(&pAsync);
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    if (pAsync) {
        pAsync->Wait();
        pAsync->Release();
    }

    VSS_SNAPSHOT_PROP prop;
    hr = pBackup->GetSnapshotProperties(snapshotId, &prop);
    if (FAILED(hr)) {
        pBackup->Release();
        CoUninitialize();
        return std::nullopt;
    }

    std::wstring widePath;
    int widePathLen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    widePath.resize(widePathLen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &widePath[0], widePathLen);

    std::wstring shadowPath = prop.m_pwszSnapshotDeviceObject;
    shadowPath += L"\\";
    shadowPath += widePath.substr(wcslen(wideVolume.c_str()));

    HANDLE hFile = CreateFileW(
        shadowPath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    std::optional<std::vector<uint8_t>> result;

    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(hFile, &fileSize)) {
            std::vector<uint8_t> buffer(static_cast<size_t>(fileSize.QuadPart));
            DWORD bytesRead = 0;
            if (ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)) {
                buffer.resize(bytesRead);
                result = std::move(buffer);
            }
        }
        CloseHandle(hFile);
    }

    VssFreeSnapshotProperties(&prop);
    pBackup->Release();
    CoUninitialize();
    return result;
}

std::optional<std::vector<uint8_t>> RawCopy::ReadViaNTFS(const std::string& path) {
    HANDLE hFile = CreateFileA(
        path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_NO_BUFFERING,
        nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(hFile, &fileSize)) {
            const size_t maxChunkSize = 64 * 1024 * 1024;
            size_t totalSize = static_cast<size_t>(fileSize.QuadPart);
            std::vector<uint8_t> buffer;
            buffer.reserve(totalSize);

            size_t bytesRead = 0;
            size_t chunkSize = (totalSize > maxChunkSize) ? maxChunkSize : totalSize;
            std::vector<uint8_t> chunk(chunkSize);

            while (bytesRead < totalSize) {
                DWORD dwBytesRead = 0;
                size_t currentChunk = (totalSize - bytesRead > chunkSize) ? chunkSize : (totalSize - bytesRead);
                if (ReadFile(hFile, chunk.data(), static_cast<DWORD>(currentChunk), &dwBytesRead, nullptr)) {
                    buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + dwBytesRead);
                    bytesRead += dwBytesRead;
                    if (dwBytesRead == 0) break;
                } else {
                    break;
                }
            }

            CloseHandle(hFile);
            if (!buffer.empty()) {
                return buffer;
            }
        } else {
            CloseHandle(hFile);
        }
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>> RawCopy::ReadViaVolumeDirect(const std::string& path) {
    return ReadWithBackupPrivileges(path);
}

std::optional<std::vector<uint8_t>> RawCopy::ReadWithBackupPrivileges(const std::string& path) {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return std::nullopt;
    }

    TOKEN_PRIVILEGES privileges = {0};
    privileges.PrivilegeCount = 2;
    LookupPrivilegeValueA(nullptr, SE_BACKUP_NAME, &privileges.Privileges[0].Luid);
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    LookupPrivilegeValueA(nullptr, SE_RESTORE_NAME, &privileges.Privileges[1].Luid);
    privileges.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    bool privilegesEnabled = AdjustTokenPrivileges(hToken, FALSE, &privileges, 0, nullptr, nullptr) != 0;
    DWORD lastError = GetLastError();

    if (!privilegesEnabled || lastError != ERROR_SUCCESS) {
        CloseHandle(hToken);
        return std::nullopt;
    }

    HANDLE hFile = CreateFileA(
        path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_NO_BUFFERING,
        nullptr);

    std::optional<std::vector<uint8_t>> result;

    if (hFile != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(hFile, &fileSize)) {
            const size_t maxChunkSize = 64 * 1024 * 1024;
            size_t totalSize = static_cast<size_t>(fileSize.QuadPart);
            std::vector<uint8_t> buffer;
            buffer.reserve(totalSize);

            size_t bytesRead = 0;
            size_t chunkSize = (totalSize > maxChunkSize) ? maxChunkSize : totalSize;
            std::vector<uint8_t> chunk(chunkSize);

            while (bytesRead < totalSize) {
                DWORD dwBytesRead = 0;
                size_t currentChunk = (totalSize - bytesRead > chunkSize) ? chunkSize : (totalSize - bytesRead);
                if (ReadFile(hFile, chunk.data(), static_cast<DWORD>(currentChunk), &dwBytesRead, nullptr)) {
                    buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + dwBytesRead);
                    bytesRead += dwBytesRead;
                    if (dwBytesRead == 0) break;
                } else {
                    break;
                }
            }

            if (!buffer.empty()) {
                result = std::move(buffer);
            }
        }
        CloseHandle(hFile);
    }

    privileges.Privileges[0].Attributes = 0;
    privileges.Privileges[1].Attributes = 0;
    AdjustTokenPrivileges(hToken, FALSE, &privileges, 0, nullptr, nullptr);
    CloseHandle(hToken);
    return result;
}

#endif // _WIN32

} // namespace amcache
