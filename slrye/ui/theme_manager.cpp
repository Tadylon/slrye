#include <ui/theme_manager.h>

#include <imgui.h>

#include <ui/styles/style.h>

namespace slrye
{

void ThemeManager::apply(EditorTheme theme)
{
    ImGuiStyle* style = nullptr;
    switch (theme)
    {
    case EditorTheme::SpectrumLight:
        style = &ImGui::Spectrum::StyleColorsSpectrum(false);
        break;
    case EditorTheme::SpectrumDark:
        style = &ImGui::Spectrum::StyleColorsSpectrum(true);
        break;
    case EditorTheme::EnemyMouse:
        style = &imgui_style_enemymouse();
        break;
    }

    if (style != nullptr && (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        style->WindowRounding = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

const char* ThemeManager::display_name(EditorTheme theme) noexcept
{
    switch (theme)
    {
    case EditorTheme::SpectrumLight:
        return "Spectrum Light";
    case EditorTheme::SpectrumDark:
        return "Spectrum Dark";
    case EditorTheme::EnemyMouse:
        return "EnemyMouse";
    }

    return "Unknown";
}

}
