#include "AmcacheNew.h"
#include "Helper.h"
#include "../utils/DateTimeUtils.h"
#include <algorithm>

namespace amcache {

AmcacheNew::ParseResult AmcacheNew::Parse(const RegistryHive& hive,
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

        result.ShortCuts = ParseShortcuts(hive);
        result.DeviceContainers = ParseDeviceContainers(hive);
        result.DevicePnps = ParseDevicePnps(hive);
        result.DriveBinaries = ParseDriverBinaries(hive);
        result.DriverPackages = ParseDriverPackages(hive);

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

bool AmcacheNew::ParseBool(const std::string& value) {
    return value == "1";
}

int64_t AmcacheNew::ParseSize(const std::string& value) {
    if (value.empty()) {
        return 0;
    }
    try {
        if (value.length() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
            return static_cast<int64_t>(std::stoull(value.substr(2), nullptr, 16));
        }
        return std::stoll(value);
    } catch (...) {
        return 0;
    }
}

std::vector<ProgramsEntryNew> AmcacheNew::ParsePrograms(const RegistryHive& hive) {
    std::vector<ProgramsEntryNew> programs;

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

ProgramsEntryNew AmcacheNew::ParseProgramEntry(const RegistryKey& key) {
    ProgramsEntryNew entry;

    entry.ProgramId = key.GetName();
    entry.KeyLastWriteTimestamp = key.GetLastWriteTime();

    if (auto val = key.GetStringValue("BundleManifestPath")) entry.BundleManifestPath = *val;
    if (auto val = key.GetStringValue("HiddenArp")) entry.HiddenArp = ParseBool(*val);
    if (auto val = key.GetStringValue("InboxModernApp")) entry.InboxModernApp = ParseBool(*val);
    if (auto val = key.GetStringValue("InstallDateArpLastModified")) entry.InstallDateArpLastModified = *val;
    if (auto val = key.GetStringValue("InstallDateFromLinkFile")) entry.InstallDateFromLinkFile = *val;
    if (auto val = key.GetStringValue("ManifestPath")) entry.ManifestPath = *val;
    if (auto val = key.GetStringValue("MsiPackageCode")) entry.MsiPackageCode = *val;
    if (auto val = key.GetStringValue("MsiProductCode")) entry.MsiProductCode = *val;
    if (auto val = key.GetStringValue("Name")) entry.Name = *val;
    if (auto val = key.GetStringValue("OSVersionAtInstallTime")) entry.OSVersionAtInstallTime = *val;
    if (auto val = key.GetStringValue("PackageFullName")) entry.PackageFullName = *val;
    if (auto val = key.GetStringValue("ProgramId")) entry.ProgramId = *val;
    if (auto val = key.GetStringValue("ProgramInstanceId")) entry.ProgramInstanceId = *val;
    if (auto val = key.GetStringValue("Publisher")) entry.Publisher = *val;
    if (auto val = key.GetStringValue("RegistryKeyPath")) entry.RegistryKeyPath = *val;
    if (auto val = key.GetStringValue("RootDirPath")) entry.RootDirPath = *val;
    if (auto val = key.GetStringValue("Source")) entry.Source = *val;
    if (auto val = key.GetStringValue("StoreAppType")) entry.StoreAppType = *val;
    if (auto val = key.GetStringValue("Type")) entry.Type = *val;
    if (auto val = key.GetStringValue("UninstallString")) entry.UninstallString = *val;
    if (auto val = key.GetStringValue("Version")) entry.Version = *val;
    if (auto val = key.GetStringValue("Manufacturer")) entry.Manufacturer = *val;

    if (auto val = key.GetInt32Value("Language")) entry.Language = *val;

    if (auto val = key.GetStringValue("InstallDate")) {
        entry.InstallDate = DateTimeUtils::ParseDateTime(*val);
    }
    if (auto val = key.GetStringValue("InstallDateMsi")) {
        entry.InstallDateMsi = DateTimeUtils::ParseDateTime(*val);
    }

    return entry;
}

std::vector<FileEntryNew> AmcacheNew::ParseFiles(const RegistryHive& hive) {
    std::vector<FileEntryNew> files;

    auto filesKey = hive.GetKeyByPath(FILES_PATH);
    if (!filesKey.IsValid()) {
        return files;
    }

    auto subkeys = filesKey.GetSubKeys();
    files.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        try {
            auto entry = ParseFileEntry(subkey);
            files.push_back(std::move(entry));
        } catch (...) {
            // Skip malformed entries
        }
    }

    return files;
}

FileEntryNew AmcacheNew::ParseFileEntry(const RegistryKey& key) {
    FileEntryNew entry;

    entry.FileKeyLastWriteTimestamp = key.GetLastWriteTime();

    if (auto val = key.GetStringValue("BinaryType")) entry.BinaryType = *val;
    if (auto val = key.GetStringValue("BinFileVersion")) entry.BinFileVersion = *val;
    if (auto val = key.GetStringValue("BinProductVersion")) entry.BinProductVersion = *val;
    if (auto val = key.GetStringValue("FileId")) {
        entry.SHA1 = FileEntryNew::ProcessSha1(*val);
    }
    if (auto val = key.GetStringValue("IsOsComponent")) entry.IsOsComponent = ParseBool(*val);
    if (auto val = key.GetStringValue("IsPeFile")) entry.IsPeFile = ParseBool(*val);
    if (auto val = key.GetStringValue("LongPathHash")) entry.LongPathHash = *val;
    if (auto val = key.GetStringValue("LowerCaseLongPath")) {
        entry.FullPath = *val;
        entry.FileExtension = FileEntryNew::ExtractFileExtension(*val);
    }
    if (auto val = key.GetStringValue("Name")) entry.Name = *val;
    if (auto val = key.GetStringValue("ProductName")) entry.ProductName = *val;
    if (auto val = key.GetStringValue("ProductVersion")) entry.ProductVersion = *val;
    if (auto val = key.GetStringValue("ProgramId")) entry.ProgramId = *val;
    if (auto val = key.GetStringValue("Publisher")) entry.Publisher = *val;
    if (auto val = key.GetStringValue("Version")) entry.Version = *val;
    if (auto val = key.GetStringValue("Description")) entry.Description = *val;
    if (auto val = key.GetStringValue("OriginalFileName")) entry.OriginalFileName = *val;

    if (auto val = key.GetInt32Value("Language")) entry.Language = *val;
    if (auto val = key.GetStringValue("Size")) entry.Size = ParseSize(*val);
    if (auto val = key.GetUInt64Value("Usn")) entry.Usn = *val;

    if (auto val = key.GetStringValue("LinkDate")) {
        auto parsed = DateTimeUtils::ParseDateTime(*val);
        if (!parsed.has_value()) {
            entry.LinkDate = DateTimeUtils::MinValue();
        } else {
            entry.LinkDate = *parsed;
        }
    }

    return entry;
}

std::vector<Shortcut> AmcacheNew::ParseShortcuts(const RegistryHive& hive) {
    std::vector<Shortcut> shortcuts;

    auto shortcutsKey = hive.GetKeyByPath(SHORTCUTS_PATH);
    if (!shortcutsKey.IsValid()) {
        return shortcuts;
    }

    auto subkeys = shortcutsKey.GetSubKeys();
    shortcuts.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        Shortcut entry;
        entry.KeyName = subkey.GetName();
        entry.KeyLastWriteTimestamp = subkey.GetLastWriteTime();

        auto values = subkey.GetValues();
        if (!values.empty()) {
            if (auto str = values[0].GetString()) {
                entry.LnkName = *str;
            }
        }

        shortcuts.push_back(std::move(entry));
    }

    return shortcuts;
}

std::vector<DeviceContainer> AmcacheNew::ParseDeviceContainers(const RegistryHive& hive) {
    std::vector<DeviceContainer> containers;

    auto containersKey = hive.GetKeyByPath(DEVICE_CONTAINERS_PATH);
    if (!containersKey.IsValid()) {
        return containers;
    }

    auto subkeys = containersKey.GetSubKeys();
    containers.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        DeviceContainer entry;
        entry.KeyName = subkey.GetName();
        entry.KeyLastWriteTimestamp = subkey.GetLastWriteTime();

        if (auto val = subkey.GetStringValue("Categories")) entry.Categories = *val;
        if (auto val = subkey.GetStringValue("DiscoveryMethod")) entry.DiscoveryMethod = *val;
        if (auto val = subkey.GetStringValue("FriendlyName")) entry.FriendlyName = *val;
        if (auto val = subkey.GetStringValue("Icon")) entry.Icon = *val;
        if (auto val = subkey.GetStringValue("IsActive")) entry.IsActive = ParseBool(*val);
        if (auto val = subkey.GetStringValue("IsConnected")) entry.IsConnected = ParseBool(*val);
        if (auto val = subkey.GetStringValue("IsMachineContainer")) entry.IsMachineContainer = ParseBool(*val);
        if (auto val = subkey.GetStringValue("IsNetworked")) entry.IsNetworked = ParseBool(*val);
        if (auto val = subkey.GetStringValue("IsPaired")) entry.IsPaired = ParseBool(*val);
        if (auto val = subkey.GetStringValue("Manufacturer")) entry.Manufacturer = *val;
        if (auto val = subkey.GetStringValue("ModelId")) entry.ModelId = *val;
        if (auto val = subkey.GetStringValue("ModelName")) entry.ModelName = *val;
        if (auto val = subkey.GetStringValue("ModelNumber")) entry.ModelNumber = *val;
        if (auto val = subkey.GetStringValue("PrimaryCategory")) entry.PrimaryCategory = *val;
        if (auto val = subkey.GetStringValue("State")) entry.State = *val;

        containers.push_back(std::move(entry));
    }

    return containers;
}

std::vector<DevicePnp> AmcacheNew::ParseDevicePnps(const RegistryHive& hive) {
    std::vector<DevicePnp> devices;

    auto devicesKey = hive.GetKeyByPath(DEVICE_PNP_PATH);
    if (!devicesKey.IsValid()) {
        return devices;
    }

    auto subkeys = devicesKey.GetSubKeys();
    devices.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        DevicePnp entry;
        entry.KeyName = subkey.GetName();
        entry.KeyLastWriteTimestamp = subkey.GetLastWriteTime();

        if (auto val = subkey.GetStringValue("BusReportedDescription")) entry.BusReportedDescription = *val;
        if (auto val = subkey.GetStringValue("Class")) entry.Class = *val;
        if (auto val = subkey.GetStringValue("ClassGuid")) entry.ClassGuid = *val;
        if (auto val = subkey.GetStringValue("COMPID")) entry.Compid = *val;
        if (auto val = subkey.GetStringValue("ContainerId")) entry.ContainerId = *val;
        if (auto val = subkey.GetStringValue("Description")) entry.Description = *val;
        if (auto val = subkey.GetStringValue("DeviceState")) entry.DeviceState = *val;
        if (auto val = subkey.GetStringValue("DriverId")) entry.DriverId = *val;
        if (auto val = subkey.GetStringValue("DriverName")) entry.DriverName = *val;
        if (auto val = subkey.GetStringValue("DriverPackageStrongName")) entry.DriverPackageStrongName = *val;
        if (auto val = subkey.GetStringValue("DriverVerDate")) entry.DriverVerDate = *val;
        if (auto val = subkey.GetStringValue("DriverVerVersion")) entry.DriverVerVersion = *val;
        if (auto val = subkey.GetStringValue("Enumerator")) entry.Enumerator = *val;
        if (auto val = subkey.GetStringValue("HWID")) entry.HWID = *val;
        if (auto val = subkey.GetStringValue("Inf")) entry.Inf = *val;
        if (auto val = subkey.GetStringValue("InstallState")) entry.InstallState = *val;
        if (auto val = subkey.GetStringValue("Manufacturer")) entry.Manufacturer = *val;
        if (auto val = subkey.GetStringValue("MatchingID")) entry.MatchingId = *val;
        if (auto val = subkey.GetStringValue("Model")) entry.Model = *val;
        if (auto val = subkey.GetStringValue("ParentId")) entry.ParentId = *val;
        if (auto val = subkey.GetStringValue("ProblemCode")) entry.ProblemCode = *val;
        if (auto val = subkey.GetStringValue("Provider")) entry.Provider = *val;
        if (auto val = subkey.GetStringValue("Service")) entry.Service = *val;
        if (auto val = subkey.GetStringValue("STACKID")) entry.Stackid = *val;

        devices.push_back(std::move(entry));
    }

    return devices;
}

std::vector<DriverBinary> AmcacheNew::ParseDriverBinaries(const RegistryHive& hive) {
    std::vector<DriverBinary> binaries;

    auto binariesKey = hive.GetKeyByPath(DRIVER_BINARY_PATH);
    if (!binariesKey.IsValid()) {
        return binaries;
    }

    auto subkeys = binariesKey.GetSubKeys();
    binaries.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        DriverBinary entry;
        entry.KeyName = subkey.GetName();
        entry.KeyLastWriteTimestamp = subkey.GetLastWriteTime();

        if (auto val = subkey.GetInt32Value("DriverCheckSum")) entry.DriverCheckSum = *val;
        if (auto val = subkey.GetStringValue("DriverCompany")) entry.DriverCompany = *val;
        if (auto val = subkey.GetStringValue("DriverId")) {
            if (val->length() > 4) {
                entry.DriverId = val->substr(4);
            } else {
                entry.DriverId = *val;
            }
        }
        if (auto val = subkey.GetStringValue("DriverInBox")) entry.DriverInBox = ParseBool(*val);
        if (auto val = subkey.GetStringValue("DriverIsKernelMode")) entry.DriverIsKernelMode = ParseBool(*val);
        if (auto val = subkey.GetStringValue("DriverLastWriteTime")) {
            entry.DriverLastWriteTime = DateTimeUtils::ParseDateTime(*val);
        }
        if (auto val = subkey.GetStringValue("DriverName")) entry.DriverName = *val;
        if (auto val = subkey.GetStringValue("DriverPackageStrongName")) entry.DriverPackageStrongName = *val;
        if (auto val = subkey.GetStringValue("DriverSigned")) entry.DriverSigned = ParseBool(*val);
        if (auto val = subkey.GetInt64Value("DriverTimeStamp")) {
            if (*val > 0) {
                entry.DriverTimeStamp = DateTimeUtils::FromUnixSeconds(*val);
            }
        }
        if (auto val = subkey.GetStringValue("DriverType")) entry.DriverType = *val;
        if (auto val = subkey.GetStringValue("DriverVersion")) entry.DriverVersion = *val;
        if (auto val = subkey.GetInt32Value("ImageSize")) entry.ImageSize = *val;
        if (auto val = subkey.GetStringValue("Inf")) entry.Inf = *val;
        if (auto val = subkey.GetStringValue("Product")) entry.Product = *val;
        if (auto val = subkey.GetStringValue("ProductVersion")) entry.ProductVersion = *val;
        if (auto val = subkey.GetStringValue("Service")) entry.Service = *val;
        if (auto val = subkey.GetStringValue("WdfVersion")) entry.WdfVersion = *val;

        binaries.push_back(std::move(entry));
    }

    return binaries;
}

std::vector<DriverPackage> AmcacheNew::ParseDriverPackages(const RegistryHive& hive) {
    std::vector<DriverPackage> packages;

    auto packagesKey = hive.GetKeyByPath(DRIVER_PACKAGE_PATH);
    if (!packagesKey.IsValid()) {
        return packages;
    }

    auto subkeys = packagesKey.GetSubKeys();
    packages.reserve(subkeys.size());

    for (auto& subkey : subkeys) {
        DriverPackage entry;
        entry.KeyName = subkey.GetName();
        entry.KeyLastWriteTimestamp = subkey.GetLastWriteTime();

        if (auto val = subkey.GetStringValue("Class")) entry.Class = *val;
        if (auto val = subkey.GetStringValue("ClassGuid")) entry.ClassGuid = *val;
        if (auto val = subkey.GetStringValue("Date")) {
            entry.Date = DateTimeUtils::ParseDateTime(*val);
        }
        if (auto val = subkey.GetStringValue("Directory")) entry.Directory = *val;
        if (auto val = subkey.GetStringValue("DriverInBox")) entry.DriverInBox = ParseBool(*val);
        if (auto val = subkey.GetStringValue("Hwids")) entry.Hwids = *val;
        if (auto val = subkey.GetStringValue("Inf")) entry.Inf = *val;
        if (auto val = subkey.GetStringValue("Provider")) entry.Provider = *val;
        if (auto val = subkey.GetStringValue("SubmissionId")) entry.SubmissionId = *val;
        if (auto val = subkey.GetStringValue("SYSFILE")) entry.SYSFILE = *val;
        if (auto val = subkey.GetStringValue("Version")) entry.Version = *val;

        packages.push_back(std::move(entry));
    }

    return packages;
}

void AmcacheNew::LinkFilesToPrograms(std::vector<ProgramsEntryNew>& programs,
                                      std::vector<FileEntryNew>& files,
                                      std::vector<FileEntryNew>& unassociated,
                                      std::vector<FileEntryNew>& associated) {
    std::map<std::string, size_t> programMap;
    for (size_t i = 0; i < programs.size(); ++i) {
        programMap[programs[i].ProgramId] = i;
    }

    for (auto& file : files) {
        if (!file.ProgramId.empty()) {
            auto it = programMap.find(file.ProgramId);
            if (it != programMap.end()) {
                file.ApplicationName = programs[it->second].Name;
                programs[it->second].FileEntries.push_back(file);
                associated.push_back(std::move(file));
                continue;
            }
        }
        file.ApplicationName = "Unassociated";
        unassociated.push_back(std::move(file));
    }
}

void AmcacheNew::ApplyFilters(std::vector<FileEntryNew>& files,
                              const std::set<std::string>& whitelist,
                              const std::set<std::string>& blacklist) {
    if (whitelist.empty() && blacklist.empty()) {
        return;
    }

    files.erase(
        std::remove_if(files.begin(), files.end(),
            [&whitelist, &blacklist](const FileEntryNew& file) {
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
