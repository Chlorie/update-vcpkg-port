#pragma once

#include <span>
#include <filesystem>
#include <fmt/color.h>

namespace uvp
{
    namespace fs = std::filesystem;

    template <typename... Ts>
    [[noreturn]] void error(Ts&&... args)
    {
        print(fg(fmt::color::red), std::forward<Ts>(args)...);
        std::puts("");
        std::exit(1);
    }

    template <typename... Ts>
    void info(Ts&&... args)
    {
        print(fg(fmt::color::khaki), std::forward<Ts>(args)...);
        std::puts("");
    }

    void overwrite_span(std::span<char> span, std::string_view str);
    std::string read_all_text(const fs::path& path);
    void write_all_text(const fs::path& path, std::string_view str);
}
