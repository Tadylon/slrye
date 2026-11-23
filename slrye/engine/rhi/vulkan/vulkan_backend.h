#pragma once

#include <cstdint>
#include <memory>
#include <engine/rhi/vulkan/vulkan_context.h>
#include <engine/rhi/vma_resources.h>

struct GLFWwindow;

namespace slrye::engine
{

/**
 * @brief RHI 平台与 Vulkan 硬件宿主（资源独占所有者 Owner）
 * 
 * 职责与层级关系：
 * 1. 物理所有权（Ownership）：
 *    - 全局独占创建并持有唯一的操作系统物理窗口（GLFWwindow*）；
 *    - 通过 vk-bootstrap 独占创建并持有 Vulkan Instance、PhysicalDevice、Logical Device、Surface；
 *    - 独占创建并持有 VmaAllocator 内存池；
 * 2. 向上暴露：
 *    - 导出非拥有型 `VulkanContext` 视图供渲染服务（如 SceneRenderer）与 UI 中间件（ImGuiBackend）借用；
 *    - 导出受控的物理窗口句柄 `window()` 供输入桥接库使用；
 *    - 提供基于 VMA 的显存缓冲区与纹理创建辅助方法（`create_buffer`, `create_image`）；
 * 3. 生命周期模式：
 *    - 支持显式对象构造与析构（RAII，所有权归 Application/EngineContext 持有）；
 *    - 同时保留静态 `instance()` 接口以向下兼容已有模块。
 */
class VulkanBackend
{
public:
    VulkanBackend();
    ~VulkanBackend();

    VulkanBackend(const VulkanBackend&) = delete;
    VulkanBackend& operator=(const VulkanBackend&) = delete;
    VulkanBackend(VulkanBackend&&) = delete;
    VulkanBackend& operator=(VulkanBackend&&) = delete;

    /// @brief 全局单例兼容访问点（若不采用显式对象持有则可通过此接口访问）
    [[nodiscard]] static VulkanBackend& instance();

    /// @brief 初始化 GLFW 窗口、Vulkan 设备与 VMA 分配器
    bool initialize(const char* title, int width, int height);

    /// @brief 检查物理窗口是否收到了关闭指令
    bool should_close() const;

    /// @brief 开始新的一帧：轮询 GLFW OS 事件
    bool begin_frame();

    /// @brief 释放所有 Vulkan 设备资源并关闭 GLFW 窗口
    void shutdown();

    /// @brief 阻塞等待 GPU 队列空闲（在调整窗口尺寸或安全析构时调用）
    void wait_idle() const;

    /// @brief 获取受控的底层 GLFW 物理窗口指针（仅供输入桥接如 ImGui 使用）
    [[nodiscard]] GLFWwindow* window() const noexcept;

    /// @brief 获取非拥有型的 Vulkan 核心句柄切片视图
    [[nodiscard]] const VulkanContext& context() const noexcept;

    /// @brief 基于底层 VmaAllocator 分配统一托管的 GPU 缓冲区
    [[nodiscard]] AllocatedBuffer create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlags flags = 0) const;

    /// @brief 基于底层 VmaAllocator 分配统一托管的 GPU 2D 纹理/图像
    [[nodiscard]] AllocatedImage create_image(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspect) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;  // PImpl 私有隐藏 vk-bootstrap 的内部结构体

    VulkanContext context_;       // 统一维护并对外借出的非拥有型句柄切片
    GLFWwindow* window_ = nullptr;// 物理系统窗口（此指针独占所有权）
    bool glfw_initialized_ = false;// GLFW 库全局初始化标志
    bool initialized_ = false;     // Vulkan 硬件环境初始化完成标志
};

}

namespace slrye
{
using engine::VulkanBackend;
}
