#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <engine/rhi/vma_resources.h>

namespace slrye::engine
{

struct VulkanContext;
class VulkanBackend;

/**
 * @brief 2D 纹理图像资源（Texture Asset）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Resources 资源层，代表已载入 GPU 显存并准备好在着色器中采样的 2D 贴图；
 * 2. 基于 stb_image 读取多种格式图片（PNG / JPEG / TGA / BMP / HDR），强制展开为 32 位 RGBA；
 * 3. 采用 VMA 分配统一显存（Staging Buffer -> Device Local AllocatedImage）；
 * 4. 内置线性过滤与边缘重复采样器（VkSampler），并自动向 ImGui 注册 ImTextureID，供 Inspector 面板直接预览缩略图。
 */
class Texture
{
public:
    Texture();
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;

    /// @brief 从本地图片文件路径加载并上传至 GPU
    [[nodiscard]] static std::shared_ptr<Texture> create_from_file(
        const VulkanContext& context,
        const std::filesystem::path& path,
        std::string* error = nullptr);

    [[nodiscard]] static std::shared_ptr<Texture> create_from_file(
        const VulkanBackend& backend,
        const std::filesystem::path& path,
        std::string* error = nullptr);

    /// @brief 从内存像素缓冲区（如嵌入式 glTF 纹理）加载并上传至 GPU
    [[nodiscard]] static std::shared_ptr<Texture> create_from_memory(
        const VulkanContext& context,
        const void* buffer,
        std::size_t size,
        std::string* error = nullptr);

    /// @brief 创建指定 RGBA 颜色的 1x1 纯色基础纹理（如纯白 BaseColor、平坦法线 (128, 128, 255) 等）
    [[nodiscard]] static std::shared_ptr<Texture> create_color(
        const VulkanContext& context,
        std::uint32_t rgba_color,
        std::string* error = nullptr);

    void destroy();

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t channels() const noexcept;
    [[nodiscard]] VkImage image() const noexcept;
    [[nodiscard]] VkImageView image_view() const noexcept;
    [[nodiscard]] VkSampler sampler() const noexcept;
    [[nodiscard]] ImTextureID texture_id() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static std::shared_ptr<Texture> create_from_pixels(
        const VulkanContext& context,
        const void* pixel_data,
        std::uint32_t width,
        std::uint32_t height,
        const std::filesystem::path& path,
        std::string* error);
};

}

namespace slrye
{
using engine::Texture;
}
