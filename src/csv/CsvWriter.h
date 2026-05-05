#pragma once

#include <string>
#include <vector>
#include <fstream>
#include "../classes/FileEntryNew.h"
#include "../classes/FileEntryOld.h"
#include "../classes/ProgramsEntryNew.h"
#include "../classes/ProgramsEntryOld.h"
#include "../classes/Shortcut.h"
#include "../classes/DeviceContainer.h"
#include "../classes/DevicePnp.h"
#include "../classes/DriverBinary.h"
#include "../classes/DriverPackage.h"

namespace amcache {

class CsvWriter {
public:
    explicit CsvWriter(const std::string& dateFormat = "%Y-%m-%d %H:%M:%S", bool useMicroseconds = false);

    bool WriteFileEntriesNew(const std::string& path, const std::vector<FileEntryNew>& entries);
    bool WriteFileEntriesOld(const std::string& path, const std::vector<FileEntryOld>& entries);
    bool WriteProgramEntriesNew(const std::string& path, const std::vector<ProgramsEntryNew>& entries);
    bool WriteProgramEntriesOld(const std::string& path, const std::vector<ProgramsEntryOld>& entries);
    bool WriteShortcuts(const std::string& path, const std::vector<Shortcut>& entries);
    bool WriteDeviceContainers(const std::string& path, const std::vector<DeviceContainer>& entries);
    bool WriteDevicePnps(const std::string& path, const std::vector<DevicePnp>& entries);
    bool WriteDriverBinaries(const std::string& path, const std::vector<DriverBinary>& entries);
    bool WriteDriverPackages(const std::string& path, const std::vector<DriverPackage>& entries);

private:
    std::string dateFormat_;
    bool useMicroseconds_;

    static std::string EscapeCsv(const std::string& field);
    std::string FormatTimestamp(const Timestamp& ts) const;
    std::string FormatTimestamp(const OptionalTimestamp& ts) const;
    static void WriteCsvLine(std::ofstream& file, const std::vector<std::string>& values);
};

} // namespace amcache
