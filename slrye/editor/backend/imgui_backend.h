#pragma once

#include <imgui_impl_vulkan.h>

#include <engine/rhi/vulkan/vulkan_context.h>

struct GLFWwindow;

namespace slrye::engine
{
class VulkanBackend;
}

namespace slrye
{

/**
 * @brief ImGui 后端适配器（借用者 / 中间件）
 * 
 * 职责：
 * 1. 桥接 Dear ImGui 与底层 Vulkan 交换链以及 GLFW 平台输入；
 * 2. 自身【不创建也不拥有】物理窗口或 Vulkan 硬件设备，所有硬件句柄均借用自 VulkanBackend；
 * 3. 内部持有：
 *    - `GLFWwindow* window_`: 借用的主窗口指针，供 ImGui_ImplGlfw 注册鼠标/键盘/窗口尺寸监听钩子；
 *    - `const VulkanContext* context_`: 借用的 Vulkan 句柄集，供 ImGui_ImplVulkan 在主交换链上录制与呈现 UI DrawData；
 *    - `VkDescriptorPool descriptor_pool_`: ImGui 专用描述符池（分配字体和 Viewport 纹理描述符）；
 *    - `ImGui_ImplVulkanH_Window window_data_`: ImGui 官方封装的主窗口交换链、帧缓冲区和同步信号量集合。
 */
class ImGuiBackend
{
public:
    [[nodiscard]] static ImGuiBackend& instance();

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;
    ImGuiBackend(ImGuiBackend&&) = delete;
    ImGuiBackend& operator=(ImGuiBackend&&) = delete;

    /// @brief 直接通过 VulkanBackend 完成高层初始化（推荐，上层无需解构 window 与 context）
    bool initialize(const engine::VulkanBackend& backend);

    /// @brief 底层细粒度初始化接口（供自定义无物理后端测试场景使用）
    bool initialize(GLFWwindow* window, const engine::VulkanContext& context);

    bool begin_frame();
    void render(ImVec4 clear_color);
    void shutdown();

    [[nodiscard]] ImGuiIO& io() const;
    [[nodiscard]] const engine::VulkanContext* vulkan_context() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    ImGuiBackend();
    ~ImGuiBackend();

    bool ensure_swapchain();
    void render_platform_windows();

    const engine::VulkanContext* context_ = nullptr;     // 借用自 VulkanBackend，非拥有
    GLFWwindow* window_ = nullptr;                       // 借用自 VulkanBackend，非拥有
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;  // ImGui 专用字体与采样器描述符池
    ImGui_ImplVulkanH_Window window_data_{};             // 主交换链与呈现管线数据
    bool swapchain_rebuild_ = false;                     // 窗口尺寸变化时的交换链重建标志
    bool initialized_ = false;                           // 初始化完成标志
};

}
