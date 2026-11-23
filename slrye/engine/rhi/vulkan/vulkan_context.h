#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace slrye::engine
{

/**
 * @brief Vulkan 核心硬件句柄上下文视图（非拥有型 Borrowed View）
 * 
 * 设计意图与层级定位：
 * - 本结构体【不拥有任何硬件对象的生命周期】，纯粹充当各子系统借用 Vulkan 原生句柄的切片容器（POD 聚合）。
 * - 物理生命周期由 VulkanBackend 独占拥有。
 * - 作用：解耦！使 SceneRenderer、ImGuiBackend、计算着色器管线以及单元测试无需依赖复杂的
 *   VulkanBackend 物理窗口驱动，只需借用此上下文即可完成资源创建、指令缓冲录制与队列提交。
 */
struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;               ///< Vulkan 驱动与应用实例
    VkPhysicalDevice physical_device = VK_NULL_HANDLE; ///< 选中的物理 GPU 硬件句柄
    VkDevice device = VK_NULL_HANDLE;                   ///< 逻辑设备通信上下文
    VkQueue graphics_queue = VK_NULL_HANDLE;            ///< 图形绘制指令队列
    std::uint32_t graphics_queue_family = 0;            ///< 图形队列族索引
    VkQueue present_queue = VK_NULL_HANDLE;             ///< 屏幕呈现指令队列（可与 graphics_queue 共享）
    std::uint32_t present_queue_family = 0;             ///< 呈现队列族索引
    VkAllocationCallbacks* allocator = nullptr;         ///< 自定义内存分配回调（通常为 nullptr）
    VkSurfaceKHR surface = VK_NULL_HANDLE;              ///< 物理窗口绘制表面
    VmaAllocator vma_allocator = VK_NULL_HANDLE;        ///< VMA 统一显存池化分配器

    /// @brief 检查上下文是否具备完整可用的基础 Vulkan 硬件环境
    [[nodiscard]] bool is_valid() const noexcept
    {
        return instance != VK_NULL_HANDLE &&
            physical_device != VK_NULL_HANDLE &&
            device != VK_NULL_HANDLE &&
            graphics_queue != VK_NULL_HANDLE &&
            surface != VK_NULL_HANDLE &&
            vma_allocator != VK_NULL_HANDLE;
    }
};

}

namespace slrye
{
using engine::VulkanContext;
}