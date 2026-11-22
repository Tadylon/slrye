#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace slrye
{

struct AllocatedBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info{};

    static AllocatedBuffer create(
        VmaAllocator allocator,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage,
        VmaAllocationCreateFlags flags = 0)
    {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = memory_usage;
        alloc_info.flags = flags;

        AllocatedBuffer res;
        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &res.buffer, &res.allocation, &res.info);
        return res;
    }

    void destroy(VmaAllocator allocator)
    {
        if (buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator, buffer, allocation);
            buffer = VK_NULL_HANDLE;
            allocation = VK_NULL_HANDLE;
        }
    }
};

struct AllocatedImage
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent2D extent{0, 0};
    VkFormat format = VK_FORMAT_UNDEFINED;

    static AllocatedImage create_2d(
        VmaAllocator allocator,
        VkDevice device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspect)
    {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent = {width, height, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        AllocatedImage res;
        res.extent = {width, height};
        res.format = format;
        vmaCreateImage(allocator, &image_info, &alloc_info, &res.image, &res.allocation, nullptr);

        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = res.image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = aspect;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &view_info, nullptr, &res.image_view);
        return res;
    }

    void destroy(VmaAllocator allocator, VkDevice device)
    {
        if (image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, image_view, nullptr);
            image_view = VK_NULL_HANDLE;
        }
        if (image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(allocator, image, allocation);
            image = VK_NULL_HANDLE;
            allocation = VK_NULL_HANDLE;
        }
    }
};

}