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

/**
 * @brief 编辑器顶层 UI 界面外壳（Workbench Shell 协调器）
 * 
 * 职责与层级关系：
 * 1. 位于 Editor Shell 层，纯粹作为 Dear ImGui 2D 界面与停靠工作区（Docking Workspace）的总装宿主；
 * 2. 统一组织顶部主菜单栏（MenuBar）、全屏停靠区（DockSpace）、底部状态栏（StatusBar）与关于弹窗；
 * 3. 作为中央容器统一持有并协调四大子面板：
 *    - `Viewport`（3D 视口呈现）
 *    - `SceneHierarchyPanel`（场景实体层级树）
 *    - `InspectorPanel`（实体组件属性检查器）
 *    - `ConsolePanel`（系统与脚本控制台输出）
 * 4. 负责各子面板间的事件联动（如在 Hierarchy 点击实体或在 Viewport 点击拾取时同步双方选中态）；
 * 5. 架构解耦原则：Shell 本身不关心底层是 Vulkan 还是其他图形 API，对 3D 渲染的初始化应委托给 Viewport。
 */
class EditorUI
{
public:
    EditorUI() = default;
    ~EditorUI();

    [[nodiscard]] static EditorUI& instance();

    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;
    EditorUI(EditorUI&&) = delete;
    EditorUI& operator=(EditorUI&&) = delete;

    /// @brief 初始化 ImGui 全局配置（Docking、字体）并挂载指定主题
    void initialize(EditorSettings& settings);

    /// @brief 绑定当前正在编辑的场景（级联分发给 Viewport、Hierarchy 与 Inspector）
    void set_scene(engine::Scene* scene);

    /// @brief 便捷转发初始化视口（内部转发至 viewport().initialize），保留此接口向下兼容
    bool initialize_viewport(
        const engine::VulkanContext& context,
        const std::filesystem::path& shader_directory,
        engine::Scene& scene,
        std::string* error = nullptr);

    /// @brief 驱动单帧 UI 布局计算与绘制，返回背景清屏颜色
    ImVec4 render();

    /// @brief 安全关闭各面板并清理场景引用
    void shutdown();

    /// @brief 动态切换编辑器全局主题
    void set_theme(EditorTheme theme);

    /// @brief 全局选中指定实体（级联同步至 Viewport、Hierarchy 和 Inspector）
    void select_entity(std::optional<engine::EntityId> entity);

    [[nodiscard]] EditorTheme theme() const noexcept;
    [[nodiscard]] std::optional<engine::EntityId> selected_entity() const noexcept;

    // 四大核心面板统一暴露访问器（统一 Facade 模式）
    [[nodiscard]] Viewport& viewport() noexcept { return viewport_; }
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }
    [[nodiscard]] SceneHierarchyPanel& hierarchy_panel() noexcept { return hierarchy_panel_; }
    [[nodiscard]] const SceneHierarchyPanel& hierarchy_panel() const noexcept { return hierarchy_panel_; }
    [[nodiscard]] InspectorPanel& inspector_panel() noexcept { return inspector_panel_; }
    [[nodiscard]] const InspectorPanel& inspector_panel() const noexcept { return inspector_panel_; }
    [[nodiscard]] ConsolePanel& console_panel() noexcept { return console_panel_; }
    [[nodiscard]] const ConsolePanel& console_panel() const noexcept { return console_panel_; }

    // 以下为历史兼容便捷方法（内部均直接操作 viewport()）
    void set_viewport_camera_state(
        const glm::vec3& position,
        float yaw_degrees,
        float pitch_degrees) noexcept;
    void set_viewport_camera_speed(float speed) noexcept;
    void set_viewport_grid_visible(bool visible) noexcept;
    [[nodiscard]] const engine::PerspectiveCamera& viewport_camera() const noexcept;
    [[nodiscard]] float viewport_camera_speed() const noexcept;
    [[nodiscard]] bool is_viewport_grid_visible() const noexcept;

private:
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
