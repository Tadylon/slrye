#pragma once

#include <imgui.h>

#include <ui/theme_manager.h>
#include <views/view.h>

namespace slrye
{

class EditorUI
{
public:
    void initialize();
    ImVec4 render(ImGuiIO& io);
    void set_theme(EditorTheme theme);

    [[nodiscard]] EditorTheme theme() const noexcept;

private:
    void draw_menu_bar();
    void draw_workspace();
    void draw_status_bar(float height);
    void draw_about_window();

    ThemeManager theme_manager_;
    EditorTheme theme_ = EditorTheme::SpectrumLight;
    EditorViewState view_state_;
    bool show_about_window_ = false;
};

}
