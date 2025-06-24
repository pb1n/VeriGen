/*───────────────────────────────────────────────────────────────────*
 *  modelsim_sim.hpp — “pure-simulation” backend for the fuzzer
 *  ---------------------------------------------------------------
 *  - Expects   vsim/vlog   to be in PATH (or edit VSIM_BIN below)
 *  - Reads the RES=xxxxx line printed by the test bench
 *───────────────────────────────────────────────────────────────────*/
#pragma once
#include "tool.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>

namespace fs = std::filesystem;

// point to the executables if they are not in PATH 
#ifndef VSIM_BIN
#   ifdef _WIN32
#       define VSIM_BIN ""                       // vsim.exe in PATH 
#   else
#       define VSIM_BIN ""                       // vsim in PATH
#   endif
#endif

class ModelSimOnlyTool : public Tool
{
    bool chat_;
public:
    explicit ModelSimOnlyTool(bool verbose = false) : chat_(verbose) {}

    std::string name() const override { return "modelsim"; }

private:
    static void write_tb(const fs::path& dir, const std::string& top)
    {
        std::ofstream f(dir / "tb.v");
        if (!f) throw std::runtime_error("cannot write tb.v");
        f << "`timescale 1ns/1ps\n"
             "module tb;\n"
             "  wire [31:0] out;\n"
          << "  " << top << " top(.out(out));\n"
             "  initial begin #1 $display(\"RES=%08h\", out); $finish; end\n"
             "endmodule\n";
    }

    static void write_do(const fs::path& dir,
                         const fs::path& rtl,
                         const std::string& top)
    {
        std::ofstream d(dir / "run.do");
        if (!d) throw std::runtime_error("cannot write run.do");

        d << "if { ![file exists work] } { vlib work }\n"
             "vlog -sv -reportprogress 300 \"" << rtl.generic_string() << "\"\n"
             "vlog -sv tb.v\n"
             "vsim -t 1ps work.tb\n"
             "run -all\n"
             "quit -f\n";
    }

    static uint32_t grab_res(const fs::path& log)
    {
        std::ifstream in(log);
        std::string   line, hex;

        while (std::getline(in, line))
            if (auto p = line.find("RES="); p != std::string::npos) {
                hex = line.substr(p + 4);
                break;
            }
        if (hex.empty()) throw std::runtime_error("RES= not found in log");
        return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
    }

public:
    ToolResult run(const fs::path& rtl,
                const std::string& top,
                const fs::path&    workDir) override
    {
        fs::create_directories(workDir);
        write_tb(workDir, top);
        write_do(workDir, rtl, top);

        // launch ModelSim batch run
    #ifdef _WIN32
        // pushd handles drive changes (e.g.  C: → D:)
        std::string cmd =
            "pushd \"" + workDir.string() + "\" & "
            VSIM_BIN "vsim -c -do \"do run.do\" -l vsim_log.txt" +
            std::string(chat_ ? "" : " > NUL 2>&1") +
            " & popd";
    #else
        std::string cmd =
            "cd \"" + workDir.string() + "\" && "
            VSIM_BIN "vsim -c -do \"do run.do\" -l vsim_log.txt" +
            std::string(chat_ ? "" : " > /dev/null 2>&1");
    #endif

        int rc = std::system(cmd.c_str());
        if (rc != 0)
            return {false, 0, workDir / "vsim_log.txt"};

        // parse RES= line
        try {
            uint32_t v = grab_res(workDir / "vsim_log.txt");
            return {true, v, workDir / "vsim_log.txt"};
        } catch (...) {
            // log didn’t contain RES= or could not be read
            return {false, 0, workDir / "vsim_log.txt"};
        }
    }
};
