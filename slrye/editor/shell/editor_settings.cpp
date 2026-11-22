#include <editor/shell/editor_settings.h>

namespace slrye
{

std::string_view editor_theme_name(EditorTheme theme) noexcept
{
    switch (theme)
    {
    case EditorTheme::SpectrumLight:
        return "SpectrumLight";
    case EditorTheme::SpectrumDark:
        return "SpectrumDark";
    case EditorTheme::EnemyMouse:
        return "EnemyMouse";
    }
    return {};
}

std::optional<EditorTheme> parse_editor_theme(std::string_view name) noexcept
{
    if (name == "SpectrumLight")
    {
        return EditorTheme::SpectrumLight;
    }
    if (name == "SpectrumDark")
    {
        return EditorTheme::SpectrumDark;
    }
    if (name == "EnemyMouse")
    {
        return EditorTheme::EnemyMouse;
    }
    return std::nullopt;
}

}
