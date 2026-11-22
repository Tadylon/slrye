#include "imgui_backend.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace slrye
{
namespace
{

constexpr std::uint32_t kMinimumImageCount = 2;

void check_vk_result(VkResult result)
{
    if (result == VK_SUCCESS)
    {
        return;
    }

    std::fprintf(stderr, "[vulkan-imgui] VkResult = %d\n", static_cast<int>(result));
    if (result < 0)
    {
        std::abort();
    }
}

bool create_imgui_descriptor_pool(
    VkDevice device,
    const VkAllocationCallbacks* allocator,
    VkDescriptorPool& descriptor_pool)
{
    constexpr std::array<VkDescriptorPoolSize, 2> pool_sizes{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 680}};
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1680;

    const VkResult result = vkCreateDescriptorPool(
        device,
        &pool_info,
        allocator,
        &descriptor_pool);
    if (result != VK_SUCCESS)
    {
        std::fprintf(
            stderr,
            "vkCreateDescriptorPool failed with VkResult %d\n",
            static_cast<int>(result));
        descriptor_pool = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void destroy_imgui_descriptor_pool(
    VkDevice device,
    const VkAllocationCallbacks* allocator,
    VkDescriptorPool& descriptor_pool)
{
    if (device != VK_NULL_HANDLE && descriptor_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptor_pool, allocator);
    }
    descriptor_pool = VK_NULL_HANDLE;
}

void setup_vulkan_window(
    const VulkanContext& context,
    GLFWwindow* window,
    ImGui_ImplVulkanH_Window& window_data)
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkBool32 present_supported = VK_FALSE;
    check_vk_result(vkGetPhysicalDeviceSurfaceSupportKHR(
        context.physical_device,
        context.graphics_queue_family,
        context.surface,
        &present_supported));
    if (present_supported != VK_TRUE)
    {
        std::fprintf(stderr, "Vulkan queue does not support window presentation\n");
        std::abort();
    }

    constexpr std::array<VkFormat, 4> requested_formats{
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_B8G8R8_UNORM};
    window_data.Surface = context.surface;
    window_data.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        context.physical_device,
        window_data.Surface,
        requested_formats.data(),
        static_cast<int>(requested_formats.size()),
        VK_COLORSPACE_SRGB_NONLINEAR_KHR);

    constexpr VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    window_data.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        context.physical_device,
        window_data.Surface,
        &present_mode,
        1);

    ImGui_ImplVulkanH_CreateOrResizeWindow(
        context.instance,
        context.physical_device,
        context.device,
        &window_data,
        context.graphics_queue_family,
        context.allocator,
        width,
        height,
        kMinimumImageCount,
        0);
}

void cleanup_vulkan_window(
    const VulkanContext& context,
    ImGui_ImplVulkanH_Window& window_data)
{
    if (context.device != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE &&
        window_data.Surface != VK_NULL_HANDLE)
    {
        ImGui_ImplVulkanH_DestroyWindow(
            context.instance,
            context.device,
            &window_data,
            context.allocator);
    }
    window_data = {};
}

void render_frame(
    const VulkanContext& context,
    ImGui_ImplVulkanH_Window& window_data,
    ImDrawData* draw_data,
    bool& swapchain_rebuild)
{
    VkSemaphore image_acquired_semaphore =
        window_data.FrameSemaphores[window_data.SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore =
        window_data.FrameSemaphores[window_data.SemaphoreIndex].RenderCompleteSemaphore;
    VkResult result = vkAcquireNextImageKHR(
        context.device,
        window_data.Swapchain,
        UINT64_MAX,
        image_acquired_semaphore,
        VK_NULL_HANDLE,
        &window_data.FrameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        swapchain_rebuild = true;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }
    if (result != VK_SUBOPTIMAL_KHR)
    {
        check_vk_result(result);
    }

    ImGui_ImplVulkanH_Frame& frame = window_data.Frames[window_data.FrameIndex];
    check_vk_result(vkWaitForFences(
        context.device,
        1,
        &frame.Fence,
        VK_TRUE,
        UINT64_MAX));
    check_vk_result(vkResetFences(context.device, 1, &frame.Fence));
    check_vk_result(vkResetCommandPool(context.device, frame.CommandPool, 0));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check_vk_result(vkBeginCommandBuffer(frame.CommandBuffer, &begin_info));

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = window_data.RenderPass;
    render_pass_info.framebuffer = frame.Framebuffer;
    render_pass_info.renderArea.extent.width = window_data.Width;
    render_pass_info.renderArea.extent.height = window_data.Height;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &window_data.ClearValue;
    vkCmdBeginRenderPass(
        frame.CommandBuffer,
        &render_pass_info,
        VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(draw_data, frame.CommandBuffer);
    vkCmdEndRenderPass(frame.CommandBuffer);
    check_vk_result(vkEndCommandBuffer(frame.CommandBuffer));

    constexpr VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquired_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame.CommandBuffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_complete_semaphore;
    check_vk_result(vkQueueSubmit(
        context.graphics_queue,
        1,
        &submit_info,
        frame.Fence));
}

void present_frame(
    const VulkanContext& context,
    ImGui_ImplVulkanH_Window& window_data,
    bool& swapchain_rebuild)
{
    if (swapchain_rebuild)
    {
        return;
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =
        &window_data.FrameSemaphores[window_data.SemaphoreIndex].RenderCompleteSemaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &window_data.Swapchain;
    present_info.pImageIndices = &window_data.FrameIndex;
    const VkResult result = vkQueuePresentKHR(context.graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        swapchain_rebuild = true;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }
    if (result != VK_SUBOPTIMAL_KHR)
    {
        check_vk_result(result);
    }
    window_data.SemaphoreIndex =
        (window_data.SemaphoreIndex + 1) % window_data.SemaphoreCount;
}

}

ImGuiBackend& ImGuiBackend::instance()
{
    static ImGuiBackend backend;
    return backend;
}

ImGuiBackend::ImGuiBackend() = default;

ImGuiBackend::~ImGuiBackend()
{
    shutdown();
}

bool ImGuiBackend::initialize(GLFWwindow* window, const VulkanContext& context)
{
    if (initialized_ || ImGui::GetCurrentContext() != nullptr ||
        window == nullptr || !context.is_valid())
    {
        return false;
    }

    context_ = &context;
    window_ = window;
    if (!create_imgui_descriptor_pool(
            context_->device,
            context_->allocator,
            descriptor_pool_))
    {
        context_ = nullptr;
        window_ = nullptr;
        return false;
    }

    setup_vulkan_window(*context_, window_, window_data_);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = context.instance;
    init_info.PhysicalDevice = context.physical_device;
    init_info.Device = context.device;
    init_info.QueueFamily = context.graphics_queue_family;
    init_info.Queue = context.graphics_queue;
    init_info.DescriptorPool = descriptor_pool_;
    init_info.MinImageCount = kMinimumImageCount;
    init_info.ImageCount = window_data_.ImageCount;
    init_info.Allocator = context.allocator;
    init_info.PipelineInfoMain.RenderPass = window_data_.RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForVulkan(window_, true))
    {
        ImGui::DestroyContext();
        cleanup_vulkan_window(*context_, window_data_);
        destroy_imgui_descriptor_pool(
            context_->device,
            context_->allocator,
            descriptor_pool_);
        context_ = nullptr;
        window_ = nullptr;
        return false;
    }

    if (!ImGui_ImplVulkan_Init(&init_info))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        cleanup_vulkan_window(*context_, window_data_);
        destroy_imgui_descriptor_pool(
            context_->device,
            context_->allocator,
            descriptor_pool_);
        context_ = nullptr;
        window_ = nullptr;
        return false;
    }

    initialized_ = true;
    return true;
}

bool ImGuiBackend::begin_frame()
{
    if (!initialized_ || context_ == nullptr || window_ == nullptr ||
        !ensure_swapchain())
    {
        return false;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    return true;
}

void ImGuiBackend::render(ImVec4 clear_color)
{
    if (!initialized_ || context_ == nullptr)
    {
        return;
    }

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool main_is_minimized =
        draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f;

    window_data_.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    window_data_.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    window_data_.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    window_data_.ClearValue.color.float32[3] = clear_color.w;
    if (!main_is_minimized)
    {
        render_frame(*context_, window_data_, draw_data, swapchain_rebuild_);
    }

    render_platform_windows();
    if (!main_is_minimized)
    {
        present_frame(*context_, window_data_, swapchain_rebuild_);
    }
}

void ImGuiBackend::shutdown()
{
    if (initialized_)
    {
        if (context_ != nullptr && context_->device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(context_->device);
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialized_ = false;
    }

    if (context_ != nullptr)
    {
        cleanup_vulkan_window(*context_, window_data_);
        destroy_imgui_descriptor_pool(
            context_->device,
            context_->allocator,
            descriptor_pool_);
    }
    else
    {
        window_data_ = {};
    }
    context_ = nullptr;
    window_ = nullptr;
    swapchain_rebuild_ = false;
}

ImGuiIO& ImGuiBackend::io() const
{
    return ImGui::GetIO();
}

const VulkanContext* ImGuiBackend::vulkan_context() const noexcept
{
    return context_;
}

bool ImGuiBackend::is_initialized() const noexcept
{
    return initialized_;
}

bool ImGuiBackend::ensure_swapchain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!swapchain_rebuild_ &&
        window_data_.Width == width &&
        window_data_.Height == height)
    {
        return true;
    }

    ImGui_ImplVulkan_SetMinImageCount(kMinimumImageCount);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        context_->instance,
        context_->physical_device,
        context_->device,
        &window_data_,
        context_->graphics_queue_family,
        context_->allocator,
        width,
        height,
        kMinimumImageCount,
        0);
    window_data_.FrameIndex = 0;
    swapchain_rebuild_ = false;
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

}
