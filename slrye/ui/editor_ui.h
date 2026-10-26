#pragma once

#include <imgui.h>

namespace slrye
{

class EditorUI
{
public:
    void initialize();
    ImVec4 render(ImGuiIO& io) const;
};

}
