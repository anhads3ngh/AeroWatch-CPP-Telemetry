#include "aerowatch/simulator.hpp"
#include "aerowatch/telemetry.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace aerowatch {

std::vector<std::string> generate_demo_packets(std::size_t count, bool inject_faults) {
    std::vector<std::string> lines;
    lines.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const bool second_vehicle = (i % 2) == 1;
        const auto step = static_cast<double>(i / 2);
        Telemetry t;
        t.timestamp_ms = 1000 + static_cast<std::uint64_t>(i / 2) * 1000;
        t.vehicle_id = second_vehicle ? "BRAVO02" : "ALPHA01";
        t.latitude_deg = 51.04 + (second_vehicle ? 0.02 : 0.0) + step * 0.0001;
        t.longitude_deg = -114.07 + (second_vehicle ? 0.015 : 0.0) + step * 0.0001;
        t.altitude_m = 120.0 + step * 4 + (second_vehicle ? 15.0 : 0.0);
        t.speed_mps = 12 + step * 0.3;
        t.battery_percent = std::max(0.0, 92.0 - step * (second_vehicle ? 1.5 : 6.0));
        t.link_percent = std::max(0.0, 97.0 - step * (second_vehicle ? 11.0 : 2.0));
        if (inject_faults && i == 9) {
            lines.push_back("malformed,sample,not,a,real,telemetry,packet");
            continue;
        }
        if (inject_faults && i == 13) t.timestamp_ms -= 2000;
        lines.push_back(format_telemetry(t));
    }
    return lines;
}

} // namespace aerowatch
