#include <services/settings/editor_settings.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <support/temporary_directory.h>

TEST_CASE("Editor settings preserve theme, viewport, and recent projects")
{
    TemporaryDirectory directory;
    const std::filesystem::path path = directory.path() / "editor.yaml";

    slrye::EditorSettings settings;
    settings.theme = slrye::EditorTheme::EnemyMouse;
    settings.camera_position = glm::vec3(2.0f, 3.0f, 4.0f);
    settings.camera_yaw_degrees = -35.0f;
    settings.camera_pitch_degrees = 12.5f;
    settings.camera_move_speed = 12.0f;
    settings.show_viewport_grid = false;
    settings.recent_projects = {
        directory.path() / "first.slproj",
        directory.path() / "second.slproj"};

    std::string error;
    REQUIRE(slrye::save_editor_settings(path, settings, &error));
    const std::optional<slrye::EditorSettings> loaded =
        slrye::load_editor_settings(path, &error);

    REQUIRE(loaded.has_value());
    CHECK(loaded->theme == slrye::EditorTheme::EnemyMouse);
    CHECK(loaded->camera_position == glm::vec3(2.0f, 3.0f, 4.0f));
    CHECK(loaded->camera_yaw_degrees == Catch::Approx(-35.0f));
    CHECK(loaded->camera_pitch_degrees == Catch::Approx(12.5f));
    CHECK(loaded->camera_move_speed == Catch::Approx(12.0f));
    CHECK_FALSE(loaded->show_viewport_grid);
    CHECK(loaded->recent_projects == settings.recent_projects);
}

TEST_CASE("Editor settings reject unknown versions, themes, and malformed YAML")
{
    TemporaryDirectory directory;
    std::string error;

    const std::filesystem::path future = directory.write_file(
        "future.yaml",
        "version: 99\ntheme: SpectrumLight\n");
    CHECK_FALSE(slrye::load_editor_settings(future, &error).has_value());
    CHECK(error.find("version") != std::string::npos);

    const std::filesystem::path unknown_theme = directory.write_file(
        "theme.yaml",
        "version: 1\ntheme: Unknown\n");
    CHECK_FALSE(slrye::load_editor_settings(unknown_theme, &error).has_value());
    CHECK_FALSE(error.empty());

    const std::filesystem::path malformed = directory.write_file(
        "malformed.yaml",
        "version: [unterminated\n");
    CHECK_FALSE(slrye::load_editor_settings(malformed, &error).has_value());
    CHECK_FALSE(error.empty());
}
