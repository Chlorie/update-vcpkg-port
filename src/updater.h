#pragma once

#include "config.h"
#include "manifest.h"
#include "portfile.h"

namespace uvp
{
    class Updater final
    {
    private:
        Config config_;
        Portfile portfile_;
        Manifest manifest_;
        std::optional<std::string> vcpkg_config_;
        fs::path version_file_;

        void print_config() const;
        void get_portfile();
        void clone_or_pull_remote_repo();
        void get_manifest();
        void get_vcpkg_config();
        void update_versions();
        void setup_test() const;
        std::string test_install() const;
        void update_sha512(std::string_view hash);
        void push_remote() const;

    public:
        explicit Updater(Config config): config_(std::move(config)) {}
        void run();
    };
}
