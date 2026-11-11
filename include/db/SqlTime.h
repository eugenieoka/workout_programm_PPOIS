#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace workout::infra::sql {

// ISO 8601 UTC, for example "2026-03-20T14:22:00Z".
inline std::string toIso(std::chrono::system_clock::time_point tp) {
    const std::time_t time = std::chrono::system_clock::to_time_t(tp);

    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

inline std::chrono::system_clock::time_point fromIso(const std::string& iso) {
    std::tm utc{};
    std::istringstream stream(iso);
    stream >> std::get_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    if (stream.fail()) {
        throw std::runtime_error("Invalid ISO-8601 timestamp in database: " + iso);
    }

#if defined(_WIN32)
    const std::time_t time = _mkgmtime(&utc);
#else
    const std::time_t time = timegm(&utc);
#endif

    if (time == static_cast<std::time_t>(-1)) {
        throw std::runtime_error("Failed to convert timestamp: " + iso);
    }
    return std::chrono::system_clock::from_time_t(time);
}

inline std::string nowIso() {
    return toIso(std::chrono::system_clock::now());
}

} // namespace workout::infra::sql