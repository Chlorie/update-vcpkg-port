#include "config.h"

#include <fmt/core.h>
#include <argh.h>

namespace uvp
{
    namespace
    {
        [[noreturn]] void show_help_msg()
        {
            fmt::print(R"(Usage:
update-vcpkg-port <name> [options]

name:           name of the port to update
-h -? --help:   show this message
-p --path:      path to the ports repo, default to "./"
-l --local:     use this local path of the port's library for updating the port (optional)
                this path is relative to the -p path if specified
-a --auto:      automatically push the ports repo to remote without confirmation 
)");
            std::exit(1);
        }
    }

    Config Config::from_cmd_args(const int argc, const char* const argv[])
    {
        const argh::parser cmd(argc, argv);
        Config config;

        if (cmd[{ "-h", "-?", "--help" }] || !(cmd(1) >> config.name))
            show_help_msg();

        cmd({ "-p", "--path" }, "./") >> config.ports_path;
        config.ports_path = canonical(config.ports_path);

        if (std::string local_repo; cmd({ "-l", "--local" }) >> local_repo)
            config.local_repo = canonical(config.ports_path / local_repo);

        config.push = cmd[{ "-a", "--auto" }];

        return config;
    }
}
