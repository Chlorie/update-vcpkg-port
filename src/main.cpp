#include "updater.h"
#include "utils.h"

int main(const int argc, const char* const argv[]) // NOLINT
{
    try { uvp::Updater(uvp::Config::from_cmd_args(argc, argv)).run(); }
    catch (const std::exception& e) { uvp::error("Exception: {}", e.what()); }
}
