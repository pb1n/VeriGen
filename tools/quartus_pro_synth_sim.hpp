#pragma once
#include "tool.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include "quartus_helper.hpp"

namespace fs = std::filesystem;

#ifndef _WIN32
constexpr const char* vsimPath =
    "/mnt/applications/Siemens/2023-24/RHELx86/QUESTA-CORE-PRIME_2023.4/questasim/linux_x86_64/vsim"; // Missing dependencies on linux
#endif

#ifdef _WIN32
constexpr const char* quartusProRoot = "C:/intelFPGA/18.1/quartus";
#else
constexpr const char* quartusProRoot = "/mnt/applications/altera/18.1/quartus";
#endif

/* ================================================================= *
 *  Low-level Quartus                                                *
 * ================================================================= */
class QuartusProSynthesiser {
    const std::string project = "veri_synth_proj";
    const fs::path    dir;
    const fs::path    tcl;
    bool              chat;
public:
    QuartusProSynthesiser(const fs::path& d, bool talk=false)
        : dir(d), tcl(d/"synth.tcl"), chat(talk)
    { fs::create_directories(dir); }

    bool writeTcl(const fs::path& rtl,const std::string& top) const {
        std::ofstream f(tcl); if(!f) return false;
        f<<"project_new "<<project<<" -overwrite\n"
         <<"set_global_assignment -name FAMILY \"Arria 10\"\n"
         <<"set_global_assignment -name TOP_LEVEL_ENTITY "<<top<<"\n"
         <<"set_global_assignment -name VERILOG_FILE \""<<fs::absolute(rtl).generic_string()<<"\"\n"
         <<"load_package flow\nexecute_module -tool map\nproject_close\n";
        return true;
    }

    bool runQuartus() const
    {
        std::string cd = "cd \"" + dir.string() + "\" && ";
        std::string qsh = std::string(quartusProRoot) +
                        "/bin/quartus_sh -t " + tcl.filename().string();
        return quiet_system(cd + qsh, chat) == 0;
    }

    bool exportVo() const
    {
        std::string cd  = "cd \"" + dir.string() + "\" && ";
    std::string eda = std::string(quartusProRoot) +
                      "/bin/quartus_eda --simulation "
                      "--tool=modelsim --format=verilog " + project;
        return quiet_system(cd + eda, chat) == 0;
    }

    bool writeTB(const std::string& top) const {
        std::ofstream f(dir/"tb.v"); if(!f) return false;
        f<<"`timescale 1ns/1ps\nmodule tb;\n"
           "wire [31:0] out;\n"<<top<<" dut(.out(out));\n"
           "initial begin #1 $display(\"RES=%08h\",out); $finish; end\n"
           "endmodule\n";
        return true;
    }

    bool writeDo() const {
        std::ofstream d(dir/"run.do"); if(!d) return false;
#ifdef _WIN32
        d<<"set QUARTUS \""<<quartusProRoot<<"\"\n";
#else
        d<<"set QUARTUS \""<<quartusProRoot<<"\"\n";
#endif
        d<<"if { ![file exists work] } { vlib work }\n"
         <<"vmap altera work\n"
         <<"vlog -reportprogress 300 \\\n"
         <<"  $QUARTUS/eda/sim_lib/altera_primitives.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/altera_mf.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/220model.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/sgate.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/twentynm_atoms.v \n"
         <<"vlog \"simulation/modelsim/"<<project<<".vo\"\n"
        #ifdef _WIN32
            <<"vlog tb.v\nvism -c -t 1ps work.tb\nrun -all\nquit -f\n";
        #else
            <<"vlog tb.v\n" + std::string(vsimPath) + " -c -t 1ps work.tb\nrun -all\nquit -f\n";
        #endif
        return true;
    }

    uint32_t runModelSim() const {
        std::string cmd =
        #ifdef _WIN32
            "cd /d \"" + dir.string() + "\" && vsim -c -l vsim_log.txt -do \"do run.do\"";
        #else
            "cd \"" + dir.string() + "\" && " + std::string(vsimPath) +
            " -c -l vsim_log.txt -do \"do run.do\"";
        #endif
        if (std::system(cmd.c_str()) != 0) throw std::runtime_error("vsim failed");
        std::ifstream log(dir/"vsim_log.txt"); std::string l,h;
        while (std::getline(log,l)) if (auto p = l.find("RES="); p != std::string::npos) { h = l.substr(p+4); break; }
        if (h.empty()) throw std::runtime_error("RES= not found");
        return static_cast<uint32_t>(std::stoul(h,nullptr,16));
    }
};

/* ================================================================= *
 *  QuartusProTool                                                      *
 * ================================================================= */
class QuartusProTool : public Tool {
    bool verbose;
public:
    explicit QuartusProTool(bool chat=false): verbose(chat){}
    std::string name() const override { return "quartus"; }

    ToolResult run(const fs::path& rtl,const std::string& top,const fs::path& w) override
    {
        QuartusProSynthesiser qs(w,verbose);
        if(!qs.writeTcl(rtl,top)||!qs.runQuartus()||!qs.exportVo()||
           !qs.writeTB(top)||!qs.writeDo())
            return {false,0,w/"quartus.log"};

        try{
            uint32_t v=qs.runModelSim();
            return {true,v,w/"vsim_log.txt"};
        }catch(const std::exception&){
            return {false,0,w/"vsim_log.txt"};
        }
    }
};
