#include <editor/shell/editor_ui.h>

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
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)1);
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
    slrye::EditorSettings settings;
    slrye::EditorUI& editor_ui = slrye::EditorUI::instance();
    slrye::EditorUI& same_editor_ui = slrye::EditorUI::instance();

    editor_ui.shutdown();
    editor_ui.initialize(settings);

    ImGuiIO& io = ImGui::GetIO();
    CHECK(&editor_ui == &same_editor_ui);
    CHECK((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);
    CHECK((io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0);
    CHECK(io.ConfigDpiScaleFonts);
    CHECK(io.ConfigDpiScaleViewports);
    CHECK(io.Fonts->Fonts.Size > 0);
    CHECK(ImGui::GetStyle().WindowRounding == 0.0f);
    editor_ui.shutdown();
}

TEST_CASE("Editor UI switches themes at runtime")
{
    ImGuiContextScope context;
    slrye::EditorSettings settings;
    slrye::EditorUI& editor_ui = slrye::EditorUI::instance();
    editor_ui.shutdown();
    editor_ui.initialize(settings);

    const ImVec4 light_background = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    editor_ui.set_theme(slrye::EditorTheme::EnemyMouse);
    const ImVec4 enemy_mouse_background = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

    CHECK(editor_ui.theme() == slrye::EditorTheme::EnemyMouse);
    CHECK(settings.theme == slrye::EditorTheme::EnemyMouse);
    CHECK(enemy_mouse_background.x != light_background.x);
    CHECK(enemy_mouse_background.y != light_background.y);
    CHECK(enemy_mouse_background.z != light_background.z);
    editor_ui.shutdown();
}

TEST_CASE("Editor shell submits the workspace, docking area, and status bar")
{
    ImGuiContextScope context;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;

    slrye::EditorUI& editor_ui = slrye::EditorUI::instance();
    slrye::EditorSettings settings;
    editor_ui.shutdown();
    editor_ui.initialize(settings);

    ImGui::NewFrame();
    editor_ui.render();
    ImGui::Render();

    const ImGuiWindow* workspace = ImGui::FindWindowByName("##EditorWorkspace");
    REQUIRE(workspace != nullptr);
    CHECK((workspace->Flags & ImGuiWindowFlags_MenuBar) != 0);
    CHECK(ImGui::FindWindowByName("Viewport") != nullptr);
    CHECK(ImGui::FindWindowByName("Scene Hierarchy") != nullptr);
    CHECK(ImGui::FindWindowByName("Inspector") != nullptr);
    CHECK(ImGui::FindWindowByName("Console") != nullptr);
    REQUIRE(ImGui::GetDrawData() != nullptr);
    CHECK(ImGui::GetDrawData()->Valid);
    // CHECK(ImGui::GetDrawData()->CmdListsCount > 0);
    editor_ui.shutdown();
}
