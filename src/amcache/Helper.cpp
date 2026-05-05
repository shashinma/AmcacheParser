#include "Helper.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace amcache {

bool Helper::IsNewFormat(const RegistryHive& hive) {
    if (hive.KeyExists(NEW_FORMAT_PATH)) {
        return true;
    }
    if (hive.KeyExists(OLD_FORMAT_PATH)) {
        return false;
    }
    return true;
}

AmcacheFormat Helper::GetFormat(const RegistryHive& hive) {
    if (hive.KeyExists(NEW_FORMAT_PATH)) {
        return AmcacheFormat::New;
    }
    if (hive.KeyExists(OLD_FORMAT_PATH)) {
        return AmcacheFormat::Old;
    }
    return AmcacheFormat::Unknown;
}

std::set<std::string> Helper::LoadHashList(const std::string& path) {
    std::set<std::string> hashes;
    std::ifstream file(path);
    if (!file.is_open()) {
        return hashes;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::transform(line.begin(), line.end(), line.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        hashes.insert(line);
    }
    return hashes;
}

bool Helper::IsInWhitelist(const std::string& sha1, const std::set<std::string>& whitelist) {
    if (whitelist.empty()) {
        return true;
    }
    std::string lowerSha1 = sha1;
    std::transform(lowerSha1.begin(), lowerSha1.end(), lowerSha1.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return whitelist.find(lowerSha1) != whitelist.end();
}

bool Helper::IsInBlacklist(const std::string& sha1, const std::set<std::string>& blacklist) {
    if (blacklist.empty()) {
        return false;
    }
    std::string lowerSha1 = sha1;
    std::transform(lowerSha1.begin(), lowerSha1.end(), lowerSha1.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return blacklist.find(lowerSha1) != blacklist.end();
}

std::vector<std::string> Helper::FindTransactionLogs(const std::string& hivePath) {
    std::vector<std::string> logs;
    fs::path hive(hivePath);
    fs::path directory = hive.parent_path();
    if (directory.empty()) {
        directory = ".";
    }
    std::string baseName = hive.stem().string();

    std::vector<std::string> logExtensions = {".LOG1", ".LOG2"};
    for (const auto& ext : logExtensions) {
        fs::path logPath = directory / (baseName + ext);
        if (fs::exists(logPath)) {
            logs.push_back(logPath.string());
        }
    }
    return logs;
}

std::string Helper::GenerateOutputFilename(const std::string& timestamp,
                                            const std::string& hiveName,
                                            const std::string& suffix,
                                            const std::string& extension) {
    return timestamp + "_" + hiveName + suffix + extension;
}

std::string Helper::GetBaseName(const std::string& path) {
    fs::path p(path);
    return p.stem().string();
}

std::string Helper::GetTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = {};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d%H%M%S");
    return oss.str();
}

} // namespace amcache
