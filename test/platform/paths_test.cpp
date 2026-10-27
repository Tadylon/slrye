#include <platform/paths.h>
#include <platform/path_encoding.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

TEST_CASE("Executable paths resolve relative to the current directory")
{
    std::vector<std::string> arguments{"bin/slrye.exe"};
    std::vector<char*> argv;
    for (std::string& argument : arguments)
    {
        argv.push_back(argument.data());
    }

    const std::filesystem::path executable =
        slrye::platform::resolve_executable_path(static_cast<int>(argv.size()), argv.data());

    CHECK(executable.is_absolute());
    CHECK(executable.filename() == std::filesystem::path("slrye.exe"));
}

TEST_CASE("Plugins directory is a sibling of the executable")
{
    const std::filesystem::path executable = std::filesystem::path("install") / "bin" / "slrye.exe";

    const std::filesystem::path plugins = slrye::platform::plugins_directory(executable);

    CHECK(plugins.parent_path() == executable.parent_path());
    CHECK(plugins.filename() == std::filesystem::path("plugins"));
}

TEST_CASE("User configuration is stored below a Slrye directory")
{
    const std::filesystem::path directory = slrye::platform::user_config_directory();

    CHECK(directory.is_absolute());
    CHECK(directory.filename() == std::filesystem::path("Slrye"));
}

TEST_CASE("Filesystem paths round-trip as UTF-8")
{
    const std::string utf8_path = "levels/\xE4\xB8\xBB\xE5\x9C\xBA\xE6\x99\xAF.slscene";
    const std::filesystem::path path = slrye::platform::path_from_utf8(utf8_path);

    CHECK(slrye::platform::path_to_utf8(path) == utf8_path);
}
