#include <ui/editor_ui.h>

#include <catch2/catch_test_macros.hpp>

namespace
{

class ImGuiContextScope
{
public:
    ImGuiContextScope()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
    }

    ~ImGuiContextScope()
    {
        ImGui::DestroyContext();
    }

    ImGuiContextScope(const ImGuiContextScope&) = delete;
    ImGuiContextScope& operator=(const ImGuiContextScope&) = delete;
};

}

TEST_CASE("Editor UI configures docking, keyboard navigation, and a default font")
{
    ImGuiContextScope context;
    slrye::EditorUI editor_ui;

    editor_ui.initialize();

    ImGuiIO& io = ImGui::GetIO();
    CHECK((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);
    CHECK((io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0);
    CHECK(io.ConfigDpiScaleFonts);
    CHECK(io.ConfigDpiScaleViewports);
    CHECK(io.Fonts->Fonts.Size > 0);
    CHECK(ImGui::GetStyle().WindowRounding == 0.0f);
}
