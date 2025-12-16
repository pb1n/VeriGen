#include "monitor_parser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace verification {

// Parse a single signal value string to uint32_t.
uint32_t parseSignalValue(const std::string& value_str) {
    // Trim whitespace
    std::string trimmed = value_str;
    size_t start = trimmed.find_first_not_of(" \t");
    size_t end = trimmed.find_last_not_of(" \t");
    trimmed = trimmed.substr(start, end - start + 1);

    // Handle special cases
    if (trimmed == "x" || trimmed == "X" || trimmed == "z" || trimmed == "Z") {
        return 0;
    }

    // Parse decimal number
    return std::stoul(trimmed);
}

// Parse Icarus Verilog $monitor output into structured cycle data
std::vector<CycleData> parseMonitorOutput(const std::string& log_output) {
    std::vector<CycleData> cycles;
    std::istringstream iss(log_output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("Time=") == 0) {
            CycleData cycle;
            // Parse time: extract number after "Time="
            size_t space_after_time = line.find(' ', 5);
            cycle.time = std::stoi(line.substr(5, space_after_time));

            std::string signals_part = line.substr(space_after_time + 1);
            std::istringstream signal_stream(signals_part);
            std::string token;
            while (signal_stream >> token) {  // Reads space-separated tokens
                // token might be "clk=1" or "count="
                // Find the '=' position
                size_t eq_pos = token.find('=');
                if (eq_pos != std::string::npos) {
                    std::string signal_name = token.substr(0, eq_pos);
                    std::string value_str = token.substr(eq_pos + 1);
                    
                    // Value might be empty, e.g., "count=" with value on next token
                    if (value_str.empty()) {
                        if (!(signal_stream >> value_str)) {
                            break; // End of line reached unexpectedly
                        }
                    }
                    cycle.signals[signal_name] = parseSignalValue(value_str);

                }
            }
            cycles.push_back(cycle);
        }
    }
    return cycles;
}

} // namespace verification
