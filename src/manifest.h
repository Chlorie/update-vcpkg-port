#pragma once

#include <filesystem>

namespace uvp
{
    namespace fs = std::filesystem;

    class Manifest final
    {
    private:
        fs::path path_;
        std::string version_type_;
        std::string version_;
        int port_version_ = 0;

    public:
        Manifest() = default;
        explicit Manifest(fs::path path);
        std::string_view version_type() const { return version_type_; }
        std::string_view version() const { return version_; }
        int port_version() const { return port_version_; }
        void copy_to(const fs::path& path) const { copy(path_, path, fs::copy_options::update_existing); }
    };
}
