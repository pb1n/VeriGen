#pragma once
#include <fstream>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <direct.h>  // For _mkdir on Windows

class IcarusSimulator {
    bool verbose;
    std::string workdir;

    void createDirectory(const std::string& path) {
#ifdef _WIN32
        _mkdir(path.c_str());
#else
        mkdir(path.c_str(), 0755);
#endif
    }

public:
    explicit IcarusSimulator(const std::string& work_directory = "sim_work", bool verbose_mode = false)
        : verbose(verbose_mode), workdir(work_directory) {
        createDirectory(workdir);
    }

    struct SimResult {
        bool success;
        std::string output;
        std::string log_file;
    };

    // Generate a testbench for a counter module
    std::string generateCounterTestbench(
        const std::string& module_name,
        int width,
        bool has_reset,
        bool has_load,
        int cycles = 10
    ) {
        std::ostringstream tb;
        tb << "`timescale 1ns/1ps\n";
        tb << "module tb;\n";
        tb << "  reg clk;\n";
        if (has_reset) tb << "  reg rst;\n";
        if (has_load) {
            tb << "  reg load_enable;\n";
            tb << "  reg [" << (width - 1) << ":0] data_in;\n";
        }
        tb << "  wire [" << (width - 1) << ":0] count;\n\n";

        // Instantiate DUT
        tb << "  " << module_name << " dut(\n";
        tb << "    .clk(clk)";
        if (has_reset) tb << ",\n    .rst(rst)";
        if (has_load) {
            tb << ",\n    .load_enable(load_enable)";
            tb << ",\n    .data_in(data_in)";
        }
        tb << ",\n    .count(count)\n";
        tb << "  );\n\n";

        // Clock generation
        tb << "  initial clk = 0;\n";
        tb << "  always #5 clk = ~clk;\n\n";

        // Test stimulus
        tb << "  initial begin\n";
        tb << "    $dumpfile(\"" << module_name << ".vcd\");\n";
        tb << "    $dumpvars(0, tb);\n\n";

        if (has_reset) {
            tb << "    rst = 1;\n";
            if (has_load) {
                tb << "    load_enable = 0;\n";
                tb << "    data_in = 0;\n";
            }
            tb << "    #20 rst = 0;\n\n";
        } else if (has_load) {
            tb << "    load_enable = 0;\n";
            tb << "    data_in = 0;\n";
            tb << "    #10;\n\n";
        }

        // Test load if available
        if (has_load) {
            tb << "    // Test load\n";
            tb << "    #10 load_enable = 1; data_in = " << width << "'d50;\n";
            tb << "    #10 load_enable = 0;\n\n";
        }

        tb << "    // Run for " << cycles << " cycles\n";
        tb << "    #" << (cycles * 10) << ";\n\n";

        tb << "    $display(\"Final count value: %d\", count);\n";
        tb << "    $finish;\n";
        tb << "  end\n\n";

        // Monitor
        tb << "  initial begin\n";
        tb << "    $monitor(\"Time=%0t clk=%b";
        if (has_reset) tb << " rst=%b";
        if (has_load) tb << " load_enable=%b data_in=%d";
        tb << " count=%d\", $time, clk";
        if (has_reset) tb << ", rst";
        if (has_load) tb << ", load_enable, data_in";
        tb << ", count);\n";
        tb << "  end\n";
        tb << "endmodule\n";

        return tb.str();
    }

    // Simulate a Verilog file with a given testbench
    SimResult simulate(const std::string& rtl_file, const std::string& testbench_content) {
        std::string tb_file = workdir + "/tb.v";
        std::string vvp_file = workdir + "/sim.vvp";
        std::string compile_log = workdir + "/iverilog.log";
        std::string sim_log = workdir + "/vvp.log";

        // Write testbench
        {
            std::ofstream tb(tb_file);
            if (!tb) {
                return {false, "Failed to write testbench", tb_file};
            }
            tb << testbench_content;
        }

        // Compile with iverilog
        std::string compile_cmd = "iverilog -g2012 -o \"" + vvp_file +
                                  "\" \"" + rtl_file +
                                  "\" \"" + tb_file +
                                  "\" > \"" + compile_log + "\" 2>&1";

        if (verbose) std::cout << "Compiling: " << compile_cmd << "\n";

        if (std::system(compile_cmd.c_str()) != 0) {
            return {false, "Compilation failed", compile_log};
        }

        // Run simulation
        std::string sim_cmd = "vvp \"" + vvp_file +
                             "\" > \"" + sim_log + "\" 2>&1";

        if (verbose) std::cout << "Simulating: " << sim_cmd << "\n";

        if (std::system(sim_cmd.c_str()) != 0) {
            return {false, "Simulation failed", sim_log};
        }

        // Read output
        std::ifstream log(sim_log);
        std::ostringstream output;
        output << log.rdbuf();

        return {true, output.str(), sim_log};
    }

    // Convenience method for counter simulation
    SimResult simulateCounter(
        const std::string& rtl_file,
        const std::string& module_name,
        int width,
        bool has_reset,
        bool has_load,
        int cycles = 10
    ) {
        std::string tb = generateCounterTestbench(module_name, width, has_reset, has_load, cycles);
        return simulate(rtl_file, tb);
    }
};