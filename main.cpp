/**********************************************************************
 * main.cpp – top-level fuzzer driver
 * -----------------------------------
 *  - “loop” generator  -> veri::Generator          (old)
 *  - “hier” generator  -> veri::HierarchyGen       (new)
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


namespace fs = std::filesystem;

/*───────────────────────────────────────────────────────────────*/
/* CLI options                                                  */
/*───────────────────────────────────────────────────────────────*/
struct Opt {
    // generic
    int      iter  = 1;
    unsigned seed  = std::random_device{}();
    int      tool  = 4;   /* default: Icarus */
    bool     chat  = false;

    // generator selection
    bool hier = false;    /* --hier => hierarchy generator */

    // loop-generator knobs
    int  min_start     = 0;
    int  max_start     = 0;
    int  min_iter      = 2;
    int  max_iter      = 16;
    bool random_update = true;

    // hierarchy-generator knobs
    int  depth      = 2;
    int  min_child  = 2;
    int  max_child  = 4;
    bool root_pref  = false; // does NOT work with Icarus, works with ModelSim
    bool rel_up     = false; // not yet functional
    bool alias      = false; // not yet functional
    bool defparam   = false;
    bool dump_gen = false;
};

// simple argv helper
static bool next_arg(int& i,int argc,char* argv[])
{ return ++i < argc; }

// option parser
static Opt parse(int argc,char* argv[])
{
    Opt o;
    for (int i=1;i<argc;++i) {
        std::string a = argv[i];

        // generic
        if ((a=="--iter"||a=="-n")   && next_arg(i,argc,argv)) o.iter = std::stoi(argv[i]);
        else if ((a=="--seed"||a=="-s") && next_arg(i,argc,argv)) o.seed = std::stoul(argv[i]);
        else if ((a=="--tool"||a=="-t") && next_arg(i,argc,argv)) o.tool = std::stoi(argv[i]);
        else if (a=="--chat"||a=="-c") o.chat = true;

        // pick generator
        else if (a=="--hier") o.hier = true;

        // loop generator knobs
        else if (a=="--min-start" && next_arg(i,argc,argv)) o.min_start = std::stoi(argv[i]);
        else if (a=="--max-start" && next_arg(i,argc,argv)) o.max_start = std::stoi(argv[i]);
        else if (a=="--min-iter"  && next_arg(i,argc,argv)) o.min_iter  = std::stoi(argv[i]);
        else if (a=="--max-iter"  && next_arg(i,argc,argv)) o.max_iter  = std::stoi(argv[i]);

        // hierarchy generator knobs
        else if (a=="--root-prefix")  o.root_pref = true;
        else if (a=="--relative-up")  o.rel_up    = true;
        else if (a=="--alias")        o.alias     = true;
        else if (a=="--defparam")     o.defparam  = true;
        else if (a=="--depth"     && next_arg(i,argc,argv)) o.depth     = std::stoi(argv[i]);
        else if (a=="--min-child" && next_arg(i,argc,argv)) o.min_child = std::stoi(argv[i]);
        else if (a=="--max-child" && next_arg(i,argc,argv)) o.max_child = std::stoi(argv[i]);
        else if (a=="--include-gen") o.dump_gen = true;

        else {
            std::cerr << "unknown option '" << a << "' – see source for help\n";
            std::exit(1);
        }
    }
    if (o.tool < 1 || o.tool > 6) {
        std::cerr << "--tool must be 1…6\n"; std::exit(1);
    }
    if (o.min_child > o.max_child) std::swap(o.min_child,o.max_child);
    return o;
}

/*───────────────────────────────────────────────────────────────*/
int main(int argc,char* argv[])
{
    Opt opt = parse(argc,argv);

    // choose the generator
    std::unique_ptr<veri::Generator>     loopGen;
    std::unique_ptr<veri::HierarchyGen>  hierGen;

    if (opt.hier) {
        veri::HierCfg cfg;
        cfg.depth       = opt.depth;
        cfg.minChild    = opt.min_child;
        cfg.maxChild    = opt.max_child;
        cfg.rootPrefix  = opt.root_pref;
        cfg.relativeUp  = opt.rel_up;
        cfg.aliasStmt   = opt.alias;
        cfg.defparam    = opt.defparam;
        cfg.enableBigGen = opt.dump_gen;
        hierGen = std::make_unique<veri::HierarchyGen>(opt.seed, cfg);
    } else {
        loopGen = std::make_unique<veri::Generator>(
                    opt.seed,
                    opt.min_start, opt.max_start,
                    opt.min_iter , opt.max_iter,
                    opt.random_update);
    }

    // pick tool chain
    std::vector<std::unique_ptr<Tool>> tools;
    switch (opt.tool) {
        case 1: tools.emplace_back(std::make_unique<QuartusTool>(opt.chat));      break;
        case 2: tools.emplace_back(std::make_unique<QuartusProTool>(opt.chat));   break;
        case 3: tools.emplace_back(std::make_unique<VivadoTool>(opt.chat));       break;
        case 4: tools.emplace_back(std::make_unique<IcarusTool>(opt.chat));       break;
        case 5: tools.emplace_back(std::make_unique<ModelSimOnlyTool>(opt.chat)); break;
        case 6: tools.emplace_back(std::make_unique<tools::CompareSimTool>(opt.chat));   break;
    }

    // main fuzz loop
    Session sess("build");
    indicators::BlockProgressBar bar{
        indicators::option::MaxProgress{static_cast<size_t>(opt.iter)},
        indicators::option::BarWidth{40},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::PrefixText{ opt.hier ? "Hierarchy-fuzzing " : "Generate-fuzzing "}
    };

    const fs::path projectRoot = fs::current_path();   // remember once

    for (int i=0; i<opt.iter; ++i)
    {
        // 1. make iteration folder
        fs::path runDir = sess.next();

        // 2. temporarily chdir into that folder
        fs::path oldCwd = fs::current_path();
        fs::current_path(runDir);

        // 3. generate RTL + golden
        fs::path  rtlPath;
        uint32_t  golden = 0;

        if (opt.hier) {
            auto [p, val] = hierGen->write("top.v");   // returns absolute path
            rtlPath = p;
            golden  = val;
            auto top_ast = hierGen->lastModule(); // Store generated 'Generate' module for use in hierarchy generator if enabled.
        } else {
            auto [fn, val] = loopGen->make("top", i, 3); // fn is relative
            rtlPath = fs::absolute(fn);                  // make it absolute
            golden  = val;
        }

        // 4. back to original cwd
        fs::current_path(oldCwd);

        // 5. run selected tool chain
        for (auto& t : tools) {
            fs::path tDir = runDir / t->name();
            auto     res  = t->run(rtlPath, "top", tDir);
          
            // First, did the tool crash or disagree internally?
            if (!res.success) {
              std::cerr << "\n[" << t->name() << "] tool failure in iteration "
                        << i << "\n  Logs: " << res.log << "\n";
              return 2;
            }
          
            // If this isn't the compare‐sim tool, compare against the golden model:
            if (t->name() != "CompareSim") {
              if (res.value != golden) {
                std::cerr << "\n[" << t->name() << "] mismatch in iteration "
                          << i << "\n  got 0x" << std::hex << res.value
                          << " expected 0x"    << golden   << std::dec
                          << "\nLogs: " << res.log << "\n";
                return 2;
              }
            }
        }

        bar.tick();
        bar.set_option(indicators::option::PostfixText{
            "iteration " + std::to_string(i+1) + "/" + std::to_string(opt.iter)});
    }

    bar.mark_as_completed();
    std::cout << "\nAll " << opt.iter << " iterations passed. Artefacts in "
            << sess.dir() << "\n";
    return 0;
}
