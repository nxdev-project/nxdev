#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <filesystem>

namespace hacbrewpack {

constexpr const char* VERSION_STRING = "3.05";

struct Options {
    std::string title_id;
    std::string key_file;
    std::string control_dir{"control"};
    std::string exefs_dir{"exefs"};
    std::string romfs_dir{"romfs"};
    std::string out_dir{"hacbrewpack_nsp"};
    std::string temp_dir{"hacbrewpack_temp"};
    std::string type{"homebrew"};
    bool no_romfs{false};
    bool verbose{false};
};

int run(const Options& opt);

} // namespace hacbrewpack
