#include <engine/resources/texture.h>

#include <functional>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <utility>

#include <imgui_impl_vulkan.h>

#include <engine/rhi/vulkan/vulkan_context.h>
#include <engine/rhi/vulkan/vulkan_backend.h>

namespace slrye::engine
{

namespace
{

bool execute_single_time_commands(
    const VulkanContext& context,
    const std::function<void(VkCommandBuffer)>& record_func)
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = context.graphics_queue_family;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(context.device, &pool_info, context.allocator, &command_pool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(context.device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        vkDestroyCommandPool(context.device, command_pool, context.allocator);
        return false;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    record_func(command_buffer);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphics_queue);

    vkFreeCommandBuffers(context.device, command_pool, 1, &command_buffer);
    vkDestroyCommandPool(context.device, command_pool, context.allocator);
    return true;
}

void transition_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        command_buffer,
        source_stage,
        destination_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

}

struct Texture::Impl
{
    VulkanContext context{};
    AllocatedImage image{};
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet texture_descriptor = VK_NULL_HANDLE;
    std::filesystem::path path;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 4;
};

Texture::Texture()
    : impl_(std::make_unique<Impl>())
{
}

Texture::~Texture()
{
    destroy();
}

Texture::Texture(Texture&&) noexcept = default;
Texture& Texture::operator=(Texture&&) noexcept = default;

void Texture::destroy()
{
    if (!impl_)
    {
        return;
    }

    if (impl_->texture_descriptor != VK_NULL_HANDLE)
    {
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplVulkan_RemoveTexture(impl_->texture_descriptor);
        }
        impl_->texture_descriptor = VK_NULL_HANDLE;
    }

    if (impl_->sampler != VK_NULL_HANDLE && impl_->context.device != VK_NULL_HANDLE)
    {
        vkDestroySampler(impl_->context.device, impl_->sampler, impl_->context.allocator);
        impl_->sampler = VK_NULL_HANDLE;
    }

    if (impl_->image.is_valid())
    {
        impl_->image.destroy(impl_->context.vma_allocator, impl_->context.device);
    }

    impl_->width = 0;
    impl_->height = 0;
    impl_->context = {};
}

std::shared_ptr<Texture> Texture::create_from_pixels(
    const VulkanContext& context,
    const void* pixel_data,
    std::uint32_t width,
    std::uint32_t height,
    const std::filesystem::path& path,
    std::string* error)
{
    if (!context.is_valid() || pixel_data == nullptr || width == 0 || height == 0)
    {
        if (error != nullptr)
        {
            *error = "Invalid parameters for texture creation";
        }
        return nullptr;
    }

    const VkDeviceSize image_size = static_cast<VkDeviceSize>(width) * height * 4;

    // 1. 创建 CPU 暂存缓冲区 (Staging Buffer)
    AllocatedBuffer staging_buffer = AllocatedBuffer::create(
        context.vma_allocator,
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    if (!staging_buffer.is_valid())
    {
        if (error != nullptr)
        {
            *error = "Failed to allocate texture staging buffer";
        }
        return nullptr;
    }

    void* mapped = staging_buffer.map(context.vma_allocator);
    if (mapped != nullptr)
    {
        std::memcpy(mapped, pixel_data, static_cast<std::size_t>(image_size));
        staging_buffer.unmap(context.vma_allocator);
    }
    else
    {
        staging_buffer.destroy(context.vma_allocator);
        if (error != nullptr)
        {
            *error = "Failed to map texture staging buffer memory";
        }
        return nullptr;
    }

    // 2. 创建 GPU 设备独占纹理 (AllocatedImage)
    AllocatedImage texture_image = AllocatedImage::create_2d(
        context.vma_allocator,
        context.device,
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    if (!texture_image.is_valid())
    {
        staging_buffer.destroy(context.vma_allocator);
        if (error != nullptr)
        {
            *error = "Failed to create GPU texture image via VMA";
        }
        return nullptr;
    }

    // 3. 执行单次传输指令
    const bool copy_success = execute_single_time_commands(
        context,
        [&](VkCommandBuffer cmd)
        {
            transition_image_layout(
                cmd,
                texture_image.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, height, 1};

            vkCmdCopyBufferToImage(
                cmd,
                staging_buffer.buffer,
                texture_image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region);

            transition_image_layout(
                cmd,
                texture_image.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

    staging_buffer.destroy(context.vma_allocator);

    if (!copy_success)
    {
        texture_image.destroy(context.vma_allocator, context.device);
        if (error != nullptr)
        {
            *error = "Failed to record and submit texture copy commands";
        }
        return nullptr;
    }

    // 4. 创建默认双线性采样器 (VkSampler)
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(context.device, &sampler_info, context.allocator, &sampler) != VK_SUCCESS)
    {
        texture_image.destroy(context.vma_allocator, context.device);
        if (error != nullptr)
        {
            *error = "Failed to create Vulkan texture sampler";
        }
        return nullptr;
    }

    // 5. 注册 ImGui 描述符
    VkDescriptorSet descriptor = ImGui_ImplVulkan_AddTexture(
        sampler,
        texture_image.image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    auto result = std::make_shared<Texture>();
    result->impl_->context = context;
    result->impl_->image = texture_image;
    result->impl_->sampler = sampler;
    result->impl_->texture_descriptor = descriptor;
    result->impl_->path = path;
    result->impl_->width = width;
    result->impl_->height = height;
    result->impl_->channels = 4;
    return result;
}

std::shared_ptr<Texture> Texture::create_from_file(
    const VulkanContext& context,
    const std::filesystem::path& path,
    std::string* error)
{
    int w = 0, h = 0, c = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &w, &h, &c, STBI_rgb_alpha);
    if (pixels == nullptr)
    {
        if (error != nullptr)
        {
            *error = "stb_image failed to load '" + path.string() + "': " + std::string(stbi_failure_reason());
        }
        return nullptr;
    }

    auto texture = create_from_pixels(
        context,
        pixels,
        static_cast<std::uint32_t>(w),
        static_cast<std::uint32_t>(h),
        path,
        error);

    stbi_image_free(pixels);
    return texture;
}

std::shared_ptr<Texture> Texture::create_from_file(
    const VulkanBackend& backend,
    const std::filesystem::path& path,
    std::string* error)
{
    return create_from_file(backend.context(), path, error);
}

std::shared_ptr<Texture> Texture::create_from_memory(
    const VulkanContext& context,
    const void* buffer,
    std::size_t size,
    std::string* error)
{
    if (buffer == nullptr || size == 0)
    {
        if (error != nullptr)
        {
            *error = "Invalid buffer memory for texture decoding";
        }
        return nullptr;
    }

    int w = 0, h = 0, c = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(buffer),
        static_cast<int>(size),
        &w, &h, &c,
        STBI_rgb_alpha);

    if (pixels == nullptr)
    {
        if (error != nullptr)
        {
            *error = "stb_image failed to decode from memory: " + std::string(stbi_failure_reason());
        }
        return nullptr;
    }

    auto texture = create_from_pixels(
        context,
        pixels,
        static_cast<std::uint32_t>(w),
        static_cast<std::uint32_t>(h),
        "embedded://memory",
        error);

    stbi_image_free(pixels);
    return texture;
}

std::shared_ptr<Texture> Texture::create_color(
    const VulkanContext& context,
    std::uint32_t rgba_color,
    std::string* error)
{
    return create_from_pixels(
        context,
        &rgba_color,
        1,
        1,
        "solid://color",
        error);
}

bool Texture::is_valid() const noexcept
{
    return impl_ && impl_->image.is_valid() && impl_->sampler != VK_NULL_HANDLE;
}

std::uint32_t Texture::width() const noexcept
{
    return impl_ ? impl_->width : 0;
}

std::uint32_t Texture::height() const noexcept
{
    return impl_ ? impl_->height : 0;
}

std::uint32_t Texture::channels() const noexcept
{
    return impl_ ? impl_->channels : 0;
}

VkImage Texture::image() const noexcept
{
    return impl_ ? impl_->image.image : VK_NULL_HANDLE;
}

VkImageView Texture::image_view() const noexcept
{
    return impl_ ? impl_->image.image_view : VK_NULL_HANDLE;
}

VkSampler Texture::sampler() const noexcept
{
    return impl_ ? impl_->sampler : VK_NULL_HANDLE;
}

ImTextureID Texture::texture_id() const noexcept
{
    return impl_ ? static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(impl_->texture_descriptor)) : ImTextureID_Invalid;
}

const std::filesystem::path& Texture::path() const noexcept
{
    static const std::filesystem::path empty_path;
    return impl_ ? impl_->path : empty_path;
}

}
