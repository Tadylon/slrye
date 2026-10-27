#pragma once

#include <imgui.h>

#include <filesystem>
#include <optional>
#include <string>

#include <domain/camera.h>
#include <domain/entity_id.h>
#include <ui/theme_manager.h>
#include <views/view.h>
#include <views/viewport.h>

namespace slrye
{

class EditorUI
{
public:
    void initialize();
    bool initialize_viewport(
        const VulkanContext& context,
        const std::filesystem::path& shader_directory,
        Scene& scene,
        std::string* error = nullptr);
    ImVec4 render(ImGuiIO& io);
    void shutdown();
    void set_theme(EditorTheme theme);
    void set_viewport_camera_state(
        const glm::vec3& position,
        float yaw_degrees,
        float pitch_degrees) noexcept;
    void set_viewport_camera_speed(float speed) noexcept;
    void set_viewport_grid_visible(bool visible) noexcept;

    [[nodiscard]] EditorTheme theme() const noexcept;
    [[nodiscard]] const PerspectiveCamera& viewport_camera() const noexcept;
    [[nodiscard]] std::optional<EntityId> selected_entity() const noexcept;
    [[nodiscard]] float viewport_camera_speed() const noexcept;
    [[nodiscard]] bool is_viewport_grid_visible() const noexcept;

private:
    void draw_menu_bar();
    void draw_workspace();
    void draw_status_bar(float height);
    void draw_about_window();

    ThemeManager theme_manager_;
    EditorTheme theme_ = EditorTheme::SpectrumLight;
    EditorViewState view_state_;
    Viewport viewport_;
    bool show_about_window_ = false;
};

}
