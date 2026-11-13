#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace aerowatch {

// Deterministic, synthetic data, never sent to any vehicle.
std::vector<std::string> generate_demo_packets(std::size_t packet_count,
                                                bool inject_faults = false);

} // namespace aerowatch
