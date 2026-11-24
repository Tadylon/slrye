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

/**
 * @brief 编辑器可用视觉主题枚举
 */
enum class EditorTheme
{
    SpectrumLight, ///< Adobe Spectrum 明亮浅色风格
    SpectrumDark,  ///< Adobe Spectrum 深色专业风格
    EnemyMouse     ///< 经典暗色高对比度风格
};

/// @brief 将主题枚举转换为 YAML 序列化所用字符串关键字（如 "spectrum_light"）
[[nodiscard]] std::string_view editor_theme_name(EditorTheme theme) noexcept;

/// @brief 从 YAML 字符串关键字反序列化为主题枚举
[[nodiscard]] std::optional<EditorTheme> parse_editor_theme(std::string_view name) noexcept;

/**
 * @brief 编辑器全局偏好设置持久化数据结构（对应 editor.yaml）
 */
struct EditorSettings
{
    EditorTheme theme = EditorTheme::SpectrumLight;        ///< 默认主题
    glm::vec3 camera_position{0.0f, 1.5f, 6.0f};          ///< 视口相机启动默认世界坐标
    float camera_yaw_degrees = 0.0f;                      ///< 视口相机启动默认偏航角
    float camera_pitch_degrees = 0.0f;                    ///< 视口相机启动默认俯仰角
    float camera_move_speed = 6.0f;                       ///< 视口相机键盘漫游速度
    bool show_viewport_grid = true;                       ///< 3D 视口基准参考网格是否可见
    std::vector<std::filesystem::path> recent_projects;   ///< 最近打开的工程路径历史列表
};

/// @brief 从本地磁盘载入 YAML 偏好设置
[[nodiscard]] std::optional<EditorSettings> load_editor_settings(
    const std::filesystem::path& path,
    std::string* error = nullptr);

/// @brief 将当前偏好设置安全持久化写回磁盘
bool save_editor_settings(
    const std::filesystem::path& path,
    const EditorSettings& settings,
    std::string* error = nullptr);

}

namespace slrye::engine
{
using slrye::EditorTheme;
using slrye::EditorSettings;
using slrye::kEditorSettingsFormatVersion;
using slrye::load_editor_settings;
using slrye::save_editor_settings;
}
