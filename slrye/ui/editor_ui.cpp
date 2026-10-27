#include <ui/editor_ui.h>

#include <ui/styles/style.h>
#include <views/view.h>

namespace slrye
{

void EditorUI::initialize()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGuiStyle& style = ImGui::Spectrum::StyleColorsSpectrum();
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    io.Fonts->AddFontDefault();
}

ImVec4 EditorUI::render(ImGuiIO& io) const
{
    return slrye::render(io);
}

}
