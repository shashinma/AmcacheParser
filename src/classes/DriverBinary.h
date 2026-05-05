#pragma once

#include "Common.h"

namespace amcache {

struct DriverBinary {
    std::string KeyName;
    Timestamp KeyLastWriteTimestamp;
    int32_t DriverCheckSum = 0;
    std::string DriverCompany;
    std::string DriverId;
    bool DriverInBox = false;
    bool DriverIsKernelMode = false;
    OptionalTimestamp DriverLastWriteTime;
    std::string DriverName;
    std::string DriverPackageStrongName;
    bool DriverSigned = false;
    OptionalTimestamp DriverTimeStamp;
    std::string DriverType;
    std::string DriverVersion;
    int32_t ImageSize = 0;
    std::string Inf;
    std::string Product;
    std::string ProductVersion;
    std::string Service;
    std::string WdfVersion;

    DriverBinary() = default;
};

} // namespace amcache
