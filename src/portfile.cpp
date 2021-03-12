#include "portfile.h"

namespace uvp
{
    namespace
    {
        std::string_view sub_sv(const std::string_view sv, const size_t begin, const size_t end)
        {
            return sv.substr(begin, end - begin);
        }

        std::span<char> sub_span(std::string& str, const size_t begin, const size_t end)
        {
            return { str.begin() + begin, str.begin() + end };
        }
    }

    void Portfile::extract_values()
    {
        const size_t func_start = content_.find("vcpkg_from_github");
        if (func_start == std::string::npos) error("The port is not from github");

        const auto check_npos = [](const size_t pos)
        {
            if (pos == std::string::npos)
                error("Syntax error in portfile");
        };

        size_t key_begin = content_.find('(', func_start);
        check_npos(key_begin);
        const size_t end = content_.find(')', ++key_begin);
        check_npos(end);

        const auto check_end = [end](const size_t pos)
        {
            if (pos > end)
                error("Syntax error in portfile");
        };

        while (true)
        {
            constexpr const char* separator = " \n\t\r)";
            key_begin = content_.find_first_not_of(separator, key_begin);
            if (key_begin > end) break;
            const size_t key_end = content_.find_first_of(separator, key_begin);
            check_end(key_end);
            const size_t value_begin = content_.find_first_not_of(separator, key_end);
            check_end(value_begin);
            const size_t value_end = content_.find_first_of(separator, value_begin);
            check_end(value_end);
            const std::string_view key = sub_sv(content_, key_begin, key_end);
            if (key == "REPO") repo_ = sub_sv(content_, value_begin, value_end);
            else if (key == "REF") ref_ = sub_span(content_, value_begin, value_end);
            else if (key == "SHA512") sha512_ = sub_span(content_, value_begin, value_end);
            key_begin = value_end;
        }

        if (repo_.empty()) error("Missing REPO parameter in vcpkg_from_github");
        if (ref_.empty()) error("Missing REF parameter in vcpkg_from_github");
        if (sha512_.empty()) error("Missing SHA512 parameter in vcpkg_from_github");
    }

    Portfile::Portfile(fs::path path):
        path_(std::move(path)), content_(read_all_text(path_)) { extract_values(); }
}
