#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace slrye::platform
{

[[nodiscard]] std::string path_to_utf8(const std::filesystem::path& path);
[[nodiscard]] std::filesystem::path path_from_utf8(std::string_view value);

}
