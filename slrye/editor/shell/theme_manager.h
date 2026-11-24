#pragma once

#include <editor/shell/editor_settings.h>

namespace slrye
{

/**
 * @brief 运行时视觉主题管理器
 * 
 * 职责：
 * 1. 负责在运行时无缝切换 Dear ImGui 的全局调色板、圆角和控件几何间距；
 * 2. 提供面向人类易读的 UI 菜单显示名称（区别于 YAML 序列化所用的全小写标识符）。
 */
class ThemeManager
{
public:
    /// @brief 应用指定的主题色彩方案到当前 ImGui 上下文
    void apply(EditorTheme theme);

    /// @brief 获取主题在 UI 菜单栏或状态栏中的易读展示标签（例如 "Spectrum Light"）
    [[nodiscard]] static const char* display_name(EditorTheme theme) noexcept;
};

}
