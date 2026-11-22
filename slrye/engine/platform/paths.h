#pragma once

#include <filesystem>

namespace slrye::platform
{

std::filesystem::path resolve_executable_path(int argc, char** argv);
std::filesystem::path plugins_directory(const std::filesystem::path& executable_path);
std::filesystem::path assets_directory(const std::filesystem::path& executable_path);
std::filesystem::path user_config_directory();

}

namespace slrye
{
using platform::resolve_executable_path;
using platform::plugins_directory;
using platform::assets_directory;
using platform::user_config_directory;
}
