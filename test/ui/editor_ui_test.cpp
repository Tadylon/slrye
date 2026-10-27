#include <ui/editor_ui.h>

#include <catch2/catch_test_macros.hpp>
#include <imgui_internal.h>

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

TEST_CASE("Editor UI switches themes at runtime")
{
    ImGuiContextScope context;
    slrye::EditorUI editor_ui;
    editor_ui.initialize();

    const ImVec4 light_background = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    editor_ui.set_theme(slrye::EditorTheme::EnemyMouse);
    const ImVec4 enemy_mouse_background = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

    CHECK(editor_ui.theme() == slrye::EditorTheme::EnemyMouse);
    CHECK(enemy_mouse_background.x != light_background.x);
    CHECK(enemy_mouse_background.y != light_background.y);
    CHECK(enemy_mouse_background.z != light_background.z);
}

TEST_CASE("Editor shell submits the workspace, docking area, and status bar")
{
    ImGuiContextScope context;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;

    slrye::EditorUI editor_ui;
    editor_ui.initialize();

    ImGui::NewFrame();
    editor_ui.render(io);
    ImGui::Render();

    CHECK(ImGui::FindWindowByName("SLRYE") != nullptr);
    REQUIRE(ImGui::GetDrawData() != nullptr);
    CHECK(ImGui::GetDrawData()->Valid);
    CHECK(ImGui::GetDrawData()->CmdListsCount > 0);
}
