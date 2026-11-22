#include <editor/shell/editor_ui.h>

#include <imgui_internal.h>

#include <algorithm>

namespace slrye
{

EditorUI& EditorUI::instance()
{
    static EditorUI editor_ui;
    return editor_ui;
}

EditorUI::~EditorUI()
{
    shutdown();
}

void EditorUI::initialize(EditorSettings& settings)
{
    settings_ = &settings;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    if (io.Fonts->Fonts.Size == 0)
    {
        io.Fonts->AddFontDefault();
    }

    theme_manager_.apply(settings.theme);
}

void EditorUI::set_scene(Scene* scene)
{
    scene_ = scene;
    selected_entity_.reset();
    if (scene_ != nullptr)
    {
        viewport_.set_scene(*scene_);
    }
    hierarchy_panel_.set_scene(scene_);
    inspector_panel_.set_scene(scene_);
}

bool EditorUI::initialize_viewport(
    const VulkanContext& context,
    const std::filesystem::path& shader_directory,
    Scene& scene,
    std::string* error)
{
    set_scene(&scene);
    return viewport_.initialize(context, shader_directory, scene, error);
}

void EditorUI::select_entity(std::optional<engine::EntityId> entity)
{
    selected_entity_ = entity;
    viewport_.set_selected_entity(entity);
    hierarchy_panel_.set_selected_entity(entity);
    inspector_panel_.set_selected_entity(entity);
}

ImVec4 EditorUI::render()
{
    draw_workspace();
    draw_about_window();

    // 双向同步 Viewport 射线拾取与 Hierarchy 点击选中
    if (viewport_.selected_entity() != selected_entity_)
    {
        select_entity(viewport_.selected_entity());
    }
    else if (hierarchy_panel_.selected_entity() != selected_entity_)
    {
        select_entity(hierarchy_panel_.selected_entity());
    }

    viewport_.render();
    hierarchy_panel_.render();
    inspector_panel_.render();
    console_panel_.render();

    return slrye::render(view_state_);
}

void EditorUI::shutdown()
{
    viewport_.shutdown();
    settings_ = nullptr;
    scene_ = nullptr;
    selected_entity_.reset();
}

void EditorUI::set_theme(EditorTheme theme)
{
    if (settings_ != nullptr)
    {
        settings_->theme = theme;
    }
    theme_manager_.apply(theme);
}

EditorTheme EditorUI::theme() const noexcept
{
    return settings_ != nullptr ? settings_->theme : EditorTheme::SpectrumLight;
}

void EditorUI::set_viewport_camera_state(
    const glm::vec3& position,
    float yaw_degrees,
    float pitch_degrees) noexcept
{
    viewport_.set_camera_state(position, yaw_degrees, pitch_degrees);
}

void EditorUI::set_viewport_camera_speed(float speed) noexcept
{
    viewport_.set_camera_speed(speed);
}

void EditorUI::set_viewport_grid_visible(bool visible) noexcept
{
    viewport_.set_grid_visible(visible);
}

const engine::PerspectiveCamera& EditorUI::viewport_camera() const noexcept
{
    return viewport_.camera();
}

std::optional<engine::EntityId> EditorUI::selected_entity() const noexcept
{
    return selected_entity_;
}

float EditorUI::viewport_camera_speed() const noexcept
{
    return viewport_.camera_speed();
}

bool EditorUI::is_viewport_grid_visible() const noexcept
{
    return viewport_.is_grid_visible();
}

void EditorUI::draw_menu_bar()
{
    if (!ImGui::BeginMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("New Project", "Ctrl+N", false, false);
        ImGui::MenuItem("Open Project...", "Ctrl+O", false, false);
        ImGui::MenuItem("Save", "Ctrl+S", false, false);
        ImGui::Separator();
        ImGui::MenuItem("Exit", "Alt+F4", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
        ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        const EditorTheme current_theme = theme();
        if (ImGui::BeginMenu("Theme"))
        {
            if (ImGui::MenuItem(
                    "Spectrum Light",
                    nullptr,
                    current_theme == EditorTheme::SpectrumLight))
            {
                set_theme(EditorTheme::SpectrumLight);
            }
            if (ImGui::MenuItem(
                    "Spectrum Dark",
                    nullptr,
                    current_theme == EditorTheme::SpectrumDark))
            {
                set_theme(EditorTheme::SpectrumDark);
            }
            if (ImGui::MenuItem(
                    "EnemyMouse",
                    nullptr,
                    current_theme == EditorTheme::EnemyMouse))
            {
                set_theme(EditorTheme::EnemyMouse);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Viewport", nullptr, viewport_.is_visible()))
        {
            viewport_.set_visible(!viewport_.is_visible());
        }
        if (ImGui::MenuItem("Scene Hierarchy", nullptr, hierarchy_panel_.is_visible()))
        {
            hierarchy_panel_.set_visible(!hierarchy_panel_.is_visible());
        }
        if (ImGui::MenuItem("Inspector", nullptr, inspector_panel_.is_visible()))
        {
            inspector_panel_.set_visible(!inspector_panel_.is_visible());
        }
        if (ImGui::MenuItem("Console", nullptr, console_panel_.is_visible()))
        {
            console_panel_.set_visible(!console_panel_.is_visible());
        }
        ImGui::Separator();
        ImGui::MenuItem("Editor Preview", nullptr, &view_state_.show_demo_window);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About Slrye"))
        {
            show_about_window_ = true;
        }
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void EditorUI::draw_workspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##EditorWorkspace", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    draw_menu_bar();

    const float status_height = ImGui::GetFrameHeight() + 4.0f;
    const ImVec2 workspace_cursor = ImGui::GetCursorPos();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 dockspace_size(
        std::max(available.x, 1.0f),
        std::max(available.y - status_height, 1.0f));

    const ImGuiID dockspace_id = ImGui::GetID("SlryeDockSpace");
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, dockspace_size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.22f, nullptr, &dock_main_id);
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.26f, nullptr, &dock_main_id);
        ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.26f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Console", dock_down_id);
        ImGui::DockBuilderDockWindow("SLRYE", dock_down_id);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, dockspace_size, ImGuiDockNodeFlags_None);
    ImGui::SetCursorPos(ImVec2(workspace_cursor.x, workspace_cursor.y + dockspace_size.y));
    draw_status_bar(status_height);
    ImGui::End();
}

void EditorUI::draw_status_bar(float height)
{
    ImGui::BeginChild(
        "##StatusBar",
        ImVec2(0.0f, height),
        ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::TextUnformatted("Ready");
    ImGui::SameLine();
    ImGui::TextDisabled(
        "| %s | %.1f FPS",
        theme_manager_.display_name(theme()),
        ImGui::GetIO().Framerate);

    ImGui::EndChild();
}

void EditorUI::draw_about_window()
{
    if (!show_about_window_)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("About Slrye", &show_about_window_))
    {
        ImGui::TextUnformatted("Slrye Engine Editor");
        ImGui::Separator();
        ImGui::TextUnformatted("Version 0.1.0");
        ImGui::TextUnformatted("C++20, Vulkan, GLFW, Dear ImGui");
    }
    ImGui::End();
}

}
