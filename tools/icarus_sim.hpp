#pragma once
#include "tool.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

inline int ivl_system(std::string cmd, bool verbose = false)
{
#ifdef _WIN32
    if (!verbose) cmd += " >nul 2>&1";
#else
    if (!verbose) cmd += " >/dev/null 2>&1";
#endif
    return std::system(cmd.c_str());
}

class IcarusTool : public Tool {
    bool verbose;
public:
    explicit IcarusTool(bool chat = false) : verbose(chat) {}
    std::string name() const override { return "icarus"; }

    ToolResult run(const fs::path& rtl,
               const std::string& top,
               const fs::path& workdir) override
    {
        fs::create_directories(workdir);

        fs::path tb = workdir / "tb.v";
        fs::path ivl_log = workdir / "iverilog.log";
        fs::path vvp_out = workdir / "vvp_out.txt";

        // Write testbench
        {
            std::ofstream f(tb);
            if (!f) return {false, 0, tb};
            f << "`timescale 1ns/1ps\nmodule tb;\n"
                "wire [31:0] res;\n"
            << top << " dut(.result(res));\n"
                "initial begin #1 $display(\"RES=%08h\", res); $finish; end\n"
                "endmodule\n";
        }

        fs::path vvp_bin = workdir / "sim.vvp";
        std::string compile_cmd =
            "iverilog -g2012 -o \"" + vvp_bin.string() + "\" -s tb \"" +
            rtl.string() + "\" \"" + tb.string() + "\" > \"" + ivl_log.string() + "\" 2>&1";

        if (std::system(compile_cmd.c_str()) != 0)
            return {false, 0, ivl_log}; // compilation failed

        std::string sim_cmd =
            "vvp \"" + vvp_bin.string() + "\" > \"" + vvp_out.string() + "\" 2>&1";

        if (std::system(sim_cmd.c_str()) != 0)
            return {false, 0, vvp_out}; // simulation failed

        std::ifstream lf(vvp_out);
        std::string line, hex;
        while (std::getline(lf, line))
            if (auto p = line.find("RES="); p != std::string::npos) {
                hex = line.substr(p + 4);
                break;
            }

        if (hex.empty())
            return {false, 0, vvp_out}; // no RES= line found

        uint32_t val = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
        return {true, val, vvp_out};
    }
};
