#include <platform/path_encoding.h>

namespace slrye::platform
{

std::string path_to_utf8(const std::filesystem::path& path)
{
    const std::u8string encoded = path.u8string();
    std::string result;
    result.reserve(encoded.size());
    for (char8_t character : encoded)
    {
        result.push_back(static_cast<char>(character));
    }
    return result;
}

std::filesystem::path path_from_utf8(std::string_view value)
{
    std::u8string encoded;
    encoded.reserve(value.size());
    for (unsigned char character : value)
    {
        encoded.push_back(static_cast<char8_t>(character));
    }
    return std::filesystem::path(encoded);
}

}
