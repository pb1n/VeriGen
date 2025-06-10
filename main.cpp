#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

#include <indicators/block_progress_bar.hpp>

#include "session.hpp"
#include "ast.hpp"
#include "tools/quartus_pro_synth_sim.hpp"
#include "tools/quartus_synth_sim.hpp"
#include "tools/icarus_sim.hpp"
#include "tools/vivado_synth_sim.hpp"
#include "tools/tool.hpp"

namespace fs = std::filesystem;

/*───────────────────────────────────────────────────────────────*/
/*  CLI options                                                  */
/*───────────────────────────────────────────────────────────────*/
struct Opt
{
    int iter = 1;
    unsigned seed = std::random_device{}();
    int tool = 1; // 1 = Quartus, 2 = Icarus/ModelSim
    bool chat = false;
};

Opt parse(int argc, char *argv[])
{
    Opt o;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if ((a == "--iter" || a == "-n") && i + 1 < argc)
            o.iter = std::stoi(argv[++i]);
        else if ((a == "--seed" || a == "-s") && i + 1 < argc)
            o.seed = std::stoul(argv[++i]);
        else if ((a == "--tool" || a == "-t") && i + 1 < argc)
            o.tool = std::stoi(argv[++i]);
        else if (a == "--chat" || a == "-c")
            o.chat = true;
        else
        {
            std::cerr << "Usage: " << argv[0]
                      << " [-n N] [-s SEED] [-t TOOL] [-c]\n";
            std::exit(1);
        }
    }
    if (o.tool < 1 || o.tool > 4)
    {
        std::cerr << "Error: --tool must be:\n - 1: Quartus Prime/ModelSim\n - 2: Quartus Pro/ModelSim\n - 3: Vivado\n - 4: Icarus\n";
        std::exit(1);
    }
    return o;
}

/*───────────────────────────────────────────────────────────────*/
/*  Main                                                         */
/*───────────────────────────────────────────────────────────────*/
int main(int argc, char *argv[])
{
    Opt opt = parse(argc, argv);

    veri::Generator gen(false, /*for*/ true, /*if*/ false, /*case*/ false,
                        opt.seed);

    /* -- select tool(s) exactly as before -- */
    std::vector<std::unique_ptr<Tool>> tools;
    switch (opt.tool)
    {
    case 1:
        tools.emplace_back(std::make_unique<QuartusTool>(opt.chat));
        break;
    case 2:
        tools.emplace_back(std::make_unique<QuartusProTool>(opt.chat));
        break;
    case 3:
        tools.emplace_back(std::make_unique<VivadoTool>(opt.chat));
        break;
    case 4:
        tools.emplace_back(std::make_unique<IcarusTool>(opt.chat));
        break;
    }

    Session sess("build");
    indicators::BlockProgressBar bar{/* …unchanged… */};

    for (int i = 0; i < opt.iter; ++i)
    {

        /* -------------- create iteration folder ---------------- */
        fs::path runDir = sess.next();

        /* -------------- generate Verilog *inside* runDir ------- */
        fs::path oldCwd = fs::current_path();
        fs::current_path(runDir); // ← temporally cd
        auto [rtlName, expected] = gen.make("top", i, 2, 2);
        fs::current_path(oldCwd); // ← restore

        /*  rtlName is just "gen_<idx>.v", build full path once  */
        fs::path rtl = runDir / rtlName;

        /* -------------- run selected EDA tool(s) --------------- */
        for (auto &t : tools)
        {
            fs::path tDir = runDir / t->name();
            auto r = t->run(rtl, "top", tDir);

            if (!r.success || r.value != expected)
            {
                std::cout << "\n[" << t->name()
                          << "] failure/mismatch in iter " << i
                          << ". Logs: " << r.log << "\n"
                          << "got: " << r.value
                          << "  expected: " << expected << "\n";
                return 2;
            }
        }

        bar.tick();
        bar.set_option(indicators::option::PostfixText{
            "iter " + std::to_string(i + 1) + "/" + std::to_string(opt.iter)});
    }

    bar.mark_as_completed();
    std::cout << "\nAll " << opt.iter << " iterations passed. Artefacts in "
              << sess.dir() << "\n";
    return 0;
}
