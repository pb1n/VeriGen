/**
 * Icarus Verilog Tool Implementation
 */

#include "icarus_tool.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace tools {

ToolResult IcarusTool::run(
    const fs::path& rtl,
    const std::string& top,
    const fs::path& workdir
) {
    fs::create_directories(workdir);

    fs::path tb = workdir / "tb.v";
    fs::path ivl_log = workdir / "iverilog.log";
    fs::path vvp_out = workdir / "vvp_out.txt";
    fs::path vcd = workdir / "dump.vcd";

    // Generate testbench
    if (config_.tb_type == IcarusTestbenchType::Combinational) {
        generateCombTestbench(tb, top);
    } else {
        generateSeqTestbench(tb, top, vcd);
    }

    // Compile with iverilog
    fs::path vvp_bin = workdir / "sim.vvp";
    std::string compile_cmd =
        "iverilog -g2012 -o \"" + vvp_bin.string() + "\" -s tb \"" +
        rtl.string() + "\" \"" + tb.string() + "\"";

    if (!config_.verbose) {
#ifdef _WIN32
        compile_cmd += " >\"" + ivl_log.string() + "\" 2>&1";
#else
        compile_cmd += " >\"" + ivl_log.string() + "\" 2>&1";
#endif
    }

    if (config_.verbose) {
        std::cout << "Compiling: " << compile_cmd << "\n";
    }

    if (std::system(compile_cmd.c_str()) != 0) {
        ToolResult result;
        result.success = false;
        result.log = ivl_log;
        result.tool_name = "icarus";
        result.error_msg = "iverilog compilation failed";
        return result;
    }

    // Simulate with vvp
    // Always redirect to file for parsing, even in verbose mode
    std::string sim_cmd =
        "vvp \"" + vvp_bin.string() + "\"";

#ifdef _WIN32
    sim_cmd += " >\"" + vvp_out.string() + "\" 2>&1";
#else
    sim_cmd += " >\"" + vvp_out.string() + "\" 2>&1";
#endif

    if (config_.verbose) {
        std::cout << "Simulating: " << sim_cmd << "\n";
    }

    if (std::system(sim_cmd.c_str()) != 0) {
        ToolResult result;
        result.success = false;
        result.log = vvp_out;
        result.tool_name = "icarus";
        result.error_msg = "vvp simulation failed";
        return result;
    }

    // In verbose mode, print the output
    if (config_.verbose) {
        std::ifstream output_file(vvp_out);
        std::string line;
        while (std::getline(output_file, line)) {
            std::cout << line << "\n";
        }
    }

    // Parse output
    if (config_.tb_type == IcarusTestbenchType::Combinational) {
        return parseCombOutput(vvp_out);
    } else {
        return parseSeqOutput(vvp_out, vcd);
    }
}

void IcarusTool::generateCombTestbench(const fs::path& path, const std::string& top) {
    std::ofstream f(path);
    if (!f) {
        throw std::runtime_error("Cannot write testbench: " + path.string());
    }

    f << "`timescale 1ns/1ps\n"
      << "module tb;\n"
      << "  // Drive inputs with test values\n"
      << "  reg [31:0] in0, in1, in2, in3;\n"
      << "  wire [31:0] out;\n"
      << "  " << top << " dut(\n"
      << "    .in0(in0), .in1(in1), .in2(in2), .in3(in3),\n"
      << "    .out(out)\n"
      << "  );\n"
      << "  initial begin\n"
      << "    // Set input values\n"
      << "    in0 = 32'h12345678;\n"
      << "    in1 = 32'hABCDEF00;\n"
      << "    in2 = 32'h55AA55AA;\n"
      << "    in3 = 32'hDEADBEEF;\n"
      << "    #1 $display(\"RES=%08h\", out);\n"
      << "    $finish;\n"
      << "  end\n"
      << "endmodule\n";
}

void IcarusTool::generateSeqTestbench(
    const fs::path& path,
    const std::string& top,
    const fs::path& vcd_path
) {
    std::ofstream f(path);
    if (!f) {
        throw std::runtime_error("Cannot write testbench: " + path.string());
    }

    f << "`timescale 1ns/1ps\n"
      << "module tb;\n";

    // Declare signals (TODO: make configurable)
    f << "  reg clk, rst;\n"
      << "  wire [7:0] count;\n\n";

    // Instantiate DUT
    f << "  " << top << " dut(\n"
      << "    .clk(clk),\n"
      << "    .rst(rst),\n"
      << "    .count(count)\n"
      << "  );\n\n";

    // Clock generator
    f << "  initial begin\n"
      << "    clk = 0;\n"
      << "    forever #5 clk = ~clk;\n"
      << "  end\n\n";

    // Stimulus
    f << "  initial begin\n"
      << "    rst = 1;\n"
      << "    #10 rst = 0;\n"
      << "    #" << config_.sim_time_ns << " $finish;\n"
      << "  end\n\n";

    // Monitor
    f << "  initial begin\n"
      << "    $monitor(\"Time=%0t clk=%b rst=%b count=%3d\",\n"
      << "             $time, clk, rst, count);\n";

    if (config_.generate_vcd) {
        f << "    $dumpfile(\"" << vcd_path.filename().string() << "\");\n"
          << "    $dumpvars(0, tb);\n";
    }

    f << "  end\n"
      << "endmodule\n";
}

ToolResult IcarusTool::parseCombOutput(const fs::path& log) {
    std::ifstream lf(log);
    std::string line, hex;

    while (std::getline(lf, line)) {
        if (auto p = line.find("RES="); p != std::string::npos) {
            hex = line.substr(p + 4);
            break;
        }
    }

    if (hex.empty()) {
        ToolResult result;
        result.success = false;
        result.log = log;
        result.tool_name = "icarus";
        result.error_msg = "No RES= line found in output";
        return result;
    }

    uint32_t val = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));

    ToolResult result;
    result.success = true;
    result.log = log;
    result.value = val;
    result.tool_name = "icarus";
    return result;
}

ToolResult IcarusTool::parseSeqOutput(const fs::path& log, const fs::path& vcd) {
    // Read entire log file into string
    std::ifstream lf(log);
    std::stringstream buffer;
    buffer << lf.rdbuf();
    std::string log_content = buffer.str();

    // Use existing monitor_parser to parse the output
    auto cycles = verification::parseMonitorOutput(log_content);

    ToolResult result;
    result.success = true;
    result.log = log;
    result.cycles = std::move(cycles);
    result.tool_name = "icarus";

    if (config_.generate_vcd && fs::exists(vcd)) {
        result.vcd = vcd;
    }

    return result;
}

} // namespace tools
