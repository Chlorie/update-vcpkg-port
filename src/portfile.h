#pragma once

#include "utils.h"

namespace uvp
{
    namespace fs = std::filesystem;

    class Portfile final
    {
    private:
        fs::path path_;
        std::string content_;
        std::string_view repo_;
        std::span<char> ref_;
        std::span<char> sha512_;

        void extract_values();

    public:
        Portfile() = default;
        explicit Portfile(fs::path path);
        std::string_view repo() const { return repo_; }
        std::string_view ref() const { return { ref_.data(), ref_.size() }; }
        std::string_view sha512() const { return { sha512_.data(), sha512_.size() }; }
        void set_ref(const std::string_view str) { overwrite_span(ref_, str); }
        void set_sha512(const std::string_view str) { overwrite_span(sha512_, str); }
        void save() const { write_all_text(path_, content_); }
    };
}
