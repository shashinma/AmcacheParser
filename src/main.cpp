#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "amcache/Helper.h"
#include "amcache/AmcacheNew.h"
#include "amcache/AmcacheOld.h"
#include "registry/RegistryWrapper.h"
#include "csv/CsvWriter.h"
#include "utils/RawCopy.h"

namespace fs = std::filesystem;

static std::string UppercaseFirst(const std::string& s) {
    if (s.empty()) return "";
    std::string result = s;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

static std::string GetVersionString() {
    return "AmcacheParser version 1.0.0\n\n"
           "Author: shashinma\n"
           "https://github.com/shashinma\n"
           "https://github.com/Artifactum";
}

int main(int argc, char** argv) {
    CLI::App app{"AmcacheParser - Windows Amcache.hve parser"};
    app.set_version_flag("-v,--version", GetVersionString());

    std::string filePath;
    std::string csvDir;
    std::string csvf;
    std::string whitelistPath;
    std::string blacklistPath;
    std::string dateFormat = "yyyy-MM-dd HH:mm:ss";
    bool includeLinked = false;
    bool noLogs = false;
    bool debug = false;
    bool trace = false;
    bool mp = false;

    app.add_option("-f", filePath, "Amcache.hve file to parse")
        ->required()
        ->check(CLI::ExistingFile);

    app.add_option("--csv", csvDir, "Directory to save CSV formatted results to")
        ->required();

    app.add_option("--csvf", csvf, "File name to save CSV formatted results to. When present, overrides default name");

    app.add_flag("-i", includeLinked, "Include file entries for Programs entries");

    app.add_option("-w", whitelistPath, "Path to file containing SHA-1 hashes to *exclude* from the results");

    app.add_option("-b", blacklistPath, "Path to file containing SHA-1 hashes to *include* from the results. Blacklisting overrides whitelisting");

    app.add_option("--dt", dateFormat, "The custom date/time format to use when displaying time stamps")
        ->default_val("yyyy-MM-dd HH:mm:ss");

    app.add_flag("--mp", mp, "When true, display higher precision for timestamps");

    app.add_flag("--nl", noLogs, "When true, ignore transaction log files for dirty hives");

    app.add_flag("--debug", debug, "Show debug information during processing");

    app.add_flag("--trace", trace, "Show trace information during processing");

    CLI11_PARSE(app, argc, argv);

    // Setup logging
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);

    if (trace) {
        spdlog::set_level(spdlog::level::trace);
        spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    } else if (debug) {
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    } else {
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("%v");
    }

    // Convert .NET date format to strftime format
    std::string strftimeFormat = dateFormat;
    {
        size_t pos = 0;
        while ((pos = strftimeFormat.find("yyyy", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 4, "%Y");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("MM", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 2, "%m");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("dd", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 2, "%d");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("HH", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 2, "%H");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("mm", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 2, "%M");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("ss", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 2, "%S");
        }
        pos = 0;
        while ((pos = strftimeFormat.find("fffffff", pos)) != std::string::npos) {
            strftimeFormat.replace(pos, 7, "");
        }
    }

    if (mp) {
        strftimeFormat = "%Y-%m-%d %H:%M:%S";
    }

    spdlog::info("{}", GetVersionString());
    std::cout << std::endl;

    std::string cmdLine;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) cmdLine += " ";
        cmdLine += argv[i];
    }
    spdlog::info("Command line: {}", cmdLine);
    std::cout << std::endl;

    if (!amcache::RawCopy::IsAdministrator()) {
        spdlog::warn("Warning: Administrator privileges not found!");
        std::cout << std::endl;
    }

    auto startTime = std::chrono::steady_clock::now();
    std::string ts = amcache::Helper::GetTimestampString();

    // Load whitelist/blacklist
    std::set<std::string> whitelist;
    std::set<std::string> blacklist;
    bool useBlacklist = false;

    if (!blacklistPath.empty()) {
        if (fs::exists(blacklistPath)) {
            blacklist = amcache::Helper::LoadHashList(blacklistPath);
            useBlacklist = true;
        } else {
            spdlog::warn("{} does not exist", blacklistPath);
        }
    } else if (!whitelistPath.empty()) {
        if (fs::exists(whitelistPath)) {
            whitelist = amcache::Helper::LoadHashList(whitelistPath);
        } else {
            spdlog::warn("{} does not exist", whitelistPath);
        }
    }

    // Create output directory
    if (!fs::exists(csvDir)) {
        try {
            fs::create_directories(csvDir);
        } catch (const std::exception& e) {
            spdlog::error("There was an error creating directory {}. Error: {} Exiting", csvDir, e.what());
            return 1;
        }
    }

    // Open hive
    amcache::RegistryHive hive;
    bool opened = false;

    try {
        opened = hive.Open(filePath);
    } catch (...) {
        opened = false;
    }

    if (!opened) {
        spdlog::info("'{}' is in use. Rerouting...", filePath);
        std::cout << std::endl;

        auto data = amcache::RawCopy::ReadLockedFile(filePath);
        if (data.has_value()) {
            opened = hive.OpenFromBuffer(*data);
        }

        if (!opened) {
            spdlog::error("{} not found or could not be accessed. Exiting", filePath);
            return 1;
        }
    }

    // Check for dirty hive
    if (hive.IsDirty()) {
        if (!noLogs) {
            auto logFiles = amcache::Helper::FindTransactionLogs(filePath);
            if (logFiles.empty()) {
                spdlog::warn("Registry hive is dirty and no transaction logs were found in the same directory! LOGs should have same base name as the hive. Aborting!!");
                return 1;
            } else {
                spdlog::warn("Registry hive is dirty. Transaction logs found but automatic replay is not implemented in this version. Data may be incomplete.");
            }
        } else {
            spdlog::warn("Registry hive is dirty and transaction logs were found in the same directory, but --nl was provided. Data may be missing! Continuing anyways...");
        }
    }

    // Detect format
    bool isNewFormat = amcache::Helper::IsNewFormat(hive);
    std::string hiveName = amcache::Helper::GetBaseName(filePath);

    amcache::CsvWriter csvWriter(strftimeFormat, mp);

    if (isNewFormat) {
        auto result = amcache::AmcacheNew::Parse(hive, includeLinked, whitelist, blacklist);

        if (!result.Success) {
            spdlog::error("Failed to parse hive: {}", result.ErrorMessage);
            return 1;
        }

        if (result.ProgramsEntries.empty() && result.UnassociatedFileEntries.empty()) {
            spdlog::warn("Hive did not contain program entries nor file entries");
        }

        // UnassociatedFileEntries
        std::string outbase = ts + "_" + hiveName + "_UnassociatedFileEntries.csv";
        if (!csvf.empty()) {
            outbase = fs::path(csvf).stem().string() + "_UnassociatedFileEntries" + fs::path(csvf).extension().string();
        }
        csvWriter.WriteFileEntriesNew((fs::path(csvDir) / outbase).string(), result.UnassociatedFileEntries);

        if (includeLinked) {
            // ProgramEntries
            outbase = ts + "_" + hiveName + "_ProgramEntries.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_ProgramEntries" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteProgramEntriesNew((fs::path(csvDir) / outbase).string(), result.ProgramsEntries);

            // AssociatedFileEntries
            outbase = ts + "_" + hiveName + "_AssociatedFileEntries.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_AssociatedFileEntries" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteFileEntriesNew((fs::path(csvDir) / outbase).string(), result.AssociatedFileEntries);
        }

        // ShortCuts
        if (!result.ShortCuts.empty()) {
            outbase = ts + "_" + hiveName + "_ShortCuts.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_ShortCuts" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteShortcuts((fs::path(csvDir) / outbase).string(), result.ShortCuts);
        }

        // DriveBinaries
        if (!result.DriveBinaries.empty()) {
            outbase = ts + "_" + hiveName + "_DriveBinaries.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_DriveBinaries" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteDriverBinaries((fs::path(csvDir) / outbase).string(), result.DriveBinaries);
        }

        // DeviceContainers
        if (!result.DeviceContainers.empty()) {
            outbase = ts + "_" + hiveName + "_DeviceContainers.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_DeviceContainers" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteDeviceContainers((fs::path(csvDir) / outbase).string(), result.DeviceContainers);
        }

        // DevicePnps
        if (!result.DevicePnps.empty()) {
            outbase = ts + "_" + hiveName + "_DevicePnps.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_DevicePnps" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteDevicePnps((fs::path(csvDir) / outbase).string(), result.DevicePnps);
        }

        // DriverPackages
        if (!result.DriverPackages.empty()) {
            outbase = ts + "_" + hiveName + "_DriverPackages.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_DriverPackages" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteDriverPackages((fs::path(csvDir) / outbase).string(), result.DriverPackages);
        }

        std::cout << std::endl;
        spdlog::info("{} is in new format!", filePath);
        std::cout << std::endl;

        spdlog::info("Total file entries found: {:,}", result.TotalFileEntries);
        if (!result.ShortCuts.empty()) {
            spdlog::info("Total shortcuts found: {:,}", result.ShortCuts.size());
        }
        if (!result.DeviceContainers.empty()) {
            spdlog::info("Total device containers found: {:,}", result.DeviceContainers.size());
        }
        if (!result.DevicePnps.empty()) {
            spdlog::info("Total device PnPs found: {:,}", result.DevicePnps.size());
        }
        if (!result.DriveBinaries.empty()) {
            spdlog::info("Total drive binaries found: {:,}", result.DriveBinaries.size());
        }
        if (!result.DriverPackages.empty()) {
            spdlog::info("Total driver packages found: {:,}", result.DriverPackages.size());
        }

        std::cout << std::endl;
        int totalProgramFileEntries = 0;
        for (const auto& pe : result.ProgramsEntries) {
            totalProgramFileEntries += static_cast<int>(pe.FileEntries.size());
        }

        if (result.UnassociatedFileEntries.size() == 1) {
            if (includeLinked) {
                spdlog::info("Found {} unassociated file entry and {} program file entries (across {} program entries)",
                    result.UnassociatedFileEntries.size(), totalProgramFileEntries, result.ProgramsEntries.size());
            } else {
                spdlog::info("Found {} unassociated file entry", result.UnassociatedFileEntries.size());
            }
        } else {
            if (includeLinked) {
                spdlog::info("Found {} unassociated file entries and {} program file entries (across {} program entries)",
                    result.UnassociatedFileEntries.size(), totalProgramFileEntries, result.ProgramsEntries.size());
            } else {
                spdlog::info("Found {} unassociated file entries", result.UnassociatedFileEntries.size());
            }
        }

        if (!whitelist.empty() || !blacklist.empty()) {
            double per = static_cast<double>(totalProgramFileEntries + result.UnassociatedFileEntries.size()) / result.TotalFileEntries;
            std::cout << std::endl;
            std::string list = useBlacklist ? "blacklist" : "whitelist";
            spdlog::info("{} hash count: {:,}", UppercaseFirst(list), useBlacklist ? blacklist.size() : whitelist.size());
            std::cout << std::endl;
            spdlog::info("Percentage of total shown based on {}: {:.3f}% ({:.3f}% savings)", list, per * 100.0, (1.0 - per) * 100.0);
        }

    } else {
        auto result = amcache::AmcacheOld::Parse(hive, includeLinked, whitelist, blacklist);

        if (!result.Success) {
            spdlog::error("Failed to parse hive: {}", result.ErrorMessage);
            return 1;
        }

        if (result.ProgramsEntries.empty() && result.UnassociatedFileEntries.empty()) {
            spdlog::warn("Hive did not contain program entries nor file entries. Exiting");
            return 1;
        }

        // UnassociatedFileEntries
        std::string outbase = ts + "_" + hiveName + "_UnassociatedFileEntries.csv";
        if (!csvf.empty()) {
            outbase = fs::path(csvf).stem().string() + "_UnassociatedFileEntries" + fs::path(csvf).extension().string();
        }
        csvWriter.WriteFileEntriesOld((fs::path(csvDir) / outbase).string(), result.UnassociatedFileEntries);

        if (includeLinked) {
            // ProgramEntries
            outbase = ts + "_" + hiveName + "_ProgramEntries.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_ProgramEntries" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteProgramEntriesOld((fs::path(csvDir) / outbase).string(), result.ProgramsEntries);

            // AssociatedFileEntries
            outbase = ts + "_" + hiveName + "_AssociatedFileEntries.csv";
            if (!csvf.empty()) {
                outbase = fs::path(csvf).stem().string() + "_AssociatedFileEntries" + fs::path(csvf).extension().string();
            }
            csvWriter.WriteFileEntriesOld((fs::path(csvDir) / outbase).string(), result.AssociatedFileEntries);
        }

        std::cout << std::endl;
        spdlog::info("{} is in old format!", filePath);
        std::cout << std::endl;

        spdlog::info("Total file entries found: {:,}", result.TotalFileEntries);

        int totalProgramFileEntries = 0;
        for (const auto& pe : result.ProgramsEntries) {
            totalProgramFileEntries += static_cast<int>(pe.FileEntries.size());
        }

        if (result.UnassociatedFileEntries.size() == 1) {
            if (includeLinked) {
                spdlog::info("Found {} unassociated file entry and {} program file entries (across {} program entries)",
                    result.UnassociatedFileEntries.size(), totalProgramFileEntries, result.ProgramsEntries.size());
            } else {
                spdlog::info("Found {} unassociated file entry", result.UnassociatedFileEntries.size());
            }
        } else {
            if (includeLinked) {
                spdlog::info("Found {} unassociated file entries and {} program file entries (across {} program entries)",
                    result.UnassociatedFileEntries.size(), totalProgramFileEntries, result.ProgramsEntries.size());
            } else {
                spdlog::info("Found {} unassociated file entries", result.UnassociatedFileEntries.size());
            }
        }

        if (!whitelist.empty() || !blacklist.empty()) {
            double per = static_cast<double>(totalProgramFileEntries + result.UnassociatedFileEntries.size()) / result.TotalFileEntries;
            std::cout << std::endl;
            std::string list = useBlacklist ? "blacklist" : "whitelist";
            spdlog::info("{} hash count: {:,}", UppercaseFirst(list), useBlacklist ? blacklist.size() : whitelist.size());
            std::cout << std::endl;
            spdlog::info("Percentage of total shown based on {}: {:.3f}% ({:.3f}% savings)", list, per * 100.0, (1.0 - per) * 100.0);
        }
    }

    std::cout << std::endl;
    spdlog::info("Results saved to: {}", csvDir);
    std::cout << std::endl;

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);
    spdlog::info("Total parsing time: {:.3f} seconds", elapsed.count());
    std::cout << std::endl;

    return 0;
}
