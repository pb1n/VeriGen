#include <iostream>
#include <iomanip>
#include <vector>
#include <indicators/block_progress_bar.hpp>
#include "session.hpp"
#include "synthesise_and_sim.hpp"
#include "ast.hpp"

namespace fs = std::filesystem;
using veri::legacy::VerilogGeneratorFor;


/* ------------ compile‑time chatter switch --------------------------- */
#ifndef DEBUG
    #define DEBUG 1
#endif

/* ---------- tiny CLI helper ---------------------------------------- */
struct Opt { int iter = 1; unsigned seed = std::random_device{}(); };
Opt parse(int argc,char* argv[])
{
    Opt o;
    for(int i=1;i<argc;++i){
        std::string a = argv[i];
        if((a=="--iter"||a=="-n") && i+1<argc) o.iter = std::stoi(argv[++i]);
        else if((a=="--seed"||a=="-s") && i+1<argc) o.seed = std::stoul(argv[++i]);
        else{
            std::cerr<<"Usage: "<<argv[0]<<" [-n N] [-s SEED]\\n";
            std::exit(1);
        }
    }
    if(o.iter<1) o.iter = 1;
    return o;
}
/* ------------------------------------------------------------------- */

int main(int argc,char* argv[])
{
    Opt opt = parse(argc,argv);
    VerilogGeneratorFor gen(opt.seed);

    Session sess("build");                 // one session per program run

    indicators::BlockProgressBar bar{
        indicators::option::BarWidth{40},
        indicators::option::MaxProgress{opt.iter},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::ForegroundColor{indicators::Color::cyan}
    };

    for(int i=0;i<opt.iter;++i)
    {
        fs::path runDir = sess.next();     // build/<stamp>/000xx/
        auto [rtlSrc, golden] = gen.make(i);

        fs::path rtl = runDir / "original.v";
        fs::copy_file(rtlSrc, rtl, fs::copy_options::overwrite_existing);

        QuartusSynthesiser qs(runDir);
        if(!qs.writeTcl(rtl,"top")   || !qs.runQuartus() ||
           !qs.exportVo()            || !qs.writeTB("top") || !qs.writeDo())
        {
            std::cerr<<"\n[FATAL] tool chain failed in iteration "<<i<<"\n";
            return 1;
        }
        uint32_t hw = qs.runModelSim();
        bool pass = (hw==golden);

        std::string postfix = std::string(pass ? "PASSED" : "FAILED") + " " +
                      std::to_string(i + 1) + "/" + std::to_string(opt.iter);
        bar.set_option(indicators::option::PostfixText{postfix});
        bar.tick();

#if DEBUG
        std::cout<<"\n--- iteration "<<i<<" ---------------------------\n"
                 <<"golden    = 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<golden
                 <<"\nsimulated = 0x"<<std::setw(8)<<hw<<std::dec
                 <<(pass? "  [OK]\n":"  [FAIL]\n");
#endif
        if(!pass){
            std::cout<<"\nMismatch – artefacts stored in "<<runDir<<"\n";
            return 2;
        }
    }

    bar.mark_as_completed();
    std::cout<<"\n[100%] all "<<opt.iter<<" iterations passed. Artefacts in "
             <<sess.dir()<< "\n";
    return 0;
}