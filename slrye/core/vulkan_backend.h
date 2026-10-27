#pragma once

#include <memory>

#include <imgui.h>

namespace slrye
{

class ImGuiBackend;

class VulkanBackend
{
public:
    VulkanBackend();
    ~VulkanBackend();

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

private:
    std::unique_ptr<ImGuiBackend> imgui_backend_;
    bool initialized_ = false;
};

}
