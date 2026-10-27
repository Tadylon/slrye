#include <views/view.h>

namespace slrye
{

ImVec4 render(ImGuiIO& io, EditorViewState& state)
{
    if (state.show_demo_window)
    {
        static float value = 0.0f;
        static int counter = 0;

        ImGui::Begin("SLRYE", &state.show_demo_window);
        ImGui::Text("Editor workspace preview");
        ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);
        ImGui::ColorEdit3("Clear color", &state.clear_color.x);

        if (ImGui::Button("Increment"))
        {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("Counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    return state.clear_color;
}

}
