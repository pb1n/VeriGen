#pragma once
#include "tool.hpp"
#include "icarus_sim.hpp"
#include "modelsim_sim.hpp"
#include <sstream>

namespace tools {

class CompareSimTool : public Tool {
public:
    // we no longer pass `chat` into Tool, just store it for our back-ends
    explicit CompareSimTool(bool chat)
      : icarus_(chat)
      , modelsim_(chat)
    {}

    std::string name() const override { return "CompareSim"; }

    // match Tool::run signature exactly
    ToolResult run(const fs::path &rtl,
                   const std::string &top,
                   const fs::path &outDir) override
    {
        // 1) run Icarus
        auto icarusDir = outDir / "icarus";
        ToolResult rI = icarus_.run(rtl, top, icarusDir);

        // 2) run ModelSim
        auto modelsimDir = outDir / "modelsim";
        ToolResult rM = modelsim_.run(rtl, top, modelsimDir);

        // if either failed, propagate failure
        if (!rI.success || !rM.success) {
            ToolResult fail;
            fail.success = false;
            fail.value   = rI.success ? rM.value : rI.value;
            std::ostringstream ss;
            ss << "=== Icarus log ===\n"
               << rI.log.string()
               << "\n=== ModelSim log ===\n"
               << rM.log.string();
            fail.log = ss.str();
            return fail;
        }

        // 3) compare results
        if (rI.value != rM.value) {
            ToolResult mismatch;
            mismatch.success = false;
            mismatch.value   = rI.value;
            std::ostringstream ss;
            ss << "Mismatch: Icarus=0x" << std::hex << rI.value
               << "  ModelSim=0x" << std::hex << rM.value;
            mismatch.log = ss.str();
            return mismatch;
        }

        // 4) agreed
        ToolResult ok;
        ok.success = true;
        ok.value   = rI.value;
        ok.log     = "";
        return ok;
    }

private:
    IcarusTool       icarus_;
    ModelSimOnlyTool modelsim_;
};

} // namespace tools
