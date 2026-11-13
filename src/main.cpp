#include "aerowatch/monitor.hpp"
#include "aerowatch/simulator.hpp"
#include "aerowatch/telemetry.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Options {
    bool faults{false};
    std::size_t samples{24};
    std::string replay;
    std::string csv_out;
};

void usage(const char* name) {
    std::cout << "AeroWatch - offline C++ simulated telemetry monitor (NO vehicle control)\n"
              << "Usage: " << name << " [--samples N] [--faults] [--csv-out FILE]\n"
              << "       " << name << " --replay FILE\n"
              << "       " << name << " --help\n";
}

Options arguments(int argc, char** argv) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") { usage(argv[0]); std::exit(0); }
        if (arg == "--faults") opts.faults = true;
        else if (arg == "--samples" || arg == "--replay" || arg == "--csv-out") {
            if (++i >= argc) throw std::invalid_argument("missing value for " + arg);
            const std::string value(argv[i]);
            if (arg == "--samples") {
                std::size_t consumed = 0;
                try { opts.samples = std::stoull(value, &consumed); }
                catch (const std::exception&) { throw std::invalid_argument("invalid --samples"); }
                if (consumed != value.size() || opts.samples < 1 || opts.samples > 100000)
                    throw std::invalid_argument("--samples must be in [1, 100000]");
            } else if (arg == "--replay") opts.replay = value;
            else opts.csv_out = value;
        } else throw std::invalid_argument("unknown argument: " + arg);
    }
    if (!opts.replay.empty() && !opts.csv_out.empty())
        throw std::invalid_argument("--replay and --csv-out cannot be combined");
    return opts;
}

std::vector<std::string> load_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("cannot open replay file: " + filename);
    std::vector<std::string> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line == aerowatch::csv_header) continue;
        rows.push_back(line);
    }
    return rows;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = arguments(argc, argv);
        const auto rows = options.replay.empty()
            ? aerowatch::generate_demo_packets(options.samples, options.faults)
            : load_file(options.replay);
        if (!options.csv_out.empty()) {
            std::ofstream out(options.csv_out);
            if (!out) throw std::runtime_error("cannot write CSV: " + options.csv_out);
            out << aerowatch::csv_header << '\n';
            for (const auto& row : rows) out << row << '\n';
            if (!out) throw std::runtime_error("failed to write CSV output");
        }
        aerowatch::TelemetryMonitor monitor;
        std::uint64_t last_timestamp = 0;
        std::size_t accepted = 0, rejected = 0;
        std::cout << "AEROWATCH | deterministic offline simulation | no flight controls\n";
        std::cout << "Vehicle   Time(ms)    Battery   Link     Alt(m)    Status\n";
        std::cout << "--------------------------------------------------------------\n";
        for (const auto& row : rows) {
            try {
                const auto packet = aerowatch::parse_telemetry(row);
                std::string reason;
                if (!monitor.ingest(packet, &reason)) {
                    ++rejected;
                    std::cerr << "[REJECTED] " << reason << '\n';
                    continue;
                }
                ++accepted;
                last_timestamp = std::max(last_timestamp, packet.timestamp_ms);
                const auto snapshots = monitor.snapshots(last_timestamp);
                const auto it = std::find_if(snapshots.begin(), snapshots.end(), [&](const auto& snapshot) {
                    return snapshot.latest.vehicle_id == packet.vehicle_id;
                });
                std::cout << std::left << std::setw(10) << packet.vehicle_id
                          << std::right << std::setw(8) << packet.timestamp_ms << "    "
                          << std::fixed << std::setprecision(1)
                          << std::setw(5) << packet.battery_percent << "%   "
                          << std::setw(5) << packet.link_percent << "%   "
                          << std::setw(7) << packet.altitude_m << "   "
                          << aerowatch::health_label(it->health) << '\n';
            } catch (const aerowatch::ParseError& e) {
                ++rejected;
                std::cerr << "[REJECTED] " << e.what() << '\n';
            }
        }
        std::cout << "\nTelemetry received: " << accepted << " accepted / " << rejected
                  << " rejected; vehicles observed: " << monitor.vehicle_count() << "\n";
        std::cout << "Final health status:\n";
        for (const auto& snapshot : monitor.snapshots(last_timestamp + 1000)) {
            std::cout << "  " << snapshot.latest.vehicle_id << ": "
                      << aerowatch::health_label(snapshot.health) << " (smoothed link "
                      << std::fixed << std::setprecision(1)
                      << snapshot.smoothed_link_percent << "%)\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        usage(argv[0]);
        return 1;
    }
}
