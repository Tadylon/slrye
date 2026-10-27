#include <app/launch_options.h>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace
{

std::vector<char*> make_argv(std::vector<std::string>& arguments)
{
    std::vector<char*> result;
    result.reserve(arguments.size());
    for (std::string& argument : arguments)
    {
        result.push_back(argument.data());
    }
    return result;
}

}

TEST_CASE("Launch options keep the executable path when no project is supplied")
{
    std::vector<std::string> arguments{"slrye.exe"};
    std::vector<char*> argv = make_argv(arguments);

    const slrye::ApplicationLaunchOptions options =
        slrye::parse_launch_options(static_cast<int>(argv.size()), argv.data());

    CHECK(options.is_valid());
    CHECK(options.executable_path.filename() == std::filesystem::path("slrye.exe"));
    CHECK_FALSE(options.project_path.has_value());
}

TEST_CASE("Launch options accept a positional project path")
{
    std::vector<std::string> arguments{"slrye.exe", "levels/main.slproj"};
    std::vector<char*> argv = make_argv(arguments);

    const slrye::ApplicationLaunchOptions options =
        slrye::parse_launch_options(static_cast<int>(argv.size()), argv.data());

    REQUIRE(options.is_valid());
    REQUIRE(options.project_path.has_value());
    CHECK(*options.project_path == std::filesystem::path("levels/main.slproj"));
}

TEST_CASE("Launch options accept an explicit project option")
{
    std::vector<std::string> arguments{"slrye.exe", "--project", "examples/demo.slproj"};
    std::vector<char*> argv = make_argv(arguments);

    const slrye::ApplicationLaunchOptions options =
        slrye::parse_launch_options(static_cast<int>(argv.size()), argv.data());

    REQUIRE(options.is_valid());
    REQUIRE(options.project_path.has_value());
    CHECK(*options.project_path == std::filesystem::path("examples/demo.slproj"));
}

TEST_CASE("Launch options reject missing project values and unknown options")
{
    std::vector<std::string> missing_value_arguments{"slrye.exe", "--project"};
    std::vector<char*> missing_value_argv = make_argv(missing_value_arguments);
    CHECK_FALSE(slrye::parse_launch_options(
        static_cast<int>(missing_value_argv.size()),
        missing_value_argv.data()).is_valid());

    std::vector<std::string> unknown_option_arguments{"slrye.exe", "--unknown"};
    std::vector<char*> unknown_option_argv = make_argv(unknown_option_arguments);
    CHECK_FALSE(slrye::parse_launch_options(
        static_cast<int>(unknown_option_argv.size()),
        unknown_option_argv.data()).is_valid());
}
