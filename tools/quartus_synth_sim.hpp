#pragma once
#include "tool.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include "quartus_helper.hpp"

namespace fs = std::filesystem;

#ifdef _WIN32
constexpr const char* quartusRoot = "C:/intelFPGA/18.1/quartus";
#else
constexpr const char* quartusRoot = "/mnt/applications/altera/18.1/quartus";
#endif

/* ================================================================= *
 *  Low-level Quartus                                                *
 * ================================================================= */
class QuartusSynthesiser {
    const std::string project = "veri_synth_proj";
    const fs::path    dir;
    const fs::path    tcl;
    bool              chat;
public:
    QuartusSynthesiser(const fs::path& d, bool talk=false)
        : dir(d), tcl(d/"synth.tcl"), chat(talk)
    { fs::create_directories(dir); }

    bool writeTcl(const fs::path& rtl,const std::string& top) const {
        std::ofstream f(tcl); if(!f) return false;
        f<<"project_new "<<project<<" -overwrite\n"
         <<"set_global_assignment -name FAMILY \"Cyclone V\"\n"
         <<"set_global_assignment -name TOP_LEVEL_ENTITY "<<top<<"\n"
         <<"set_global_assignment -name SYSTEMVERILOG_FILE \""<<fs::absolute(rtl).generic_string()<<"\"\n"
         <<"load_package flow\nexecute_module -tool map\nproject_close\n";
        return true;
    }

    bool runQuartus() const {
        std::string cd = std::string("cd ") + (dir.string()) + " && ";
        std::string qsh = "quartus_sh -t " + tcl.filename().string();
        std::string fit = "quartus_fit " + project;
        return quiet_system(cd + qsh, chat) == 0 && quiet_system(cd + fit, chat) == 0;
    }

    bool exportVo() const {
        std::string cd = std::string("cd ") + (dir.string()) + " && ";
        std::string eda = "quartus_eda --simulation=on --tool=modelsim --format=verilog " + project;
        return quiet_system(cd + eda, chat) == 0;
    }

    bool writeTB(const std::string& top) const {
        std::ofstream f(dir/"tb.v"); if(!f) return false;
        f<<"`timescale 1ns/1ps\nmodule tb;\n"
           "wire [31:0] res;\n"<<top<<" dut(.result(res));\n"
           "initial begin #1 $display(\"RES=%08h\",res); $finish; end\n"
           "endmodule\n";
        return true;
    }

    bool writeDo() const {
        std::ofstream d(dir/"run.do"); if(!d) return false;
#ifdef _WIN32
        d<<"set QUARTUS \""<<quartusRoot<<"\"\n";
#else
        d<<"set QUARTUS \""<<quartusRoot<<"\"\n";
#endif
        d<<"if { ![file exists work] } { vlib work }\n"
         <<"vmap altera work\n"
         <<"vlog -reportprogress 300 \\\n"
         <<"  $QUARTUS/eda/sim_lib/altera_primitives.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/altera_mf.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/220model.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/sgate.v \\\n"
         <<"  $QUARTUS/eda/sim_lib/cyclonev_atoms.v\n"
         <<"  pwd\n"
         <<"vlog \"simulation/modelsim/"<<project<<".vo\"\n"
         <<"vlog tb.v\nvsim -t 1ps work.tb\nrun -all\nquit -f\n";
        return true;
    }

    uint32_t runModelSim() const {
#ifdef _WIN32

        std::string cmd = "cd /d \"" + dir.string() + "\" && vsim -c -l vsim_log.txt -do \"do run.do\" > NUL 2>&1";
#else
        std::string cmd = "cd \"" + dir.string() + "\" && vsim -c -l vsim_log.txt -do \"do run.do\"";
#endif
        if (std::system(cmd.c_str()) != 0) throw std::runtime_error("vsim failed");
        std::ifstream log(dir/"vsim_log.txt"); std::string l,h;
        while (std::getline(log,l)) if (auto p = l.find("RES="); p != std::string::npos) { h = l.substr(p+4); break; }
        if (h.empty()) throw std::runtime_error("RES= not found");
        return static_cast<uint32_t>(std::stoul(h,nullptr,16));
    }
};

/* ================================================================= *
 *  QuartusTool                                                      *
 * ================================================================= */
class QuartusTool : public Tool {
    bool verbose;
public:
    explicit QuartusTool(bool chat=false): verbose(chat){}
    std::string name() const override { return "quartus"; }

    ToolResult run(const fs::path& rtl,const std::string& top,const fs::path& w) override
    {
        QuartusSynthesiser qs(w,verbose);
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
