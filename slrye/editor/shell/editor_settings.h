#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>

namespace slrye
{

enum class EditorTheme
{
    SpectrumLight,
    SpectrumDark,
    EnemyMouse
};

[[nodiscard]] std::string_view editor_theme_name(EditorTheme theme) noexcept;
[[nodiscard]] std::optional<EditorTheme> parse_editor_theme(std::string_view name) noexcept;

struct EditorSettings
{
    EditorTheme theme = EditorTheme::SpectrumLight;
    glm::vec3 camera_position{0.0f, 1.5f, 6.0f};
    float camera_yaw_degrees = 0.0f;
    float camera_pitch_degrees = 0.0f;
    float camera_move_speed = 6.0f;
    bool show_viewport_grid = true;
    std::vector<std::filesystem::path> recent_projects;
};

}
