#pragma once
//  util/make_numbered.hpp
//
//  Tiny helper:  "top.v" + idx → "top_00.v"
//
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <string>

namespace util
{
/// Return <basename>_NN.<ext>
/// • If `basename` already contains a path, the path is preserved.
/// • `digits` controls the zero-padding (default 2  → 00, 01, …).
inline std::string
make_numbered(const std::filesystem::path& basename,
              int                            idx,
              int                            digits = 2)
{
    std::ostringstream oss;
    oss << basename.stem().string()                //  "top"
        << '_' << std::setfill('0') << std::setw(digits)
        << idx                                    //  "00"
        << basename.extension().string();          //  ".v"

    return basename.has_parent_path()
           ? (basename.parent_path() / oss.str()).string()
           : oss.str();
}
} // namespace util
