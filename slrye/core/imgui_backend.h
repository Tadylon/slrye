#pragma once

#include <imgui.h>
#include <imgui_impl_vulkan.h>

struct GLFWwindow;

namespace slrye
{

class ImGuiBackend
{
public:
    ImGuiBackend();
    ~ImGuiBackend();

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;
    ImGuiBackend(ImGuiBackend&&) = delete;
    ImGuiBackend& operator=(ImGuiBackend&&) = delete;

    bool initialize(GLFWwindow* window, ImGui_ImplVulkan_InitInfo& init_info);
    bool begin_frame();
    void render_platform_windows();
    void shutdown();

    [[nodiscard]] ImGuiIO& io() const;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    bool initialized_ = false;
};

}
