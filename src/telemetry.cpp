#include "aerowatch/telemetry.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace aerowatch {
namespace {
std::string trim(std::string s) {
    const auto whitespace = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto start = std::find_if_not(s.begin(), s.end(), whitespace);
    const auto stop = std::find_if_not(s.rbegin(), s.rend(), whitespace).base();
    return start >= stop ? "" : std::string(start, stop);
}

double parse_real(const std::string& raw, const char* name) {
    if (raw.empty()) throw ParseError(std::string(name) + " is missing");
    std::size_t parsed = 0;
    double n = 0;
    try { n = std::stod(raw, &parsed); }
    catch (const std::exception&) { throw ParseError(std::string(name) + " is not numeric"); }
    if (parsed != raw.size() || !std::isfinite(n))
        throw ParseError(std::string(name) + " must be finite and numeric");
    return n;
}

void in_range(double x, double lo, double hi, const char* field) {
    if (x < lo || x > hi) throw ParseError(std::string(field) + " out of range");
}
} // namespace

Telemetry parse_telemetry(std::string_view line) {
    const std::string input(line);
    std::istringstream stream(input);
    std::vector<std::string> fields;
    std::string field;
    while (std::getline(stream, field, ',')) fields.push_back(trim(field));
    if (!input.empty() && input.back() == ',') fields.emplace_back("");
    if (fields.size() != 8) throw ParseError("expected exactly 8 CSV fields");

    Telemetry packet;
    if (fields[0].empty() || fields[0][0] == '-' || fields[0][0] == '+')
        throw ParseError("timestamp_ms must be a positive integer");
    std::size_t parsed = 0;
    try { packet.timestamp_ms = std::stoull(fields[0], &parsed); }
    catch (const std::exception&) { throw ParseError("timestamp_ms is invalid"); }
    if (parsed != fields[0].size() || packet.timestamp_ms == 0)
        throw ParseError("timestamp_ms must be a positive integer");

    packet.vehicle_id = fields[1];
    if (packet.vehicle_id.empty() || packet.vehicle_id.size() > 32 ||
        !std::all_of(packet.vehicle_id.begin(), packet.vehicle_id.end(),
                     [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-'; }))
        throw ParseError("invalid vehicle_id; use 1-32 alphanumeric, _ or - characters");

    packet.latitude_deg = parse_real(fields[2], "latitude_deg");
    packet.longitude_deg = parse_real(fields[3], "longitude_deg");
    packet.altitude_m = parse_real(fields[4], "altitude_m");
    packet.speed_mps = parse_real(fields[5], "speed_mps");
    packet.battery_percent = parse_real(fields[6], "battery_percent");
    packet.link_percent = parse_real(fields[7], "link_percent");

    in_range(packet.latitude_deg, -90.0, 90.0, "latitude_deg");
    in_range(packet.longitude_deg, -180.0, 180.0, "longitude_deg");
    in_range(packet.altitude_m, -500.0, 50000.0, "altitude_m");
    in_range(packet.speed_mps, 0.0, 250.0, "speed_mps");
    in_range(packet.battery_percent, 0.0, 100.0, "battery_percent");
    in_range(packet.link_percent, 0.0, 100.0, "link_percent");
    return packet;
}

std::string format_telemetry(const Telemetry& t) {
    std::ostringstream s;
    s << t.timestamp_ms << ',' << t.vehicle_id << ','
      << std::fixed << std::setprecision(6) << t.latitude_deg << ',' << t.longitude_deg << ','
      << std::setprecision(2) << t.altitude_m << ',' << t.speed_mps << ','
      << t.battery_percent << ',' << t.link_percent;
    return s.str();
}

} // namespace aerowatch
