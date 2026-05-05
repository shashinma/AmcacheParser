#pragma once

#include "Common.h"

namespace amcache {

struct DeviceContainer {
    std::string KeyName;
    Timestamp KeyLastWriteTimestamp;
    std::string Categories;
    std::string DiscoveryMethod;
    std::string FriendlyName;
    std::string Icon;
    bool IsActive = false;
    bool IsConnected = false;
    bool IsMachineContainer = false;
    bool IsNetworked = false;
    bool IsPaired = false;
    std::string Manufacturer;
    std::string ModelId;
    std::string ModelName;
    std::string ModelNumber;
    std::string PrimaryCategory;
    std::string State;

    DeviceContainer() = default;
};

} // namespace amcache
