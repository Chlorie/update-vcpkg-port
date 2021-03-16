#pragma once

#include <filesystem>
#include <optional>

namespace uvp
{
    namespace fs = std::filesystem;

    struct Config final
    {
        std::string name;
        fs::path ports_path;
        std::optional<fs::path> local_repo;
        bool push = false;
        bool fix = false;

        static Config from_cmd_args(int argc, const char* const argv[]);
    };
}
