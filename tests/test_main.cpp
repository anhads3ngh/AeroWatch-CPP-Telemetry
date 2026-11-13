#include "aerowatch/monitor.hpp"
#include "aerowatch/simulator.hpp"
#include "aerowatch/telemetry.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using aerowatch::Health;
using aerowatch::Telemetry;
using aerowatch::TelemetryMonitor;

namespace {
void verify(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Telemetry sample(std::uint64_t ts = 1000, std::string id = "AIR01", double battery = 85, double link = 95) {
    return {ts, std::move(id), 51.0486, -114.0708, 100.0, 10.0, battery, link};
}

bool parse_fails(const std::string& line) {
    try { (void)aerowatch::parse_telemetry(line); }
    catch (const aerowatch::ParseError&) { return true; }
    return false;
}

std::vector<std::pair<std::string, std::function<void()>>> cases = {
    {"parser_valid", [] {
        const auto t = aerowatch::parse_telemetry("1000,AIR01,51.05,-114.07,110.2,12.2,85,91");
        verify(t.vehicle_id == "AIR01" && t.timestamp_ms == 1000 && t.link_percent == 91, "valid data parsed incorrectly");
    }},
    {"parser_roundtrip", [] {
        const auto t = aerowatch::parse_telemetry(aerowatch::format_telemetry(sample()));
        verify(t.vehicle_id == "AIR01" && t.battery_percent == 85 && t.longitude_deg == -114.0708, "roundtrip failed");
    }},
    {"parser_missing_field", [] { verify(parse_fails("1000,AIR01,51,-114,120,15,90"), "missing field accepted"); }},
    {"parser_extra_field", [] { verify(parse_fails("1000,AIR01,51,-114,120,15,90,90,abc"), "extra field accepted"); }},
    {"parser_invalid_coordinate", [] { verify(parse_fails("1000,AIR01,91,-114,120,15,90,90"), "invalid latitude accepted"); }},
    {"parser_nan", [] { verify(parse_fails("1000,AIR01,51,-114,nan,15,90,90"), "non-finite number accepted"); }},
    {"parser_negative_battery", [] { verify(parse_fails("1000,AIR01,51,-114,120,15,-1,90"), "negative battery accepted"); }},
    {"parser_invalid_id", [] { verify(parse_fails("1000,AIR 01,51,-114,120,15,90,90"), "invalid ID accepted"); }},
    {"parser_zero_timestamp", [] { verify(parse_fails("0,AIR01,51,-114,120,15,90,90"), "zero timestamp accepted"); }},
    {"monitor_nominal", [] {
        TelemetryMonitor m;
        verify(m.ingest(sample()) && m.snapshots(2000)[0].health == Health::Nominal, "nominal state failed");
    }},
    {"monitor_warning_battery", [] {
        TelemetryMonitor m;
        m.ingest(sample(1000, "AIR01", 32));
        verify(m.snapshots(1000)[0].health == Health::Warning, "battery warning failed");
    }},
    {"monitor_critical_link", [] {
        TelemetryMonitor m;
        m.ingest(sample(1000, "AIR01", 90, 12));
        verify(m.snapshots(1000)[0].health == Health::Critical, "link critical failed");
    }},
    {"monitor_offline", [] {
        TelemetryMonitor m;
        m.ingest(sample());
        verify(m.snapshots(4501)[0].health == Health::Offline, "stale timeout failed");
    }},
    {"monitor_duplicate_rejection", [] {
        TelemetryMonitor m;
        m.ingest(sample());
        verify(!m.ingest(sample(1000, "AIR01", 9)) && m.snapshots(1000)[0].latest.battery_percent == 85,
               "duplicate packet must not mutate state");
    }},
    {"monitor_out_of_order", [] {
        TelemetryMonitor m;
        m.ingest(sample(2000));
        verify(!m.ingest(sample(1000)) && m.snapshots(2000)[0].latest.timestamp_ms == 2000,
               "out-of-order packet must not mutate state");
    }},
    {"monitor_vehicle_isolation", [] {
        TelemetryMonitor m;
        m.ingest(sample(1000, "ALPHA", 90));
        m.ingest(sample(1000, "BRAVO", 10));
        const auto snaps = m.snapshots(1000);
        verify(snaps.size() == 2 && snaps[0].health == Health::Nominal &&
               snaps[1].health == Health::Critical, "vehicle states incorrectly coupled");
    }},
    {"monitor_smoothing", [] {
        TelemetryMonitor m;
        m.ingest(sample(1000, "AIR01", 100, 100));
        m.ingest(sample(2000, "AIR01", 80, 80));
        const auto s = m.snapshots(2000)[0];
        verify(std::abs(s.smoothed_battery_percent - 93.0) < 1e-6, "smoothing factor incorrect");
    }},
    {"monitor_invalid_configuration", [] {
        aerowatch::MonitorConfig bad;
        bad.smoothing_factor = 0;
        bool threw = false;
        try { TelemetryMonitor m(bad); }
        catch (const std::invalid_argument&) { threw = true; }
        verify(threw, "invalid configuration accepted");
    }},
    {"simulator_determinism", [] {
        verify(aerowatch::generate_demo_packets(12) == aerowatch::generate_demo_packets(12), "demo not deterministic");
    }},
    {"simulator_fault_injection", [] {
        auto rows = aerowatch::generate_demo_packets(20, true);
        verify(rows.size() == 20 && parse_fails(rows[9]), "fault injection not available");
        const auto old = aerowatch::parse_telemetry(rows[13]);
        const auto previous = aerowatch::parse_telemetry(rows[11]);
        verify(old.timestamp_ms <= previous.timestamp_ms && old.vehicle_id == previous.vehicle_id,
               "stale packet not injected");
    }},
};
} // namespace

int main(int argc, char** argv) {
    std::string selected;
    if (argc == 3 && std::string(argv[1]) == "--case") selected = argv[2];
    else if (argc != 1) { std::cerr << "Usage: aerowatch_tests [--case NAME]\n"; return 2; }
    std::size_t executed = 0;
    for (const auto& [name, run] : cases) {
        if (!selected.empty() && name != selected) continue;
        try { run(); ++executed; std::cout << "[PASS] " << name << '\n'; }
        catch (const std::exception& e) {
            std::cerr << "[FAIL] " << name << ": " << e.what() << '\n';
            return 1;
        }
    }
    if (executed == 0) { std::cerr << "test case not found: " << selected << '\n'; return 2; }
    std::cout << "Executed " << executed << " test(s) successfully\n";
    return 0;
}
