#pragma once

#include "Common.h"
#include "FileEntryNew.h"
#include <vector>

namespace amcache {

struct ProgramsEntryNew {
    std::string BundleManifestPath;
    bool HiddenArp = false;
    bool InboxModernApp = false;
    OptionalTimestamp InstallDate;
    std::string InstallDateArpLastModified;
    OptionalTimestamp InstallDateMsi;
    std::string InstallDateFromLinkFile;
    int32_t Language = 0;
    std::string ManifestPath;
    std::string MsiPackageCode;
    std::string MsiProductCode;
    std::string Name;
    std::string OSVersionAtInstallTime;
    std::string PackageFullName;
    std::string ProgramId;
    std::string ProgramInstanceId;
    std::string Publisher;
    std::string RegistryKeyPath;
    std::string RootDirPath;
    std::string Source;
    std::string StoreAppType;
    std::string Type;
    std::string UninstallString;
    std::string Version;
    std::string Manufacturer;
    Timestamp KeyLastWriteTimestamp;
    std::vector<FileEntryNew> FileEntries;

    ProgramsEntryNew() = default;
};

} // namespace amcache
