#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace slrye::engine
{

struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    std::uint32_t graphics_queue_family = 0;
    VkQueue present_queue = VK_NULL_HANDLE;
    std::uint32_t present_queue_family = 0;
    VkAllocationCallbacks* allocator = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    
    // VMA 统一内存分配器
    VmaAllocator vma_allocator = VK_NULL_HANDLE;

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