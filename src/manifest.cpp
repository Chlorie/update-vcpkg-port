#include "manifest.h"

#include <array>
#include <nlohmann/json.hpp>

#include "utils.h"

namespace uvp
{
    namespace nl = nlohmann;

    Manifest::Manifest(fs::path path):
        path_(std::move(path))
    {
        constexpr std::array<std::string_view, 4> arr
        {
            "version",
            "version-semver",
            "version-date",
            "version-string"
        };

        const auto j = nl::json::parse(read_all_text(path_));
        for (const auto& [k, v] : j.items())
        {
            if (std::ranges::find(arr, k) != arr.end())
            {
                version_type_ = k;
                version_ = v;
            }
            if (k == "port-version")
                port_version_ = v.get<int>();
        }
    }
}
