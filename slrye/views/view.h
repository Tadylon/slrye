#pragma once

#include <imgui.h>

namespace slrye
{

struct EditorViewState
{
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.075f, 0.085f, 0.105f, 1.0f);
};

ImVec4 render(ImGuiIO& io, EditorViewState& state);

}
