/**
 * Tool Framework - Base Interfaces
 *
 * Provides extensible infrastructure for simulator/synthesis tools.
 * Users can create custom tools by implementing the Tool interface
 * and registering custom parsers.
 */

#pragma once

#include <filesystem>
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include "parsers/monitor_parser.hpp"

namespace fs = std::filesystem;

namespace tools {

// Reuse CycleData from monitor_parser for compatibility
using CycleData = verification::CycleData;

/**
 * Result from tool execution
 *
 * Supports both combinational (single value) and sequential (cycle data)
 */
struct ToolResult {
    bool success;               // false -> tool crashed / failed
    fs::path log;               // Main log file for inspection

    // For combinational circuits: single output value
    std::optional<uint32_t> value;

    // For sequential circuits: cycle-by-cycle data
    std::vector<CycleData> cycles;

    // VCD file (if generated)
    std::optional<fs::path> vcd;

    // Additional metadata
    std::string tool_name;
    std::string error_msg;      // Empty if success=true

    // Helpers
    bool isCombinational() const { return value.has_value(); }
    bool isSequential() const { return !cycles.empty(); }
};

/**
 * Base Tool Interface
 *
 * All simulation/synthesis tools implement this interface.
 * The framework supports both simple single-value outputs (combinational)
 * and cycle-by-cycle outputs (sequential).
 */
class Tool {
public:
    virtual ~Tool() = default;

    /**
     * Short identifier for this tool (e.g., "icarus", "verilator", "quartus")
     */
    virtual std::string name() const = 0;

    /**
     * Run the tool on given RTL
     *
     * @param rtl       Path to Verilog RTL file
     * @param top       Top module name
     * @param workdir   Working directory for this run
     * @return ToolResult with success status and output data
     */
    virtual ToolResult run(
        const fs::path& rtl,
        const std::string& top,
        const fs::path& workdir
    ) = 0;

    /**
     * Check if tool is available on system
     */
    virtual bool isAvailable() const = 0;
};

/**
 * Output Parser Interface
 *
 * Allows users to define custom parsers for different output formats.
 * Examples: VCD parser, custom monitor format, synthesis report parser
 */
class OutputParser {
public:
    virtual ~OutputParser() = default;

    /**
     * Parse tool output into structured cycle data
     */
    virtual std::vector<CycleData> parse(const fs::path& output_file) = 0;

    /**
     * Check if this parser can handle the given file
     */
    virtual bool canParse(const fs::path& file) const = 0;
};

/**
 * Testbench Generator Interface
 *
 * Generates testbenches with appropriate stimulus.
 * Users can create custom testbench generators for different verification scenarios.
 */
class TestbenchGenerator {
public:
    virtual ~TestbenchGenerator() = default;

    /**
     * Generate testbench for the given top module
     *
     * @param top           Top module name
     * @param output_path   Where to write testbench
     * @param params        Tool-specific parameters (e.g., input values, clock period)
     */
    virtual void generate(
        const std::string& top,
        const fs::path& output_path,
        const std::map<std::string, std::string>& params
    ) = 0;
};

} // namespace tools
