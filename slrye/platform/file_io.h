#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace slrye::platform
{

bool read_text_file(
    const std::filesystem::path& path,
    std::string& contents,
    std::string* error = nullptr);

bool write_text_file_atomic(
    const std::filesystem::path& path,
    std::string_view contents,
    std::string* error = nullptr);

}
