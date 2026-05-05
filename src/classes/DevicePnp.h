#pragma once

#include "Common.h"

namespace amcache {

struct DevicePnp {
    std::string KeyName;
    Timestamp KeyLastWriteTimestamp;
    std::string BusReportedDescription;
    std::string Class;
    std::string ClassGuid;
    std::string Compid;
    std::string ContainerId;
    std::string Description;
    std::string DeviceState;
    std::string DriverId;
    std::string DriverName;
    std::string DriverPackageStrongName;
    std::string DriverVerDate;
    std::string DriverVerVersion;
    std::string Enumerator;
    std::string HWID;
    std::string Inf;
    std::string InstallState;
    std::string Manufacturer;
    std::string MatchingId;
    std::string Model;
    std::string ParentId;
    std::string ProblemCode;
    std::string Provider;
    std::string Service;
    std::string Stackid;

    DevicePnp() = default;
};

} // namespace amcache
