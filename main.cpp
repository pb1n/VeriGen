#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <indicators/block_progress_bar.hpp>
#include "session.hpp"
#include "ast.hpp"
#include "tools/quartus_synth_sim.hpp"
#include "tools/icarus_sim.hpp"
#include "tools/tool.hpp" 

namespace fs = std::filesystem;
using veri::legacy::VerilogGeneratorFor;

/* ---------- CLI --------------------------------------------------- */
struct Opt { int iter=1; unsigned seed=std::random_device{}(); };
Opt parse(int argc,char* argv[])
{
    Opt o;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if((a=="--iter"||a=="-n")&&i+1<argc) o.iter=std::stoi(argv[++i]);
        else if((a=="--seed"||a=="-s")&&i+1<argc) o.seed=std::stoul(argv[++i]);
        else { std::cerr<<"Usage: "<<argv[0]<<" [-n N] [-s SEED]\n"; std::exit(1); }
    }
    return o;
}

int main(int argc,char* argv[])
{
    Opt opt=parse(argc,argv);
    VerilogGeneratorFor gen(opt.seed);

    // register tools
    std::vector<std::unique_ptr<Tool>> tools;
    tools.emplace_back(std::make_unique<QuartusTool>(false)); // Quartus + ModelSim
    //tools.emplace_back(std::make_unique<IcarusTool>(false)); // Icarus
    Session sess("build");

    indicators::BlockProgressBar bar{
        indicators::option::BarWidth{80},
        indicators::option::MaxProgress{opt.iter},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}
    };

    for(int i=0;i<opt.iter;++i){
        fs::path runDir=sess.next();
        auto [rtlSrc,expected]=gen.make(i);
        fs::path rtl = runDir/"original.v";
        fs::copy_file(rtlSrc,rtl,fs::copy_options::overwrite_existing);

        bool ok=true;
        for(auto& t:tools){
            fs::path tDir=runDir/t->name();
            auto r=t->run(rtl,"top",tDir);
            if(!r.success||r.value!=expected){
                std::cout<<"\n["<<t->name()<<"] failure/mismatch in iter "<<i
                         <<". Logs: "<<r.log<<"\n"<<"got:"<< r.value << " expected: " << expected << "\n";
                return 2;
            }
        }
        bar.tick();
        bar.set_option(indicators::option::PostfixText{
            "iter "+std::to_string(i+1)+"/"+std::to_string(opt.iter)});
    }

    bar.mark_as_completed();
    std::cout<<"\nAll "<<opt.iter<<" iterations passed. Artefacts in "
             <<sess.dir()<<"\n";
    return 0;
}
