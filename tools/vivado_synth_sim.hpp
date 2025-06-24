/*───────────────────────────────────────────────────────────────────*
 *  VivadoTool – (Vivado 2024.2)                                     *
 *      • uses xc7k70t                                               *
 *───────────────────────────────────────────────────────────────────*/
#pragma once
#include "tool.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

class VivadoTool : public Tool
{
    static constexpr const char* PART = "xc7k70t";  
    bool chat_;

    /* ---------- helpers -------------------------------------------------- */
    static std::string vivadoBin()
    {
        if (const char* e = std::getenv("VIVADO_BIN"))
            return e;
        return "/mnt/applications/Xilinx/24.2/Vivado/2024.2/bin/vivado";
    }
    static std::string xvlogBin () { return "xvlog"; }
    static std::string xelabBin () { return "xelab"; }
    static std::string xsimBin  () { return "xsim";  }

public:
    explicit VivadoTool(bool verbose = false) : chat_(verbose) {}
    std::string name() const override { return "vivado"; }

    /* ==================================================================== */
    ToolResult run(const std::filesystem::path& rtl,
                   const std::string&           top,
                   const std::filesystem::path& dir) override
    {
        namespace fs = std::filesystem;
        fs::create_directories(dir);

        /* ── 1. copy DUT ───────────────────────────────────────────────── */
        const fs::path rtlCopy = dir / "dut.v";
        fs::copy_file(rtl, rtlCopy, fs::copy_options::overwrite_existing);

        /* ── 2. emit minimal TB ───────────────────────────────────────── */
        const fs::path tb = dir / "tb.v";
        {
            std::ofstream f(tb);
            f << "module tb;\n"
              << "  wire [31:0] out;\n"
              << "  " << top << " dut(.out(out));\n"
              << "  initial begin\n"
              << "    #1 $display(\"RES=%0x\", out);\n"
              << "    $finish;\n"
              << "  end\n"
              << "endmodule\n";
        }

        /* ── 3. TCL for *synthesis only* (no simulation) ──────────────── */
        const fs::path tcl = dir / "run.tcl";
        {
            std::ofstream t(tcl);
            t << "set_param messaging.defaultLimit 0\n"
              << "create_project -in_memory -part " << PART << "\n"
              << "read_verilog {" << rtlCopy.string() << "}\n"
              << "read_verilog {" << tb.string()      << "}\n"
              << "synth_design -mode out_of_context "
              << " -top tb -part " << PART << "\n"
              << "write_checkpoint " << (dir / "post_synth.dcp").string() << "\n"
              << "quit\n";
        }

        /* ── 4. run Vivado in batch mode (synthesis) ──────────────────── */
        const fs::path vivadoLog = dir / "vivado.log";
        {
            std::ostringstream cmd;
            cmd << vivadoBin() << " -mode batch -nolog -nojournal "
                << "-source " << tcl.string()
                << " > " << vivadoLog.string() << " 2>&1";
            if (std::system(cmd.str().c_str()) != 0)
                if (chat_) std::cerr << "[Vivado] synthesis exited with errors\n";
        }

        /* ── 5. run xsim (vlog → elab → sim) ─────────────────────────── */
        const fs::path simLog = dir / "xsim.log";

        std::ostringstream simCmd;
        simCmd
            << "cd " << dir.string() << " && "
            << xvlogBin() << " dut.v tb.v && "
            << xelabBin() << " tb -s tb_sim && "
            << xsimBin()  << " tb_sim -runall > xsim.log";

        int simRC = std::system(simCmd.str().c_str());

        /* ── 6. scan xsim output for  RES=<hex> ───────────────────────── */
        std::ifstream L(simLog);
        std::string line, resStr;
        std::regex  re(R"(RES=([0-9a-fA-F]+))");
        std::smatch m;
        while (std::getline(L, line))
            if (std::regex_search(line, m, re)) { resStr = m[1]; break; }

        /* ── 7. pack result object ────────────────────────────────────── */
        ToolResult R;
        R.log     = simLog.string();
        R.success = (simRC == 0) && !resStr.empty();
        R.value   = resStr.empty() ? 0 : std::stoul(resStr, nullptr, 16);

        if (chat_)
            std::cout << "[Vivado] "
                      << (R.success ? "SUCCESS" : "FAIL")
                      << " 0x" << std::hex << R.value << std::dec << '\n';

        return R;
    }
};
