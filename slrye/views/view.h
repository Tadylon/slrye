#pragma once

#include <imgui.h>

namespace slrye
{

struct EditorViewState
{
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);
};

ImVec4 render(ImGuiIO& io, EditorViewState& state);

}
