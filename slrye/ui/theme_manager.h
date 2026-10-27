#pragma once

namespace slrye
{

enum class EditorTheme
{
    SpectrumLight,
    SpectrumDark,
    EnemyMouse
};

class ThemeManager
{
public:
    void apply(EditorTheme theme);

    [[nodiscard]] static const char* display_name(EditorTheme theme) noexcept;
};

}
