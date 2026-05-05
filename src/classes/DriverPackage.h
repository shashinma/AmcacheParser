#pragma once

#include "Common.h"

namespace amcache {

struct DriverPackage {
    std::string KeyName;
    Timestamp KeyLastWriteTimestamp;
    std::string Class;
    std::string ClassGuid;
    OptionalTimestamp Date;
    std::string Directory;
    bool DriverInBox = false;
    std::string Hwids;
    std::string Inf;
    std::string Provider;
    std::string SubmissionId;
    std::string SYSFILE;
    std::string Version;

    DriverPackage() = default;
};

} // namespace amcache
