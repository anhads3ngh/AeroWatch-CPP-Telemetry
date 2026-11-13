#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace aerowatch {

// Purely simulated telemetry. This is NOT an implementation of MAVLink or STANAG 4586.
struct Telemetry {
    std::uint64_t timestamp_ms{};
    std::string vehicle_id;
    double latitude_deg{};
    double longitude_deg{};
    double altitude_m{};
    double speed_mps{};
    double battery_percent{};
    double link_percent{};
};

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline constexpr std::string_view csv_header =
    "timestamp_ms,vehicle_id,latitude_deg,longitude_deg,altitude_m,speed_mps,battery_percent,link_percent";

Telemetry parse_telemetry(std::string_view csv_line);
std::string format_telemetry(const Telemetry& telemetry);

} // namespace aerowatch
