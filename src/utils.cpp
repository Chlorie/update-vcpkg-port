#include "utils.h"

#include <fstream>

namespace uvp
{
    void overwrite_span(const std::span<char> span, const std::string_view str)
    {
        if (span.size() != str.size()) error("Size of span and string don't match");
        (void)std::ranges::copy(str, span.begin());
    }

    std::string read_all_text(const fs::path& path)
    {
        std::ifstream fs(path);
        fs.ignore(std::numeric_limits<std::streamsize>::max());
        const std::streamsize size = fs.gcount();
        std::string result(size, '\0');
        fs.seekg(0, std::ios::beg);
        fs.read(result.data(), size);
        if (fs.fail() && !fs.eof()) error("Failed to read text file: {}", path.string());
        return result;
    }

    void write_all_text(const fs::path& path, const std::string_view str)
    {
        std::ofstream fs(path);
        fs << str;
    }
}
