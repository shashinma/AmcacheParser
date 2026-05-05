#pragma once

#include "Common.h"

namespace amcache {

struct Shortcut {
    std::string KeyName;
    std::string LnkName;
    Timestamp KeyLastWriteTimestamp;

    Shortcut() = default;
    Shortcut(const std::string& keyName, const std::string& lnkName, Timestamp ts)
        : KeyName(keyName), LnkName(lnkName), KeyLastWriteTimestamp(ts) {}
};

} // namespace amcache
