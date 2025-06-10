/*───────────────────────────────────────────────────────────────────*
 *  VivadoTool – WebPACK-compatible wrapper (Vivado 2024.2)         *
 *      • uses Artix-7 xc7a35tcpg236-1 (WebPACK)                    *
 *      • generates a tiny project in-memory, then simulates with   *
 *        xsim to print  RES=<hex>                                  *
 *───────────────────────────────────────────────────────────────────*/
#pragma once
#include "tool.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <cstdlib>
#include <filesystem>

class VivadoTool : public Tool
{
    bool chat_;
    static std::string vivadoBin()
    {
        if (const char* e = std::getenv("VIVADO_BIN"))
            return e;
        return "/mnt/applications/Xilinx/24.2/Vivado/2024.2/bin/vivado";
    }

    static constexpr const char* PART = "xc7a35tcpg236-1";   // WebPACK part

public:
    explicit VivadoTool(bool verbose = false) : chat_(verbose) {}

    std::string name() const override { return "vivado"; }

    ToolResult run(const std::filesystem::path& rtl,
                   const std::string&           top,
                   const std::filesystem::path& dir) override
    {
        namespace fs = std::filesystem;
        fs::create_directories(dir);

        /* ───────────────────────── 1. Copy DUT ───────────────────────── */
        fs::path rtlCopy = dir / "dut.v";
        fs::copy_file(rtl, rtlCopy, fs::copy_options::overwrite_existing);

        /* ───────────────────────── 2. Minimal TB ─────────────────────── */
        fs::path tb = dir / "tb.v";
        {
            std::ofstream f(tb);
            f << "module tb;\n"
              << "  wire [31:0] result;\n"
              << "  " << top << " dut(.result(result));\n"
              << "  initial begin\n"
              << "    #1 $display(\"RES=%0x\", result);\n"
              << "    $finish;\n"
              << "  end\n"
              << "endmodule\n";
        }

        /* ───────────────────────── 3. TCL script ─────────────────────── */
        fs::path tcl = dir / "run.tcl";
        {
            std::ofstream t(tcl);
            t << "set_param messaging.defaultLimit 0\n"
              << "create_project -in_memory -part " << PART << "\n"
              << "read_verilog {" << rtlCopy.string() << "}\n"
              << "read_verilog {" << tb.string()      << "}\n"
              << "synth_design -top tb -part " << PART << " -mode out_of_context\n"
              << "launch_simulation -scripts_only\n"
              << "open_sim -name sim_1\n"
              << "run 10 ns\n"
              << "quit\n";
        }

        /* ───────────────────────── 4. Invoke Vivado ──────────────────── */
        fs::path log = dir / "vivado.log";
        std::ostringstream cmd;
        cmd << vivadoBin() << " -mode batch -nolog -nojournal "
            << "-source " << tcl.string()
            << " > " << log.string() << " 2>&1";
        int rc = std::system(cmd.str().c_str());

        /* ───────────────────────── 5. Parse RES=… ────────────────────── */
        std::ifstream L(log);
        std::string   line, resStr;
        std::regex    re(R"(RES=([0-9a-fA-F]+))");
        std::smatch   m;
        while (std::getline(L, line))
            if (std::regex_search(line, m, re)) { resStr = m[1]; break; }

        ToolResult R;
        R.log     = log.string();
        R.success = (rc == 0) && !resStr.empty();
        R.value   = resStr.empty() ? 0 : std::stoul(resStr, nullptr, 16);

        if (chat_)
            std::cout << "[Vivado] "
                      << (R.success ? "SUCCESS" : "FAIL") << " 0x"
                      << std::hex << R.value << std::dec << '\n';

        return R;
    }
};
