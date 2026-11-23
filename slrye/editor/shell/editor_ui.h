#pragma once

#include <imgui.h>

#include <filesystem>
#include <optional>
#include <string>

#include <engine/renderer/camera.h>
#include <editor/shell/editor_settings.h>
#include <engine/scene/entity_id.h>
#include <editor/shell/theme_manager.h>
#include <editor/panels/console_panel.h>
#include <editor/panels/inspector_panel.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <editor/panels/view.h>
#include <editor/panels/viewport.h>

namespace slrye::engine
{

class Scene;
struct VulkanContext;

}

namespace slrye {
class EditorUI
{
public:
    [[nodiscard]] static EditorUI& instance();

    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;
    EditorUI(EditorUI&&) = delete;
    EditorUI& operator=(EditorUI&&) = delete;

    void initialize(EditorSettings& settings);
    void set_scene(engine::Scene* scene);
    bool initialize_viewport(
        const engine::VulkanContext& context,
        const std::filesystem::path& shader_directory,
        engine::Scene& scene,
        std::string* error = nullptr);
    ImVec4 render();
    void shutdown();
    void set_theme(EditorTheme theme);
    void set_viewport_camera_state(
        const glm::vec3& position,
        float yaw_degrees,
        float pitch_degrees) noexcept;
    void set_viewport_camera_speed(float speed) noexcept;
    void set_viewport_grid_visible(bool visible) noexcept;

    void select_entity(std::optional<engine::EntityId> entity);

    [[nodiscard]] EditorTheme theme() const noexcept;
    [[nodiscard]] const engine::PerspectiveCamera& viewport_camera() const noexcept;
    [[nodiscard]] std::optional<engine::EntityId> selected_entity() const noexcept;
    [[nodiscard]] float viewport_camera_speed() const noexcept;
    [[nodiscard]] bool is_viewport_grid_visible() const noexcept;

    [[nodiscard]] SceneHierarchyPanel& hierarchy_panel() noexcept { return hierarchy_panel_; }
    [[nodiscard]] InspectorPanel& inspector_panel() noexcept { return inspector_panel_; }
    [[nodiscard]] ConsolePanel& console_panel() noexcept { return console_panel_; }

private:
    EditorUI() = default;
    ~EditorUI();

    void draw_menu_bar();
    void draw_workspace();
    void draw_status_bar(float height);
    void draw_about_window();

    ThemeManager theme_manager_;
    EditorSettings* settings_ = nullptr;
    EditorViewState view_state_;
    engine::Scene* scene_ = nullptr;
    std::optional<engine::EntityId> selected_entity_;

    Viewport viewport_;
    SceneHierarchyPanel hierarchy_panel_;
    InspectorPanel inspector_panel_;
    ConsolePanel console_panel_;

    bool show_about_window_ = false;
};

}
