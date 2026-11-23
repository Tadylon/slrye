#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <imgui.h>

#include <engine/scene/entity_id.h>

namespace slrye::engine
{

class PerspectiveCamera;
class Scene;
struct VulkanContext;
class VulkanBackend;

/**
 * @brief 视口 3D 场景离屏渲染器
 * 
 * 职责与层级关系：
 * 1. 位于 Renderer 层，负责将 Scene 中的网格、选中高亮线框、网格基准地面（Grid）
 *    离屏渲染至一张独立的 Vulkan 颜色纹理（AllocatedImage）；
 * 2. 内部采用 PImpl 模式（Impl），隔离 Vulkan 管线、渲染通道与 VMA 显存分配细节；
 * 3. 构造与析构函数为 public，支持由 Viewport 面板或其他渲染持有者直接作为成员持有。
 */
class SceneRenderer
{
public:
    SceneRenderer();
    ~SceneRenderer();

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    SceneRenderer(SceneRenderer&&) = delete;
    SceneRenderer& operator=(SceneRenderer&&) = delete;

    bool initialize(
        const VulkanContext& context,
        const std::filesystem::path& shader_directory,
        std::string* error = nullptr);
    bool initialize(
        const VulkanBackend& backend,
        const std::filesystem::path& shader_directory,
        std::string* error = nullptr);
    bool render(
        const Scene& scene,
        const PerspectiveCamera& camera,
        std::uint32_t width,
        std::uint32_t height,
        std::optional<EntityId> selected_entity = std::nullopt,
        bool show_grid = true,
        std::string* error = nullptr);
    void shutdown();

    [[nodiscard]] ImTextureID texture_id() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}

namespace slrye
{
using engine::SceneRenderer;
}
