#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <imgui.h>

#include <domain/camera.h>
#include <domain/entity_id.h>
#include <services/rendering/scene_renderer.h>
#include <views/camera_controller.h>

namespace slrye
{

class Scene;
struct VulkanContext;

class Viewport
{
public:
    bool initialize(
        const VulkanContext& context,
        const std::filesystem::path& shader_directory,
        Scene& scene,
        std::string* error = nullptr);
    void render(ImGuiIO& io);
    void shutdown();

    void set_scene(Scene& scene) noexcept;
    void set_camera_state(
        const glm::vec3& position,
        float yaw_degrees,
        float pitch_degrees) noexcept;
    void set_selected_entity(std::optional<EntityId> entity) noexcept;
    void set_visible(bool visible) noexcept;
    void set_camera_speed(float speed) noexcept;
    void set_grid_visible(bool visible) noexcept;

    [[nodiscard]] PerspectiveCamera& camera() noexcept;
    [[nodiscard]] const PerspectiveCamera& camera() const noexcept;
    [[nodiscard]] std::optional<EntityId> selected_entity() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] bool is_visible() const noexcept;
    [[nodiscard]] float camera_speed() const noexcept;
    [[nodiscard]] bool is_grid_visible() const noexcept;

private:
    void draw_toolbar();
    void process_input(ImGuiIO& io, const ImVec2& image_position, const ImVec2& image_size);
    void focus_selected();
    void request_redraw() noexcept;

    SceneRenderer renderer_;
    CameraController controller_;
    PerspectiveCamera camera_;
    Scene* scene_ = nullptr;
    std::optional<EntityId> selected_entity_;
    std::uint64_t rendered_scene_revision_ = 0;
    std::uint32_t rendered_width_ = 0;
    std::uint32_t rendered_height_ = 0;
    std::string last_error_;
    bool initialized_ = false;
    bool camera_dirty_ = true;
    bool rotating_ = false;
    bool visible_ = true;
    bool show_grid_ = true;
};

}
