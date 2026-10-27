#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace slrye
{

struct VulkanContext
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    std::uint32_t graphics_queue_family = 0;
    VkDescriptorPool imgui_descriptor_pool = VK_NULL_HANDLE;
    VkAllocationCallbacks* allocator = nullptr;

    [[nodiscard]] bool is_valid() const noexcept
    {
        return instance != VK_NULL_HANDLE &&
            physical_device != VK_NULL_HANDLE &&
            device != VK_NULL_HANDLE &&
            graphics_queue != VK_NULL_HANDLE &&
            imgui_descriptor_pool != VK_NULL_HANDLE;
    }
};

}
