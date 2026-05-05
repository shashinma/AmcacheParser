#include "CsvWriter.h"
#include "../utils/DateTimeUtils.h"
#include <algorithm>

namespace amcache {

CsvWriter::CsvWriter(const std::string& dateFormat, bool useMicroseconds)
    : dateFormat_(dateFormat), useMicroseconds_(useMicroseconds) {}

std::string CsvWriter::EscapeCsv(const std::string& field) {
    bool needsEscaping = false;
    for (char c : field) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            needsEscaping = true;
            break;
        }
    }

    if (!needsEscaping) {
        return field;
    }

    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

std::string CsvWriter::FormatTimestamp(const Timestamp& ts) const {
    if (useMicroseconds_) {
        return DateTimeUtils::FormatMicroseconds(ts);
    }
    return DateTimeUtils::Format(ts, dateFormat_);
}

std::string CsvWriter::FormatTimestamp(const OptionalTimestamp& ts) const {
    if (!ts.has_value()) {
        return "";
    }
    return FormatTimestamp(*ts);
}

void CsvWriter::WriteCsvLine(std::ofstream& file, const std::vector<std::string>& values) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) file << ",";
        file << EscapeCsv(values[i]);
    }
    file << "\n";
}

// ============================================================================
// New Format - FileEntryNew (Unassociated and Associated)
// Column order matching .NET original exactly:
// 0:ApplicationName 1:ProgramId 2:FileKeyLastWriteTimestamp 3:SHA1 4:IsOsComponent
// 5:FullPath 6:Name 7:FileExtension 8:LinkDate 9:ProductName 10:Size
// 11:Version 12:ProductVersion 13:LongPathHash 14:BinaryType 15:IsPeFile
// 16:BinFileVersion 17:BinProductVersion 18:Language
// ============================================================================
bool CsvWriter::WriteFileEntriesNew(const std::string& path, const std::vector<FileEntryNew>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "ApplicationName", "ProgramId", "FileKeyLastWriteTimestamp", "SHA1", "IsOsComponent",
        "FullPath", "Name", "FileExtension", "LinkDate", "ProductName", "Size",
        "Version", "ProductVersion", "LongPathHash", "BinaryType", "IsPeFile",
        "BinFileVersion", "BinProductVersion", "Language"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.ApplicationName,
            e.ProgramId,
            FormatTimestamp(e.FileKeyLastWriteTimestamp),
            e.SHA1,
            e.IsOsComponent ? "True" : "False",
            e.FullPath,
            e.Name,
            e.FileExtension,
            FormatTimestamp(e.LinkDate),
            e.ProductName,
            std::to_string(e.Size),
            e.Version,
            e.ProductVersion,
            e.LongPathHash,
            e.BinaryType,
            e.IsPeFile ? "True" : "False",
            e.BinFileVersion,
            e.BinProductVersion,
            std::to_string(e.Language)
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// New Format - ProgramsEntryNew
// 0:ProgramId 1:KeyLastWriteTimestamp 2:Name 3:Version 4:Publisher 5:InstallDate
// 6:OSVersionAtInstallTime 7:BundleManifestPath 8:HiddenArp 9:InboxModernApp
// 10:Language 11:ManifestPath 12:MsiPackageCode 13:MsiProductCode 14:PackageFullName
// 15:ProgramInstanceId 16:RegistryKeyPath 17:RootDirPath 18:Type 19:Source
// 20:StoreAppType 21:UninstallString
// ============================================================================
bool CsvWriter::WriteProgramEntriesNew(const std::string& path, const std::vector<ProgramsEntryNew>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "ProgramId", "KeyLastWriteTimestamp", "Name", "Version", "Publisher", "InstallDate",
        "OSVersionAtInstallTime", "BundleManifestPath", "HiddenArp", "InboxModernApp",
        "Language", "ManifestPath", "MsiPackageCode", "MsiProductCode", "PackageFullName",
        "ProgramInstanceId", "RegistryKeyPath", "RootDirPath", "Type", "Source",
        "StoreAppType", "UninstallString"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.ProgramId,
            FormatTimestamp(e.KeyLastWriteTimestamp),
            e.Name,
            e.Version,
            e.Publisher,
            FormatTimestamp(e.InstallDate),
            e.OSVersionAtInstallTime,
            e.BundleManifestPath,
            e.HiddenArp ? "True" : "False",
            e.InboxModernApp ? "True" : "False",
            std::to_string(e.Language),
            e.ManifestPath,
            e.MsiPackageCode,
            e.MsiProductCode,
            e.PackageFullName,
            e.ProgramInstanceId,
            e.RegistryKeyPath,
            e.RootDirPath,
            e.Type,
            e.Source,
            e.StoreAppType,
            e.UninstallString
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// Old Format - FileEntryOld
// 0:ProgramName 1:ProgramID 2:VolumeID 3:VolumeIDLastWriteTimestamp 4:FileID
// 5:FileIDLastWriteTimestamp 6:SHA1 7:FullPath 8:FileExtension 9:MFTEntryNumber
// 10:MFTSequenceNumber 11:FileSize 12:FileVersionString 13:FileVersionNumber
// 14:FileDescription 15:SizeOfImage 16:PEHeaderHash 17:PEHeaderChecksum
// 18:BinProductVersion 19:BinFileVersion 20:LinkerVersion 21:BinaryType
// 22:IsLocal 23:GuessProgramID 24:Created 25:LastModified 26:LastModifiedStore
// 27:LinkDate 28:LanguageID 29:ProductName 30:CompanyName 31:SwitchBackContext
// ============================================================================
bool CsvWriter::WriteFileEntriesOld(const std::string& path, const std::vector<FileEntryOld>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "ProgramName", "ProgramID", "VolumeID", "VolumeIDLastWriteTimestamp", "FileID",
        "FileIDLastWriteTimestamp", "SHA1", "FullPath", "FileExtension", "MFTEntryNumber",
        "MFTSequenceNumber", "FileSize", "FileVersionString", "FileVersionNumber",
        "FileDescription", "SizeOfImage", "PEHeaderHash", "PEHeaderChecksum",
        "BinProductVersion", "BinFileVersion", "LinkerVersion", "BinaryType",
        "IsLocal", "GuessProgramID", "Created", "LastModified", "LastModifiedStore",
        "LinkDate", "LanguageID", "ProductName", "CompanyName", "SwitchBackContext"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.ProgramName,
            e.ProgramID,
            e.VolumeID,
            FormatTimestamp(e.VolumeIDLastWriteTimestamp),
            e.FileID,
            FormatTimestamp(e.FileIDLastWriteTimestamp),
            e.SHA1,
            e.FullPath,
            e.FileExtension,
            std::to_string(e.MFTEntryNumber),
            std::to_string(e.MFTSequenceNumber),
            e.FileSize.has_value() ? std::to_string(*e.FileSize) : "",
            e.FileVersionString,
            e.FileVersionNumber,
            e.FileDescription,
            e.SizeOfImage.has_value() ? std::to_string(*e.SizeOfImage) : "",
            e.PEHeaderHash,
            e.PEHeaderChecksum.has_value() ? std::to_string(*e.PEHeaderChecksum) : "",
            std::to_string(e.BinProductVersion),
            std::to_string(e.BinFileVersion),
            std::to_string(e.LinkerVersion),
            std::to_string(e.BinaryType),
            std::to_string(e.IsLocal),
            std::to_string(e.GuessProgramID),
            FormatTimestamp(e.Created),
            FormatTimestamp(e.LastModified),
            FormatTimestamp(e.LastModifiedStore),
            FormatTimestamp(e.LinkDate),
            e.LanguageID.has_value() ? std::to_string(*e.LanguageID) : "",
            e.ProductName,
            e.CompanyName,
            e.SwitchBackContext
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// Old Format - ProgramsEntryOld
// 0:ProgramID 1:LastWriteTimestamp 2:ProgramName 3:ProgramVersion 4:VendorName
// 5:InstallDateEpoch_a 6:InstallDateEpoch_b 7:LanguageCode 8:InstallSource
// 9:UninstallRegistryKey 10:PathsList
// ============================================================================
bool CsvWriter::WriteProgramEntriesOld(const std::string& path, const std::vector<ProgramsEntryOld>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "ProgramID", "LastWriteTimestamp", "ProgramName", "ProgramVersion", "VendorName",
        "InstallDateEpoch_a", "InstallDateEpoch_b", "LanguageCode", "InstallSource",
        "UninstallRegistryKey", "PathsList"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.ProgramID,
            FormatTimestamp(e.LastWriteTimestamp),
            e.ProgramName_0,
            e.ProgramVersion_1,
            e.VendorName_2,
            FormatTimestamp(e.InstallDateEpoch_a),
            FormatTimestamp(e.InstallDateEpoch_b),
            e.LanguageCode_3,
            e.InstallSource_6,
            e.UninstallRegistryKey_7,
            e.PathsList_d
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// Shortcuts
// 0:KeyName 1:LnkName 2:KeyLastWriteTimestamp
// ============================================================================
bool CsvWriter::WriteShortcuts(const std::string& path, const std::vector<Shortcut>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = { "KeyName", "LnkName", "KeyLastWriteTimestamp" };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.KeyName,
            e.LnkName,
            FormatTimestamp(e.KeyLastWriteTimestamp)
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// DeviceContainers
// 0:KeyName 1:KeyLastWriteTimestamp 2:Categories 3:DiscoveryMethod 4:FriendlyName
// 5:Icon 6:IsActive 7:IsConnected 8:IsMachineContainer 9:IsNetworked 10:IsPaired
// 11:Manufacturer 12:ModelId 13:ModelName 14:ModelNumber 15:PrimaryCategory 16:State
// ============================================================================
bool CsvWriter::WriteDeviceContainers(const std::string& path, const std::vector<DeviceContainer>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "KeyName", "KeyLastWriteTimestamp", "Categories", "DiscoveryMethod", "FriendlyName",
        "Icon", "IsActive", "IsConnected", "IsMachineContainer", "IsNetworked", "IsPaired",
        "Manufacturer", "ModelId", "ModelName", "ModelNumber", "PrimaryCategory", "State"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.KeyName,
            FormatTimestamp(e.KeyLastWriteTimestamp),
            e.Categories,
            e.DiscoveryMethod,
            e.FriendlyName,
            e.Icon,
            e.IsActive ? "True" : "False",
            e.IsConnected ? "True" : "False",
            e.IsMachineContainer ? "True" : "False",
            e.IsNetworked ? "True" : "False",
            e.IsPaired ? "True" : "False",
            e.Manufacturer,
            e.ModelId,
            e.ModelName,
            e.ModelNumber,
            e.PrimaryCategory,
            e.State
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// DevicePnps
// 0:KeyName 1:KeyLastWriteTimestamp 2:BusReportedDescription 3:Class 4:ClassGuid
// 5:Compid 6:ContainerId 7:Description 8:DriverId 9:DriverPackageStrongName
// 10:DriverName 11:DriverVerDate 12:DriverVerVersion 13:Enumerator 14:HWID
// 15:Inf 16:InstallState 17:Manufacturer 18:MatchingId 19:Model 20:ParentId
// 21:ProblemCode 22:Provider 23:Service 24:Stackid
// ============================================================================
bool CsvWriter::WriteDevicePnps(const std::string& path, const std::vector<DevicePnp>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "KeyName", "KeyLastWriteTimestamp", "BusReportedDescription", "Class", "ClassGuid",
        "Compid", "ContainerId", "Description", "DriverId", "DriverPackageStrongName",
        "DriverName", "DriverVerDate", "DriverVerVersion", "Enumerator", "HWID",
        "Inf", "InstallState", "Manufacturer", "MatchingId", "Model", "ParentId",
        "ProblemCode", "Provider", "Service", "Stackid"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.KeyName,
            FormatTimestamp(e.KeyLastWriteTimestamp),
            e.BusReportedDescription,
            e.Class,
            e.ClassGuid,
            e.Compid,
            e.ContainerId,
            e.Description,
            e.DriverId,
            e.DriverPackageStrongName,
            e.DriverName,
            e.DriverVerDate,
            e.DriverVerVersion,
            e.Enumerator,
            e.HWID,
            e.Inf,
            e.InstallState,
            e.Manufacturer,
            e.MatchingId,
            e.Model,
            e.ParentId,
            e.ProblemCode,
            e.Provider,
            e.Service,
            e.Stackid
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// DriverBinaries
// 0:KeyName 1:KeyLastWriteTimestamp 2:DriverTimeStamp 3:DriverLastWriteTime 4:DriverName
// 5:DriverInBox 6:DriverIsKernelMode 7:DriverSigned 8:DriverCheckSum 9:DriverCompany
// 10:DriverId 11:DriverPackageStrongName 12:DriverType 13:DriverVersion 14:ImageSize
// 15:Inf 16:Product 17:ProductVersion 18:Service 19:WdfVersion
// ============================================================================
bool CsvWriter::WriteDriverBinaries(const std::string& path, const std::vector<DriverBinary>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "KeyName", "KeyLastWriteTimestamp", "DriverTimeStamp", "DriverLastWriteTime", "DriverName",
        "DriverInBox", "DriverIsKernelMode", "DriverSigned", "DriverCheckSum", "DriverCompany",
        "DriverId", "DriverPackageStrongName", "DriverType", "DriverVersion", "ImageSize",
        "Inf", "Product", "ProductVersion", "Service", "WdfVersion"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.KeyName,
            FormatTimestamp(e.KeyLastWriteTimestamp),
            FormatTimestamp(e.DriverTimeStamp),
            FormatTimestamp(e.DriverLastWriteTime),
            e.DriverName,
            e.DriverInBox ? "True" : "False",
            e.DriverIsKernelMode ? "True" : "False",
            e.DriverSigned ? "True" : "False",
            std::to_string(e.DriverCheckSum),
            e.DriverCompany,
            e.DriverId,
            e.DriverPackageStrongName,
            e.DriverType,
            e.DriverVersion,
            std::to_string(e.ImageSize),
            e.Inf,
            e.Product,
            e.ProductVersion,
            e.Service,
            e.WdfVersion
        };
        WriteCsvLine(file, values);
    }

    return true;
}

// ============================================================================
// DriverPackages
// 0:KeyName 1:KeyLastWriteTimestamp 2:Date 3:Class 4:Directory 5:DriverInBox
// 6:Hwids 7:Inf 8:Provider 9:SubmissionId 10:SYSFILE 11:Version
// ============================================================================
bool CsvWriter::WriteDriverPackages(const std::string& path, const std::vector<DriverPackage>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<std::string> headers = {
        "KeyName", "KeyLastWriteTimestamp", "Date", "Class", "Directory",
        "DriverInBox", "Hwids", "Inf", "Provider", "SubmissionId", "SYSFILE", "Version"
    };
    WriteCsvLine(file, headers);

    for (const auto& e : entries) {
        std::vector<std::string> values = {
            e.KeyName,
            FormatTimestamp(e.KeyLastWriteTimestamp),
            FormatTimestamp(e.Date),
            e.Class,
            e.Directory,
            e.DriverInBox ? "True" : "False",
            e.Hwids,
            e.Inf,
            e.Provider,
            e.SubmissionId,
            e.SYSFILE,
            e.Version
        };
        WriteCsvLine(file, values);
    }

    return true;
}

} // namespace amcache
