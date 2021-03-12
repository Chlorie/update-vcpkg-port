#include "command.h"

#include <boost/process.hpp>
#include "utils.h"

namespace uvp
{
    namespace bp = boost::process;

    ProcessResult run_command(const std::string& command)
    {
        print(fg(fmt::color::cornflower_blue), "Running command: {}\n", command);

        bp::ipstream stream;
        bp::child child(command, bp::std_out > stream);

        std::string line;
        ProcessResult result;
        while (stream && std::getline(stream, line))
        {
            fmt::print("{}\n", line);
            result.output.push_back(std::move(line));
        }

        child.wait();
        result.return_code = child.exit_code();

        print(fg(fmt::color::cornflower_blue), "Process returned {}\n", result.return_code);

        return result;
    }
}
