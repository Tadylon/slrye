#include <ui/editor_ui.h>

#include <imgui_internal.h>

#include <algorithm>

namespace slrye
{

void EditorUI::initialize()
{
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

    set_theme(theme_);
}

ImVec4 EditorUI::render(ImGuiIO& io)
{
    draw_workspace();
    draw_about_window();
    viewport_.render(io);
    return slrye::render(io, view_state_);
}

bool EditorUI::initialize_viewport(
    const VulkanContext& context,
    const std::filesystem::path& shader_directory,
    Scene& scene,
    std::string* error)
{
    return viewport_.initialize(context, shader_directory, scene, error);
}

void EditorUI::shutdown()
{
    viewport_.shutdown();
}

void EditorUI::set_theme(EditorTheme theme)
{
    theme_ = theme;
    theme_manager_.apply(theme_);
}

EditorTheme EditorUI::theme() const noexcept
{
    return theme_;
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

const PerspectiveCamera& EditorUI::viewport_camera() const noexcept
{
    return viewport_.camera();
}

std::optional<EntityId> EditorUI::selected_entity() const noexcept
{
    return viewport_.selected_entity();
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
        if (ImGui::BeginMenu("Theme"))
        {
            if (ImGui::MenuItem("Spectrum Light", nullptr, theme_ == EditorTheme::SpectrumLight))
            {
                set_theme(EditorTheme::SpectrumLight);
            }
            if (ImGui::MenuItem("Spectrum Dark", nullptr, theme_ == EditorTheme::SpectrumDark))
            {
                set_theme(EditorTheme::SpectrumDark);
            }
            if (ImGui::MenuItem("EnemyMouse", nullptr, theme_ == EditorTheme::EnemyMouse))
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
        ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
        ImGui::DockBuilderDockWindow("SLRYE", dockspace_id);
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
        theme_manager_.display_name(theme_),
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
