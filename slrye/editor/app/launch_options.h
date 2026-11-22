#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace slrye
{

struct ApplicationLaunchOptions
{
    std::filesystem::path executable_path;
    std::optional<std::filesystem::path> project_path;
    std::string error;

    [[nodiscard]] bool is_valid() const noexcept
    {
        return error.empty();
    }
};

ApplicationLaunchOptions parse_launch_options(int argc, char** argv);

}
