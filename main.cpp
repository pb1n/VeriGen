/**********************************************************************
 * main.cpp – top-level fuzzer driver
 *********************************************************************/
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

#include <indicators/block_progress_bar.hpp>

#include "session.hpp"
#include "ast.hpp"
#include "hierarchy_generator.hpp"
#include "tools/quartus_pro_synth_sim.hpp"
#include "tools/quartus_synth_sim.hpp"
#include "tools/icarus_sim.hpp"
#include "tools/vivado_synth_sim.hpp"
#include "tools/tool.hpp"
#include "tools/modelsim_sim.hpp"
#include "tools/compare_sim.hpp"
#include "util/make_numbered.hpp"
#include <future>

namespace fs = std::filesystem;

/*────────────────────── CLI options ─────────────────────────────*/
struct Opt {
    int      iter      = 1;
    unsigned seed      = std::random_device{}();
    int      tool      = 4;      /* default: Icarus */
    bool     chat      = false;

    bool hier          = false;  /* --hier => hierarchy generator */

    /* loop-gen knobs */
    int  min_start     = 0,  max_start = 0;
    int  min_iter      = 2,  max_iter  = 16;
    bool random_update = true;

    /* hier-gen knobs */
    int  depth         = 2;
    int  min_child     = 2,  max_child = 4;
    bool root_pref     = false;
    bool rel_up        = false;
    bool alias         = false;
    bool defparam      = false;
    bool dump_gen      = false;
    double bigGenProb  = 0.5;   /* chance that a leaf is a “Generate” module */

    /* emit-only mode */
    bool        emit_only = false;
    std::string emit_file;
};

/* helper */
static bool next_arg(int& i,int argc,char* argv[]){ return ++i < argc; }

/*────────────────────── option parser ───────────────────────────*/
static Opt parse(int argc,char* argv[])
{
    Opt o;
    for (int i=1;i<argc;++i) {
        std::string a = argv[i];
        /* generic */
        if ((a=="--iter"||a=="-n")   && next_arg(i,argc,argv)) o.iter = std::stoi(argv[i]);
        else if ((a=="--seed"||a=="-s") && next_arg(i,argc,argv)) o.seed = std::stoul(argv[i]);
        else if ((a=="--tool"||a=="-t") && next_arg(i,argc,argv)) o.tool = std::stoi(argv[i]);
        else if (a=="--chat"||a=="-c") o.chat = true;
        /* generator kind */
        else if (a=="--hier") o.hier = true;
        /* loop knobs */
        else if (a=="--min-start" && next_arg(i,argc,argv)) o.min_start = std::stoi(argv[i]);
        else if (a=="--max-start" && next_arg(i,argc,argv)) o.max_start = std::stoi(argv[i]);
        else if (a=="--min-iter"  && next_arg(i,argc,argv)) o.min_iter  = std::stoi(argv[i]);
        else if (a=="--max-iter"  && next_arg(i,argc,argv)) o.max_iter  = std::stoi(argv[i]);
        /* hier knobs */
        else if (a=="--root-prefix")  o.root_pref = true;
        else if (a=="--relative-up")  o.rel_up    = true;
        else if (a=="--alias")        o.alias     = true;
        else if (a=="--defparam")     o.defparam  = true;
        else if (a=="--depth"     && next_arg(i,argc,argv)) o.depth     = std::stoi(argv[i]);
        else if (a=="--min-child" && next_arg(i,argc,argv)) o.min_child = std::stoi(argv[i]);
        else if (a=="--max-child" && next_arg(i,argc,argv)) o.max_child = std::stoi(argv[i]);
        else if (a=="--gen-prob" && next_arg(i,argc,argv)) o.bigGenProb = std::stod(argv[i]);
        else if (a=="--include-gen")  o.dump_gen  = true;
        /* emit-only */
        else if (a=="--emit-file" && next_arg(i,argc,argv)) { o.emit_only = true; o.emit_file = argv[i]; }
        else { std::cerr<<"unknown option '"<<a<<"'\n"; std::exit(1); }
    }
    if (o.min_child > o.max_child) std::swap(o.min_child,o.max_child);
    return o;
}

/*──────────────────────── main ────────────────────────────────*/
int main(int argc,char* argv[])
{
    Opt opt = parse(argc,argv);

    /* choose generator */
    std::unique_ptr<veri::Generator>     loopGen;
    std::unique_ptr<veri::HierarchyGen>  hierGen;

    if (opt.hier) {
        veri::HierCfg cfg;
        cfg.depth        = opt.depth;
        cfg.minChild     = opt.min_child;
        cfg.maxChild     = opt.max_child;
        cfg.rootPrefix   = opt.root_pref;
        cfg.relativeUp   = opt.rel_up;
        cfg.aliasStmt    = opt.alias;
        cfg.defparam     = opt.defparam;
        cfg.enableBigGen = opt.dump_gen;
        cfg.bigGenProb = opt.bigGenProb;
        hierGen = std::make_unique<veri::HierarchyGen>(opt.seed, cfg);
    } else {
        loopGen = std::make_unique<veri::Generator>(
                    opt.seed,
                    opt.min_start, opt.max_start,
                    opt.min_iter , opt.max_iter,
                    opt.random_update);
    }

    /* choose tool */
    std::vector<std::unique_ptr<Tool>> tools;
    switch (opt.tool) {
        case 1: tools.emplace_back(std::make_unique<QuartusTool>(opt.chat));             break;
        case 2: tools.emplace_back(std::make_unique<QuartusProTool>(opt.chat));          break;
        case 3: tools.emplace_back(std::make_unique<VivadoTool>(opt.chat));              break;
        case 4: tools.emplace_back(std::make_unique<IcarusTool>(opt.chat));              break;
        case 5: tools.emplace_back(std::make_unique<ModelSimOnlyTool>(opt.chat));        break;
        case 6: tools.emplace_back(std::make_unique<tools::CompareSimTool>(opt.chat));   break;
    }

    /* counters for final summary + progress-bar display */
    std::size_t crash_cnt    = 0;
    std::size_t mismatch_cnt = 0;
    std::size_t timeout_cnt  = 0;
    constexpr auto TIME_LIMIT = std::chrono::minutes{10};

    Session sess("build");  
    std::string prefix = opt.dump_gen ? "Gen+Hier-fuzz" : "Hier-fuzz";
    indicators::BlockProgressBar bar{
        indicators::option::MaxProgress{static_cast<size_t>(opt.iter)},
        indicators::option::BarWidth{25},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowPercentage{false},
        indicators::option::ShowRemainingTime{true},
        indicators::option::PrefixText{ opt.hier ? prefix : "Gen-fuzz"}
    };

    const fs::path root_cwd = fs::current_path();

    for (int i = 0; i < opt.iter; ++i)
    {
        fs::path runDir = sess.next();
        fs::path oldCwd = fs::current_path();
        fs::current_path(runDir);

        /* ─── generate RTL ────────────────────────────────────── */
        fs::path  rtlPath;
        uint32_t  golden = 0;

        if (opt.hier) {
            auto [p, val] = hierGen->write("top.v");
            rtlPath = p; golden = val;
        } else {
            auto [fn,val] = loopGen->make("top", i, opt.depth);
            rtlPath = fs::absolute(fn); golden = val;
        }

        /* emit-only mode */
        if (opt.emit_only) {
            fs::current_path(oldCwd);
            fs::path outName = (opt.iter==1)
                ? fs::path{opt.emit_file}
                : util::make_numbered(opt.emit_file, i, 2);

            if (opt.hier) hierGen->write(outName.string());
            else          loopGen->make(outName.string(), i, opt.depth);

            std::cout << "Wrote " << outName << '\n';
            bar.tick();
            bar.set_option(indicators::option::PostfixText{
                "iter " + std::to_string(i+1) + "/" + std::to_string(opt.iter) +
                " | crashes 0 | mism 0"});
            continue;
        }

        fs::current_path(oldCwd);

        /* ─── run tools ───────────────────────────────────────── */
        for (auto& t : tools)
        {
            fs::path tDir = runDir / t->name();

            /* run with 15-minute watchdog */
            auto fut = std::async(std::launch::async,
                                  [&]{ return t->run(rtlPath, "top", tDir); });

            if (fut.wait_for(TIME_LIMIT) == std::future_status::timeout) {
                ++timeout_cnt;
                std::cerr << "\n["<<t->name()<<"] Time-out (>10 min) in iteration "
                          << i << '\n';
            }

            ToolResult res = fut.get();

            if (!res.success) {
                ++crash_cnt;
                std::cerr << "\n["<<t->name()<<"] Tool failure in iteration "
                          << i << "\n  Logs: " << res.log << '\n';
            }
            if (t->name() != "CompareSim" && res.value != golden) {
                ++mismatch_cnt;
                std::cerr << "\n["<<t->name()<<"] Mismatch in iteration "
                          << i << "\n  got 0x"<<std::hex<<res.value
                          << " expected 0x"<<golden<<std::dec
                          << "\nLogs: "<<res.log<<'\n';
            }
        }

        /* ─── progress-bar update ────────────────────────────── */
        bar.tick();
        bar.set_option(indicators::option::PostfixText{
           "iter "   + std::to_string(i+1) + "/" + std::to_string(opt.iter) +
           " | crash "   + std::to_string(crash_cnt) +
           " | mism "    + std::to_string(mismatch_cnt) +
           " | tOut "    + std::to_string(timeout_cnt)});
    }

    bar.mark_as_completed();
    std::cout <<
R"( __      __       _  _____            
 \ \    / /      (_)/ ____|           
  \ \  / /__ _ __ _| |  __  ___ _ __  
   \ \/ / _ \ '__| | | |_ |/ _ \ '_ \ 
    \  /  __/ |  | | |__| |  __/ | | |
     \/ \___|_|  |_|\_____|\___|_| |_| )" << "\n\n";
    std::cout << "\n=============== Summary ===============\n";
    std::cout << "      Iterations : " << opt.iter     << '\n';
    std::cout << "      Crashes    : " << crash_cnt    << '\n';
    std::cout << "      Mismatches : " << mismatch_cnt << '\n';
    std::cout << "      Time-outs  : " << timeout_cnt  << '\n';
    std::cout << "      Seed       : " << opt.seed  << '\n';
    std::cout << "Artefacts in " << sess.dir() << '\n';

    if (crash_cnt==0 && mismatch_cnt==0 && timeout_cnt==0) return 0;
    if (crash_cnt)    return 3;   // crash
    if (timeout_cnt)  return 2;   // timeout
    return 1; // only mismatches
}
