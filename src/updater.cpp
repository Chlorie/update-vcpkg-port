#include "updater.h"
#include "command.h"

#include <nlohmann/json.hpp>

namespace uvp
{
    namespace nl = nlohmann;

    void Updater::print_config() const
    {
        info("Config:");
        fmt::print(
            "    Port name:         {}\n"
            "    Ports path:        {}\n"
            "    Local repo path:   {}\n"
            "    Push to remote:    {}\n"
            "    Fix failed update: {}\n",
            config_.name, config_.ports_path.string(),
            config_.local_repo ? config_.local_repo->string() : "none",
            config_.push, config_.fix);
    }

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

    void Updater::get_vcpkg_config()
    {
        info("Finding vcpkg config file (vcpkg-configuration.json)...");
        if (const fs::path result = *config_.local_repo / "vcpkg-configuration.json";
            exists(result))
        {
            vcpkg_config_ = read_all_text(result);
            fmt::print("Found vcpkg config\n");
        }
        else
            fmt::print("No vcpkg config found\n");
    }

    void Updater::update_versions()
    {
        {
            info("Copying manifest file...");
            manifest_.copy_to(config_.ports_path / "ports" / config_.name / "vcpkg.json");
        }
        {
            info("Updating portfile REF...");
            current_path(*config_.local_repo);
            const auto obj = run_command("git rev-parse HEAD").output[0];
            portfile_.set_ref(obj);
            portfile_.save();
        }
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
        {
            info("Commit changes...");
            current_path(config_.ports_path);
            run_command("git add -A");
            if (config_.fix)
                run_command("git commit --amend --no-edit");
            else
            {
                const std::string version = manifest_.port_version() == 0 ?
                                                std::string(manifest_.version()) :
                                                fmt::format("{} (port version {})", manifest_.port_version());
                run_command(fmt::format(R"(git commit -m "Update {} to {}")", config_.name, version));
            }
        }
        {
            info("Updating version file...");
            const auto obj = run_command(fmt::format("git rev-parse HEAD:ports/{}", config_.name)).output[0];
            const char initial[]{ config_.name[0], '-', '\0' };
            version_file_ = canonical(config_.ports_path / "versions" / initial / (config_.name + ".json"));
            auto json = nl::json::parse(read_all_text(version_file_));
            auto& versions = json["versions"];
            if (const bool fix_front_version = [&]
            {
                if (versions.empty()) return false;
                const auto& front = versions.front();
                if (const auto iter = front.find(manifest_.version_type());
                    iter == front.end() || iter.value().get_ref<const std::string&>() != manifest_.version())
                    return false;
                const auto iter = front.find("port-version");
                if (iter == front.end()) return manifest_.port_version() == 0;
                return iter.value().get<int>() == manifest_.port_version();
            }(); fix_front_version)
                versions.front()["git-tree"] = obj;
            else
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
        write_all_text(test_path / "vcpkg.json", nl::json{
            { "name", "vcpkg-ports-test" },
            { "version-string", "0.0.1" },
            { "dependencies", { config_.name } }
        }.dump(4));

        nl::json vcpkg_config{
            {
                "registries", {
                    {
                        { "kind", "git" },
                        { "repository", "file:///" + config_.ports_path.generic_string() },
                        { "packages", { config_.name } }
                    }
                }
            }
        };
        if (vcpkg_config_)
        {
            if (const nl::json dep_json = nl::json::parse(*vcpkg_config_);
                dep_json.contains("registries"))
            {
                const auto& dep_reg = dep_json["registries"];
                auto& reg = vcpkg_config["registries"];
                reg.insert(reg.end(), dep_reg.begin(), dep_reg.end());
            }
        }

        write_all_text(test_path / "vcpkg-configuration.json", vcpkg_config.dump(4));
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
        if (iter == res.end())
        {
            if (code == 0)
                return {};
            else
                error("Installation test failed, and the fix cannot be done automatically");
        }
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
        print_config();
        get_portfile();
        clone_or_pull_remote_repo();
        get_manifest();
        get_vcpkg_config();
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
