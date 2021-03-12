#pragma once

#include <vector>
#include <string>

namespace uvp
{
    struct ProcessResult final
    {
        int return_code = 0;
        std::vector<std::string> output;
    };

    ProcessResult run_command(const std::string& command);
}
