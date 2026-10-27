#include <platform/paths.h>

namespace slrye::platform
{

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

}
