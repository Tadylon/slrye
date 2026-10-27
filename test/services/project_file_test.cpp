#include <services/project/project_file.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <support/temporary_directory.h>

TEST_CASE("Project files round-trip and resolve their start scene")
{
    TemporaryDirectory directory;
    const std::filesystem::path path = directory.path() / "sample.slproj";
    const slrye::ProjectFile project{"Sample", std::filesystem::path("levels") / "main.slscene"};

    std::string error;
    REQUIRE(slrye::save_project_file(path, project, &error));

    const std::optional<slrye::ProjectFile> loaded =
        slrye::load_project_file(path, &error);
    REQUIRE(loaded.has_value());
    CHECK(loaded->name == "Sample");
    CHECK(loaded->start_scene == project.start_scene);

    const std::filesystem::path scene_path =
        slrye::resolve_project_scene_path(path, loaded->start_scene);
    CHECK(scene_path == (directory.path() / "levels" / "main.slscene").lexically_normal());
}

TEST_CASE("Project files reject invalid and unsupported content")
{
    TemporaryDirectory directory;
    std::string error;

    const std::filesystem::path missing_scene = directory.write_file(
        "missing_scene.slproj",
        "version: 1\nname: Broken\n");
    CHECK_FALSE(slrye::load_project_file(missing_scene, &error).has_value());
    CHECK_FALSE(error.empty());

    const std::filesystem::path future = directory.write_file(
        "future.slproj",
        "version: 99\nname: Future\nstart_scene: main.slscene\n");
    CHECK_FALSE(slrye::load_project_file(future, &error).has_value());
    CHECK(error.find("version") != std::string::npos);

    const std::filesystem::path malformed = directory.write_file(
        "malformed.slproj",
        "version: [unterminated\n");
    CHECK_FALSE(slrye::load_project_file(malformed, &error).has_value());
    CHECK_FALSE(error.empty());
}
