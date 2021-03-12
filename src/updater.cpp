#include "updater.h"
#include "command.h"

#include <nlohmann/json.hpp>

namespace uvp
{
    namespace nl = nlohmann;

    void Updater::get_portfile()
    {
        info("Parsing portfile.cmake...");
        portfile_ = Portfile(config_.ports_path / "ports" / config_.name / "portfile.cmake");
        fmt::print("Current portfile paramaters:\n    REPO:   {}\n    REF:    {}\n    SHA512: {}\n",
            portfile_.repo(), portfile_.ref(), portfile_.sha512());
    }

    void Updater::clone_or_pull_remote_repo()
    {
        if (config_.local_repo) return;
        info("Cloning / pulling remote repo...");
        const auto temp_path = config_.ports_path / "temp";
        create_directories(temp_path);
        current_path(temp_path);
        const fs::path repo_dir = canonical(fs::path(portfile_.repo()).stem());
        if (exists(repo_dir) && is_directory(repo_dir))
        {
            current_path(repo_dir);
            run_command("git pull");
        }
        else
            run_command(fmt::format("git clone https://github.com/{}.git", portfile_.repo()));
        config_.local_repo = repo_dir;
    }

    void Updater::get_manifest()
    {
        info("Finding manifest file (vcpkg.json)...");
        if (const fs::path result = *config_.local_repo / "vcpkg.json";
            exists(result))
            manifest_ = Manifest(canonical(result));
        else
            error("Cannot find manifest file (vcpkg.json)");
        fmt::print("{}: {}, port-version: {}\n",
            manifest_.version_type(), manifest_.version(), manifest_.port_version());
    }

    void Updater::update_versions()
    {
        // Copy manifest
        {
            info("Copying manifest file...");
            manifest_.copy_to(config_.ports_path / "ports" / config_.name / "vcpkg.json");
        }
        // Update portfile (later this is updated again modifying the SHA512)
        {
            info("Updating portfile REF...");
            current_path(*config_.local_repo);
            const auto obj = run_command("git rev-parse HEAD").output[0];
            portfile_.set_ref(obj);
            portfile_.save();
        }
        // Update baseline
        {
            info("Updating baseline...");
            const auto path = config_.ports_path / "versions/baseline.json";
            auto json = nl::json::parse(read_all_text(path));
            json["default"][config_.name] = nl::json{
                { "baseline", manifest_.version() },
                { "port-version", manifest_.port_version() }
            };
            write_all_text(path, json.dump(4));
        }
        // Commit changes
        {
            info("Commit changes...");
            current_path(config_.ports_path);
            run_command("git add -A");
            run_command(fmt::format(R"(git commit -m "Update {} to {}")", config_.name, manifest_.version()));
        }
        // Update git-tree
        {
            info("Updating version file...");
            const auto obj = run_command(fmt::format("git rev-parse HEAD:ports/{}", config_.name)).output[0];
            const char initial[]{ config_.name[0], '-', '\0' };
            version_file_ = canonical(config_.ports_path / "versions" / initial / (config_.name + ".json"));
            auto json = nl::json::parse(read_all_text(version_file_));
            auto& versions = json["versions"];
            versions.insert(versions.begin(), nl::json{
                { manifest_.version_type(), manifest_.version() },
                { "port-version", manifest_.port_version() },
                { "git-tree", obj }
            });
            write_all_text(version_file_, json.dump(4));
            run_command("git add -A");
            run_command("git commit --amend --no-edit");
        }
    }

    void Updater::setup_test() const
    {
        info("Setting up installation test...");
        const auto test_path = config_.ports_path / "temp/install-test";
        if (!exists(test_path)) create_directories(test_path);
        write_all_text(test_path / "vcpkg.json", fmt::format(
            R"({{
    "name": "vcpkg-ports-test",
    "version-string": "0.0.1",
    "dependencies": [ "{}" ]
}}
)",
            config_.name));
        write_all_text(test_path / "vcpkg-configuration.json", fmt::format(
            R"({{
    "registries": [
        {{
            "kind": "git",
            "repository": "file:///{}",
            "packages": [ "{}" ]
        }}
    ]
}}
)",
            config_.ports_path.generic_string(), config_.name));
    }

    std::string Updater::test_install() const
    {
        static constexpr std::string_view actual_hash_sv = "Actual hash: [ ";
        constexpr size_t hash_length = 128;
        info("Testing installation...");
        current_path(config_.ports_path / "temp/install-test");
        const auto [code, res] = run_command("vcpkg install");
        const auto iter = std::ranges::find_if(res,
            [](const std::string& str) { return str.find(actual_hash_sv) != std::string::npos; });
        if (iter == res.end()) return {};
        const std::string& str = *iter;
        const size_t offset = str.find(actual_hash_sv) + actual_hash_sv.size();
        return str.substr(offset, hash_length);
    }

    void Updater::update_sha512(const std::string_view hash)
    {
        {
            info("Updating portfile SHA512...");
            fmt::print("Actual hash is {}\n", hash);
            current_path(config_.ports_path);
            portfile_.set_sha512(hash);
            portfile_.save();
            run_command("git add -A");
            run_command("git commit --amend --no-edit");
        }
        {
            info("Updating version file git-tree...");
            const auto obj = run_command(fmt::format("git rev-parse HEAD:ports/{}", config_.name)).output[0];
            auto json = nl::json::parse(read_all_text(version_file_));
            json["versions"][0]["git-tree"] = obj;
            write_all_text(version_file_, json.dump(4));
            run_command("git add -A");
            run_command("git commit --amend --no-edit");
        }
    }

    void Updater::push_remote() const
    {
        if (!config_.push) return;
        info("Pushing ports to remote repo...");
        current_path(config_.ports_path);
        run_command("git push");
    }

    void Updater::run()
    {
        get_portfile();
        clone_or_pull_remote_repo();
        get_manifest();
        update_versions();
        setup_test();
        while (true)
        {
            const std::string hash = test_install();
            if (hash.empty()) break;
            update_sha512(hash);
        }
        push_remote();
        info("Port {} updated successfully!", config_.name);
    }
}
