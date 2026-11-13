#pragma once

#include "aerowatch/telemetry.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace aerowatch {

enum class Health { Nominal, Warning, Critical, Offline };
std::string_view health_label(Health health);

struct MonitorConfig {
    std::uint64_t stale_after_ms{3500};
    double low_battery_warning{35.0};
    double low_battery_critical{20.0};
    double low_link_warning{45.0};
    double low_link_critical{20.0};
    double smoothing_factor{0.35};
};

struct Snapshot {
    Telemetry latest;
    Health health{Health::Offline};
    double smoothed_battery_percent{};
    double smoothed_link_percent{};
};

class TelemetryMonitor {
public:
    explicit TelemetryMonitor(MonitorConfig config = {});
    // Rejects duplicate or out-of-order vehicle timestamps without corrupting state.
    bool ingest(const Telemetry& telemetry, std::string* rejection = nullptr);
    std::vector<Snapshot> snapshots(std::uint64_t current_time_ms) const;
    std::size_t vehicle_count() const noexcept { return vehicles_.size(); }

private:
    struct Reading {
        Telemetry latest;
        double smoothed_battery{};
        double smoothed_link{};
    };
    MonitorConfig config_;
    std::unordered_map<std::string, Reading> vehicles_;
};

} // namespace aerowatch
