#pragma once
/* session.hpp – timestamped session & iteration folder helper
    ----------------------------------------------------------------
    Example:
        Session sess("build");
        fs::path iter0 = sess.next();   // build/2025‑05‑14_18‑22‑33/00000
------------------------------------------------------------------ */
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

class Session {
    fs::path sessionDir;
    int      counter = 0;
public:
    explicit Session(const fs::path& baseDir = "build")
    {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm     = *std::localtime(&tt);

        std::ostringstream stamp;
        stamp << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
        sessionDir = baseDir / stamp.str();
        fs::create_directories(sessionDir);
    }

    // path for current session
    const fs::path& dir() const { return sessionDir; }

    // returns build/<stamp>/00042
    fs::path next()
    {
        std::ostringstream pad; pad<<std::setw(5)<<std::setfill('0')<<counter++;
        fs::path p = sessionDir / pad.str();
        fs::create_directories(p);
        return p;
    }
};