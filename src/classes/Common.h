#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <cstdint>

namespace amcache {

using Timestamp = std::chrono::system_clock::time_point;
using OptionalTimestamp = std::optional<Timestamp>;

} // namespace amcache
