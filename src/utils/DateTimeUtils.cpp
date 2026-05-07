#include "DateTimeUtils.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>

namespace amcache {

DateTimeUtils::Timestamp DateTimeUtils::FromUnixSeconds(int64_t seconds) {
    return std::chrono::system_clock::from_time_t(static_cast<time_t>(seconds));
}

DateTimeUtils::Timestamp DateTimeUtils::FromUnixMilliseconds(int64_t milliseconds) {
    auto duration = std::chrono::milliseconds(milliseconds);
    return Timestamp(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
}

DateTimeUtils::Timestamp DateTimeUtils::FromFileTime(uint64_t filetime) {
    if (filetime < FILETIME_UNIX_DIFF) {
        return Timestamp{};
    }
    uint64_t unix_100ns = filetime - FILETIME_UNIX_DIFF;
    auto duration = std::chrono::duration<uint64_t, std::ratio<1, 10000000>>(unix_100ns);
    return Timestamp(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
}

DateTimeUtils::OptionalTimestamp DateTimeUtils::FromFileTimeSafe(uint64_t filetime) {
    if (filetime < MIN_VALID_FILETIME || filetime == 0) {
        return std::nullopt;
    }
    auto ts = FromFileTime(filetime);
    if (!IsValid(ts)) {
        return std::nullopt;
    }
    return ts;
}

DateTimeUtils::OptionalTimestamp DateTimeUtils::ParseDateTime(const std::string& dateStr) {
    if (dateStr.empty()) {
        return std::nullopt;
    }

    std::tm tm = {};
    std::istringstream ss(dateStr);

    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (!ss.fail()) {
        return FromUnixSeconds(std::mktime(&tm));
    }

    ss.clear();
    ss.str(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (!ss.fail()) {
        return FromUnixSeconds(std::mktime(&tm));
    }

    ss.clear();
    ss.str(dateStr);
    ss >> std::get_time(&tm, "%m/%d/%Y");
    if (!ss.fail()) {
        return FromUnixSeconds(std::mktime(&tm));
    }

    ss.clear();
    ss.str(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (!ss.fail()) {
        return FromUnixSeconds(std::mktime(&tm));
    }

    ss.clear();
    ss.str(dateStr);
    ss >> std::get_time(&tm, "%m/%d/%Y %H:%M:%S");
    if (!ss.fail()) {
        return FromUnixSeconds(std::mktime(&tm));
    }

    return std::nullopt;
}

std::string DateTimeUtils::Format(const Timestamp& ts, const std::string& format) {
    auto time_t_val = std::chrono::system_clock::to_time_t(ts);

    // Handle .NET DateTimeOffset.MinValue (0001-01-01 00:00:00)
    if (time_t_val == MIN_VALUE_SECONDS) {
        return "0001-01-01 00:00:00";
    }

    if (!IsValid(ts)) {
        return "";
    }

    std::tm tm = {};

#ifdef _WIN32
    gmtime_s(&tm, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, format.c_str());
    return ss.str();
}

std::string DateTimeUtils::Format(const OptionalTimestamp& ts, const std::string& format) {
    if (!ts.has_value()) {
        return "";
    }
    return Format(*ts, format);
}

std::string DateTimeUtils::FormatMicroseconds(const Timestamp& ts) {
    auto time_t_val = std::chrono::system_clock::to_time_t(ts);

    // Handle .NET DateTimeOffset.MinValue (0001-01-01 00:00:00.0000000)
    if (time_t_val == MIN_VALUE_SECONDS) {
        return "0001-01-01 00:00:00.0000000";
    }

    if (!IsValid(ts)) {
        return "";
    }

    std::tm tm = {};

#ifdef _WIN32
    gmtime_s(&tm, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm);
#endif

    auto since_epoch = ts.time_since_epoch();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    // .NET uses 7 digits (ticks / 100ns). Convert nanoseconds to 100-nanosecond ticks.
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - secs);
    uint64_t ticks = nanos.count() / 100; // 100-nanosecond ticks
    if (ticks > 9999999) ticks = 9999999;

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(7) << ticks;
    return ss.str();
}

DateTimeUtils::Timestamp DateTimeUtils::Now() {
    return std::chrono::system_clock::now();
}

bool DateTimeUtils::IsValid(const Timestamp& ts) {
    static const Timestamp epoch{};
    if (ts == epoch) {
        return false;
    }
    auto time_t_val = std::chrono::system_clock::to_time_t(ts);
    // Allow .NET DateTimeOffset.MinValue (0001-01-01)
    if (time_t_val == MIN_VALUE_SECONDS) {
        return true;
    }
    if (time_t_val <= 0) {
        return false;
    }
    constexpr time_t max_reasonable = 4102444800;
    if (time_t_val > max_reasonable) {
        return false;
    }
    return true;
}

bool DateTimeUtils::IsValid(const OptionalTimestamp& ts) {
    if (!ts.has_value()) {
        return false;
    }
    return IsValid(*ts);
}

DateTimeUtils::Timestamp DateTimeUtils::MinValue() {
    return Timestamp(std::chrono::seconds(MIN_VALUE_SECONDS));
}

} // namespace amcache
