#pragma once
#include <cstdlib>
#include <string>

inline int quiet_system(std::string cmd, bool verbose)
{
#ifdef _WIN32
    if (!verbose) cmd += " >nul 2>&1";
#else
    if (!verbose) cmd += " >/dev/null 2>&1";
#endif
    return std::system(cmd.c_str());
}

