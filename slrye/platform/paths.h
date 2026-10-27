#pragma once

#include <filesystem>

namespace slrye::platform
{

std::filesystem::path resolve_executable_path(int argc, char** argv);
std::filesystem::path plugins_directory(const std::filesystem::path& executable_path);

}
