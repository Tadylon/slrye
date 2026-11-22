#pragma once

#include <memory>
#include <engine/rhi/vulkan/vulkan_context.h>

struct GLFWwindow;

namespace slrye::engine
{

class VulkanBackend
{
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

    struct Impl;
    std::unique_ptr<Impl> impl_;

    VulkanContext context_;
    GLFWwindow* window_ = nullptr;
    bool glfw_initialized_ = false;
    bool initialized_ = false;
};

}
