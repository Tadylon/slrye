#pragma once

#include <core/vulkan_context.h>
#include <VkBootstrap.h>
struct GLFWwindow;

namespace slrye
{

class VulkanBackend
{
private:
    // 在 VulkanBackend 内部持有 vkb 包装对象以便释放
    struct Impl
    {
        vkb::Instance vkb_instance;
        vkb::Device vkb_device;
    };
public:
    [[nodiscard]] static VulkanBackend& instance();

    VulkanBackend(const VulkanBackend&) = delete;
    VulkanBackend& operator=(const VulkanBackend&) = delete;
    VulkanBackend(VulkanBackend&&) = delete;
    VulkanBackend& operator=(VulkanBackend&&) = delete;

    bool initialize(const char* title, int width, int height);
    bool should_close() const;
    bool begin_frame();
    void shutdown();

    [[nodiscard]] GLFWwindow* window() const noexcept;
    [[nodiscard]] const VulkanContext& context() const noexcept;

private:
    VulkanBackend();
    ~VulkanBackend();

    VulkanContext context_;
    GLFWwindow* window_ = nullptr;
    VkDebugReportCallbackEXT debug_report_ = VK_NULL_HANDLE;
    Impl* impl_;
    bool glfw_initialized_ = false;
    bool initialized_ = false;
};

}
