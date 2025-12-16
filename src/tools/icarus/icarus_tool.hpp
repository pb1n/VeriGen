/**
 * Icarus Verilog Simulator Tool
 *
 * Supports both combinational (single value) and sequential (monitor-based) verification.
 */

#pragma once

#include "../tool_base.hpp"
#include "../parsers/monitor_parser.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace tools {

/**
 * Testbench type for Icarus simulation
 */
enum class IcarusTestbenchType {
    Combinational,  // Single-value output ($display RES=xxxxx)
    Sequential      // Sequential with $monitor and VCD
};

/**
 * Configuration for Icarus tool
 */
struct IcarusConfig {
    IcarusTestbenchType tb_type = IcarusTestbenchType::Combinational;
    bool verbose = false;
    bool generate_vcd = false;
    int sim_time_ns = 100;  // For sequential simulations

    // Port configuration (for testbench generation)
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};

/**
 * Icarus Verilog Tool
 *
 * Wraps iverilog + vvp for simulation.
 * Supports both combinational and sequential verification.
 */
class IcarusTool : public Tool {
public:
    explicit IcarusTool(const IcarusConfig& config = IcarusConfig{})
        : config_(config) {}

    std::string name() const override { return "icarus"; }

    bool isAvailable() const override {
        // Try running iverilog -V (version check)
        int ret = std::system(
#ifdef _WIN32
            "iverilog -V >nul 2>&1"
#else
            "iverilog -V >/dev/null 2>&1"
#endif
        );
        return ret == 0;
    }

    ToolResult run(
        const fs::path& rtl,
        const std::string& top,
        const fs::path& workdir
    ) override;

private:
    IcarusConfig config_;

    /**
     * Generate combinational testbench (single value output)
     */
    void generateCombTestbench(const fs::path& path, const std::string& top);

    /**
     * Generate sequential testbench (with clock and monitor)
     */
    void generateSeqTestbench(
        const fs::path& path,
        const std::string& top,
        const fs::path& vcd_path
    );

    /**
     * Parse combinational output (RES=xxxxx line)
     */
    ToolResult parseCombOutput(const fs::path& log);

    /**
     * Parse sequential output (monitor lines)
     */
    ToolResult parseSeqOutput(const fs::path& log, const fs::path& vcd);
};

} // namespace tools
