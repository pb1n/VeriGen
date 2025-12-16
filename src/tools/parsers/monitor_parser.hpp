#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace verification {

/**
 * Represents signal values at a specific simulation time
 *
 * Example: Time=5000 clk=1 rst=0 count=3
 * becomes: {time: 5000, signals: {{"clk", 1}, {"rst", 0}, {"count", 3}}}
 */
struct CycleData {
    int time;  // Simulation time in picoseconds
    std::map<std::string, uint32_t> signals;  // signal_name → value

    CycleData() : time(0) {}
};

/**
 * Parse Icarus Verilog $monitor output into structured cycle data
 *
 * Input format (from $monitor):
 *   Time=0 clk=0 rst=1 count=  x
 *   Time=5000 clk=1 rst=1 count=  0
 *   Time=10000 clk=0 rst=0 count=  0
 *   Time=15000 clk=1 rst=0 count=  1
 *
 * TODO: Implement parsing logic
 * - Split by lines
 * - For each line starting with "Time=", parse the time value
 * - Parse signal=value pairs (handle spaces, x for unknown, decimal/hex)
 * - Skip VCD info lines, $finish messages, etc.
 * - Handle multi-bit values with proper parsing
 *
 * @param log_output Full simulation log from IcarusSimulator
 * @return Vector of cycle data, one entry per time step
 */ 
std::vector<CycleData> parseMonitorOutput(const std::string& log_output);

/**
 * Helper: Parse a single signal value string to uint32_t
 *
 * Examples:
 *   "0" → 0
 *   "1" → 1
 *   "  5" → 5 (with leading spaces)
 *   " 10" → 10
 *   "  x" → 0 (treat unknown as 0, or could throw exception)
 *   "255" → 255
 *
 * TODO: Implement value parsing
 * - Trim whitespace
 * - Handle 'x' (unknown) and 'z' (high-impedance) - decide on policy
 * - Parse decimal numbers
 * - Could extend to handle hex (0xAB) if needed
 *
 * @param value_str String representation of signal value
 * @return Parsed uint32_t value
 */
uint32_t parseSignalValue(const std::string& value_str);

} // namespace verification
