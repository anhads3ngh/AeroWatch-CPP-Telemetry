#include "aerowatch/monitor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aerowatch {

std::string_view health_label(Health health) {
    switch (health) {
        case Health::Nominal: return "NOMINAL";
        case Health::Warning: return "WARNING";
        case Health::Critical: return "CRITICAL";
        case Health::Offline: return "OFFLINE";
    }
    return "UNKNOWN";
}

TelemetryMonitor::TelemetryMonitor(MonitorConfig config) : config_(config) {
    if (config_.stale_after_ms == 0 || config_.smoothing_factor <= 0 ||
        config_.smoothing_factor > 1 ||
        config_.low_battery_critical > config_.low_battery_warning ||
        config_.low_link_critical > config_.low_link_warning ||
        !std::isfinite(config_.smoothing_factor))
        throw std::invalid_argument("invalid monitor configuration");
}

bool TelemetryMonitor::ingest(const Telemetry& t, std::string* rejection) {
    const auto it = vehicles_.find(t.vehicle_id);
    if (it != vehicles_.end() && t.timestamp_ms <= it->second.latest.timestamp_ms) {
        if (rejection) *rejection = "duplicate or out-of-order timestamp for " + t.vehicle_id;
        return false;
    }
    if (it == vehicles_.end()) {
        vehicles_.emplace(t.vehicle_id, Reading{t, t.battery_percent, t.link_percent});
    } else {
        auto& current = it->second;
        const auto alpha = config_.smoothing_factor;
        current.smoothed_battery = alpha * t.battery_percent + (1 - alpha) * current.smoothed_battery;
        current.smoothed_link = alpha * t.link_percent + (1 - alpha) * current.smoothed_link;
        current.latest = t;
    }
    return true;
}

std::vector<Snapshot> TelemetryMonitor::snapshots(std::uint64_t now) const {
    std::vector<Snapshot> result;
    result.reserve(vehicles_.size());
    for (const auto& [name, r] : vehicles_) {
        Health health = Health::Nominal;
        const bool stale = now >= r.latest.timestamp_ms &&
                           (now - r.latest.timestamp_ms) > config_.stale_after_ms;
        if (stale) health = Health::Offline;
        else if (r.latest.battery_percent <= config_.low_battery_critical ||
                 r.latest.link_percent <= config_.low_link_critical) health = Health::Critical;
        else if (r.latest.battery_percent <= config_.low_battery_warning ||
                 r.latest.link_percent <= config_.low_link_warning) health = Health::Warning;
        result.push_back(Snapshot{r.latest, health, r.smoothed_battery, r.smoothed_link});
    }
    std::sort(result.begin(), result.end(), [](const Snapshot& a, const Snapshot& b) {
        return a.latest.vehicle_id < b.latest.vehicle_id;
    });
    return result;
}

} // namespace aerowatch
