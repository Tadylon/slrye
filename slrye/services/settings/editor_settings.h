#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec3.hpp>

namespace slrye
{

inline constexpr std::uint32_t kEditorSettingsFormatVersion = 1;

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
    std::vector<std::filesystem::path> recent_projects;
};

[[nodiscard]] std::optional<EditorSettings> load_editor_settings(
    const std::filesystem::path& path,
    std::string* error = nullptr);

bool save_editor_settings(
    const std::filesystem::path& path,
    const EditorSettings& settings,
    std::string* error = nullptr);

}
