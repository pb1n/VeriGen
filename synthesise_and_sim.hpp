#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

/* -----------------------------------------------------------
 *  Toggle chatter here.
 * ---------------------------------------------------------*/
#ifndef DEBUG
    #define DEBUG 0 
#endif
/* ---------------------------------------------------------*/

/* helper – run a shell command, optionally silenced */
inline int quiet_system(std::string cmd)
{
#if DEBUG
    return std::system(cmd.c_str());
#else
#   ifdef _WIN32
        cmd += " >nul 2>&1";
#   else
        cmd += " >/dev/null 2>&1";
#   endif
    return std::system(cmd.c_str());
#endif
}

/* ================================================================= */
class QuartusSynthesiser {
    const std::string projectName = "veri_synth_proj";
    const fs::path    workDir;          /*  build/        */
    const fs::path    scriptPath;       /*  build/synth.tcl */
public:
    explicit QuartusSynthesiser(const fs::path& build)
        : workDir(build), scriptPath(build / "synth.tcl")
    { fs::create_directories(workDir); }

    /* ---------------- TCL script ---------------------------------- */
    bool writeTcl(const fs::path& verilog, const std::string& top) const {
        std::ofstream tcl(scriptPath);
        if(!tcl) return false;
        std::string v = fs::absolute(verilog).generic_string();
        tcl << "project_new " << projectName << " -overwrite\n"
            << "set_global_assignment -name FAMILY \"Cyclone V\"\n"
            << "set_global_assignment -name TOP_LEVEL_ENTITY " << top << "\n"
            << "set_global_assignment -name VERILOG_FILE \"" << v << "\"\n"
            << "load_package flow\nexecute_module -tool map\nproject_close\n";
        return true;
    }

    /* ---------------- Quartus compile ----------------------------- */
    bool runQuartus() const {
        const std::string cd = "cd /d \""+workDir.string()+"\" && ";
        return quiet_system(cd + "quartus_sh -t " + scriptPath.filename().string()) == 0 &&
               quiet_system(cd + "quartus_fit "    + projectName)                    == 0;
    }

    /* ---------------- gate‑level net‑list (.vo) ------------------- */
    bool exportVo() const {
        const std::string cd = "cd /d \""+workDir.string()+"\" && ";
        std::string cmd = "quartus_eda --simulation=on --tool=modelsim "
                          "--format=verilog " + projectName;
        return quiet_system(cd+cmd) == 0;
    }

    fs::path voPath() const { return workDir/"simulation"/"modelsim"/(projectName+".vo"); }

    /* --------------- test‑bench ----------------------------------- */
    bool writeTB(const std::string& top) const {
        std::ofstream tb(workDir/"tb.v");
        if(!tb) return false;
        tb << "`timescale 1ns/1ps\nmodule tb;\n"
              "wire [31:0] res;\n"<<top<<" dut(.result(res));\n"
              "initial begin #1 $display(\"RES=%08h\",res); $finish; end\n"
              "endmodule\n";
#if DEBUG
        std::cout<<"TB Written\n";
#endif
        return true;
    }

    /* --------------- ModelSim ‘run.do’ ---------------------------- */
    bool writeDo() const {
        std::ofstream d(workDir/"run.do");
        if(!d) return false;
        d <<
        "set QUARTUS \"C:/intelFPGA/18.1/quartus\"\n"
        "if { ![file exists work] } { vlib work }\n"
        "vmap altera work\n"
        "\n# vendor libraries\n"
        "vlog -reportprogress 300 \\\n"
        "  $QUARTUS/eda/sim_lib/altera_primitives.v \\\n"
        "  $QUARTUS/eda/sim_lib/altera_mf.v \\\n"
        "  $QUARTUS/eda/sim_lib/220model.v \\\n"
        "  $QUARTUS/eda/sim_lib/sgate.v \\\n"
        "  $QUARTUS/eda/sim_lib/cyclonev_atoms.v\n\n"
        "# design + TB\n"
        "vlog \"simulation/modelsim/"<<projectName<<".vo\"\n"
        "vlog tb.v\n\n"
        "# run\n"
        "vsim -t 1ps work.tb\nrun -all\nquit -f\n";
        return true;
    }

    /* --------------- run ModelSim & read RES= line --------------- */
    uint32_t runModelSim() const
    {
        /* -l <file>  writes the full transcript to vsim_log.txt
        (output still appears on‑screen).                           */
        const std::string cmd =
            "cd /d \"" + workDir.string() + "\" && "
            "vsim -c -l vsim_log.txt -do \"do run.do\"";

        /* run it ­– let stdout/stderr flow freely */
        if (std::system(cmd.c_str()) != 0)
            throw std::runtime_error("vsim failed");

        /* parse vsim_log.txt for the RES= line ---------------------- */
        std::ifstream log(workDir / "vsim_log.txt");
        std::string line, hex;
        while (std::getline(log, line))
            if (auto pos = line.find("RES="); pos != std::string::npos)
            {   hex = line.substr(pos + 4);  break; }

        if (hex.empty())
            throw std::runtime_error("RES line not found in ModelSim output");

        return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
    }
};
