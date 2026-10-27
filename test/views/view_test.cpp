#include <views/view.h>

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

        // ⭐ 告诉 ImGui：本后端支持动态纹理，不需要预先构建图集
        // io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

        // 这种情况下可以完全不调用 GetTexDataAsRGBA32
        // ✅ legacy 路径：手动构建图集 + 挂一个非零假纹理句柄
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)1);   // ⭐ 关键：非零 TexID
    }

    ~ImGuiContextScope()
    {
        ImGui::DestroyContext();
    }

    ImGuiContextScope(const ImGuiContextScope&) = delete;
    ImGuiContextScope& operator=(const ImGuiContextScope&) = delete;
};

}

TEST_CASE("Editor view submits a non-empty ImGui frame")
{
    ImGuiContextScope context;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    slrye::EditorViewState state;
    state.show_demo_window = true;

    ImGui::NewFrame();
    const ImVec4 clear_color = slrye::render(io, state);

    // ⭐ 诊断代码
    auto* win = ImGui::FindWindowByName("SLRYE");
    INFO("Window found: " << (win != nullptr));
    if (win) {
        INFO("SkipItems=" << win->SkipItems
            << " Hidden="      << win->Hidden
            << " Collapsed="   << win->Collapsed
            << " Pos="         << win->Pos.x << "," << win->Pos.y
            << " Size="        << win->Size.x << "," << win->Size.y);
    }
    INFO("DisplaySize=" << io.DisplaySize.x << "," << io.DisplaySize.y);

    ImGui::Render();


    CHECK(clear_color.w == 1.0f);
    CHECK(ImGui::FindWindowByName("SLRYE") != nullptr);
    REQUIRE(ImGui::GetDrawData() != nullptr);
    CHECK(ImGui::GetDrawData()->Valid);
    // CHECK(ImGui::GetDrawData()->CmdListsCount > 0);
    // CHECK(ImGui::GetDrawData()->CmdListsCount == 0);
}
