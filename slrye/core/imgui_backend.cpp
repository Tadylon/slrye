#include <core/imgui_backend.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <limits>

namespace slrye
{

ImGuiBackend::ImGuiBackend() = default;

ImGuiBackend::~ImGuiBackend()
{
    shutdown();
}

bool ImGuiBackend::initialize(GLFWwindow* window, ImGui_ImplVulkan_InitInfo& init_info)
{
    if (initialized_ || ImGui::GetCurrentContext() != nullptr || window == nullptr ||
        init_info.Instance == VK_NULL_HANDLE ||
        init_info.PhysicalDevice == VK_NULL_HANDLE || init_info.Device == VK_NULL_HANDLE ||
        init_info.QueueFamily == std::numeric_limits<std::uint32_t>::max() ||
        init_info.Queue == VK_NULL_HANDLE ||
        init_info.DescriptorPool == VK_NULL_HANDLE ||
        init_info.PipelineInfoMain.RenderPass == VK_NULL_HANDLE)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    if (!ImGui_ImplGlfw_InitForVulkan(window, true))
    {
        ImGui::DestroyContext();
        return false;
    }

    ImGui_ImplVulkan_Init(&init_info);
    initialized_ = true;
    return true;
}

bool ImGuiBackend::begin_frame()
{
    if (!initialized_)
    {
        return false;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    return true;
}

void ImGuiBackend::render_platform_windows()
{
    if (!initialized_ || !(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        return;
    }

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void ImGuiBackend::shutdown()
{
    if (!initialized_)
    {
        return;
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

ImGuiIO& ImGuiBackend::io() const
{
    return ImGui::GetIO();
}

bool ImGuiBackend::is_initialized() const noexcept
{
    return initialized_;
}

}
