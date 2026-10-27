#include <platform/paths.h>

#include <cstdlib>
#include <string>

namespace slrye::platform
{
namespace
{

std::filesystem::path environment_path(const char* name)
{
#ifdef _WIN32
    const std::wstring wide_name(name, name + std::char_traits<char>::length(name));
    const wchar_t* value = _wgetenv(wide_name.c_str());
    return value != nullptr ? std::filesystem::path(value) : std::filesystem::path{};
#else
    const char* value = std::getenv(name);
    return value != nullptr ? std::filesystem::path(value) : std::filesystem::path{};
#endif
}

}

std::filesystem::path resolve_executable_path(int argc, char** argv)
{
    if (argc > 0 && argv != nullptr && argv[0] != nullptr)
    {
        return std::filesystem::absolute(argv[0]);
    }

    return std::filesystem::absolute(std::filesystem::current_path() / "slrye.exe");
}

std::filesystem::path plugins_directory(const std::filesystem::path& executable_path)
{
    return executable_path.parent_path() / "plugins";
}

std::filesystem::path user_config_directory()
{
    std::filesystem::path root = environment_path("LOCALAPPDATA");
    if (root.empty())
    {
        root = environment_path("APPDATA");
    }
    if (root.empty())
    {
        root = environment_path("USERPROFILE");
    }
    if (root.empty())
    {
        std::error_code error;
        root = std::filesystem::current_path(error);
        if (error)
        {
            root = std::filesystem::path(".");
        }
    }

    return root / "Slrye";
}

}
