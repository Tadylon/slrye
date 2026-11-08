#pragma once

#include <imgui.h>

#include <core/vulkan_context.h>

namespace slrye
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
    void render(ImVec4 clear_color);
    void shutdown();

    ImGuiIO& io() const;
    [[nodiscard]] const VulkanContext& context() const noexcept;

private:
    VulkanBackend();
    ~VulkanBackend();

    VulkanContext context_;
    bool initialized_ = false;
};

}
