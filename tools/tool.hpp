#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct ToolResult {
    bool        success;   // false -> tool crashed / synthesis failed
    uint32_t    value;     // result read back from the simulator
    fs::path    log;       // path to the main log for inspection
};

class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string name() const = 0;                       // short id, e.g. "quartus"
    virtual ToolResult  run(const fs::path& rtl,                // RTL file to use
                            const std::string& top,             // top-module name
                            const fs::path& workdir) = 0;       // per-iteration dir
};