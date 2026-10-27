#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <imgui.h>

#include <domain/entity_id.h>

namespace slrye
{

class PerspectiveCamera;
class Scene;
struct VulkanContext;

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
