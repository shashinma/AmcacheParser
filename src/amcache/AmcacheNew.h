#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "../classes/FileEntryNew.h"
#include "../classes/ProgramsEntryNew.h"
#include "../classes/Shortcut.h"
#include "../classes/DeviceContainer.h"
#include "../classes/DevicePnp.h"
#include "../classes/DriverBinary.h"
#include "../classes/DriverPackage.h"
#include "../registry/RegistryWrapper.h"

namespace amcache {

class AmcacheNew {
public:
    struct ParseResult {
        std::vector<ProgramsEntryNew> ProgramsEntries;
        std::vector<FileEntryNew> UnassociatedFileEntries;
        std::vector<FileEntryNew> AssociatedFileEntries;
        std::vector<Shortcut> ShortCuts;
        std::vector<DeviceContainer> DeviceContainers;
        std::vector<DevicePnp> DevicePnps;
        std::vector<DriverBinary> DriveBinaries;
        std::vector<DriverPackage> DriverPackages;
        int TotalFileEntries = 0;
        bool Success = false;
        std::string ErrorMessage;
    };

    static ParseResult Parse(const RegistryHive& hive,
                            bool includeLinkedFiles,
                            const std::set<std::string>& whitelist,
                            const std::set<std::string>& blacklist);

private:
    static std::vector<ProgramsEntryNew> ParsePrograms(const RegistryHive& hive);
    static ProgramsEntryNew ParseProgramEntry(const RegistryKey& key);
    static std::vector<FileEntryNew> ParseFiles(const RegistryHive& hive);
    static FileEntryNew ParseFileEntry(const RegistryKey& key);
    static std::vector<Shortcut> ParseShortcuts(const RegistryHive& hive);
    static std::vector<DeviceContainer> ParseDeviceContainers(const RegistryHive& hive);
    static std::vector<DevicePnp> ParseDevicePnps(const RegistryHive& hive);
    static std::vector<DriverBinary> ParseDriverBinaries(const RegistryHive& hive);
    static std::vector<DriverPackage> ParseDriverPackages(const RegistryHive& hive);
    static void LinkFilesToPrograms(std::vector<ProgramsEntryNew>& programs,
                                    std::vector<FileEntryNew>& files,
                                    std::vector<FileEntryNew>& unassociated,
                                    std::vector<FileEntryNew>& associated);
    static void ApplyFilters(std::vector<FileEntryNew>& files,
                            const std::set<std::string>& whitelist,
                            const std::set<std::string>& blacklist);
    static bool ParseBool(const std::string& value);
    static int64_t ParseSize(const std::string& value);

    static constexpr const char* PROGRAMS_PATH = "Root\\InventoryApplication";
    static constexpr const char* FILES_PATH = "Root\\InventoryApplicationFile";
    static constexpr const char* SHORTCUTS_PATH = "Root\\InventoryApplicationShortcut";
    static constexpr const char* DEVICE_CONTAINERS_PATH = "Root\\InventoryDeviceContainer";
    static constexpr const char* DEVICE_PNP_PATH = "Root\\InventoryDevicePnp";
    static constexpr const char* DRIVER_BINARY_PATH = "Root\\InventoryDriverBinary";
    static constexpr const char* DRIVER_PACKAGE_PATH = "Root\\InventoryDriverPackage";
};

} // namespace amcache
