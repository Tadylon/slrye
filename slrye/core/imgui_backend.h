#pragma once

// #include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <core/vulkan_context.h>

struct GLFWwindow;

namespace slrye
{

class ImGuiBackend
{
public:
    [[nodiscard]] static ImGuiBackend& instance();

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;
    ImGuiBackend(ImGuiBackend&&) = delete;
    ImGuiBackend& operator=(ImGuiBackend&&) = delete;

    bool initialize(GLFWwindow* window, const VulkanContext& context);
    bool begin_frame();
    void render(ImVec4 clear_color);
    void shutdown();

    [[nodiscard]] ImGuiIO& io() const;
    [[nodiscard]] const VulkanContext* vulkan_context() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    ImGuiBackend();
    ~ImGuiBackend();

    bool ensure_swapchain();
    void render_platform_windows();

    const VulkanContext* context_ = nullptr;
    GLFWwindow* window_ = nullptr;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    ImGui_ImplVulkanH_Window window_data_{};
    bool swapchain_rebuild_ = false;
    bool initialized_ = false;
};

}
