#include <editor/app/application.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include <editor/backend/imgui_backend.h>
#include <editor/shell/editor_ui.h>
#include <editor/shell/editor_settings.h>


#include <engine/rhi/vulkan/vulkan_backend.h>
#include <engine/renderer/camera.h>
#include <engine/scene/scene.h>
#include <engine/scene/entity.h>
#include <engine/scene/components.h>
#include <engine/platform/paths.h>
#include <engine/services/project/project_file.h>
#include <engine/services/serialization/scene_serializer.h>

namespace slrye
{

Application& Application::instance(int argc, char** argv)
{
    static Application application(argc, argv);
    return application;
}

Application::Application(int argc, char** argv)
    : launch_options_(parse_launch_options(argc, argv)),
      active_scene_(std::make_unique<engine::Scene>("Untitled")),
    //   plugin_manager_(PluginManager::instance()),
      backend_(engine::VulkanBackend::instance()),
      imgui_backend_(ImGuiBackend::instance()),
      ui_(EditorUI::instance())
{
    // plugin_manager_.set_logger([](const std::string& message)
    // {
    //     fprintf(stderr, "[plugin] %s\n", message.c_str());
    // });
}

Application::~Application() = default;

int Application::run()
{
    if (!launch_options_.is_valid())
    {
        fprintf(stderr, "Startup error: %s\n", launch_options_.error.c_str());
        return 2;
    }

    load_editor_preferences();
    if (launch_options_.project_path.has_value())
    {
        std::string project_error;
        if (!load_startup_project(project_error))
        {
            fprintf(stderr, "Project error: %s\n", project_error.c_str());
            save_editor_preferences();
            return 2;
        }
    }
    else if (active_scene_->entity_count() == 0)
    {
        engine::Entity cube = active_scene_->create_entity("Cube");
        cube.get<engine::TransformComponent>().position = glm::vec3(0.0f, 0.5f, 0.0f);
        cube.add<engine::ColliderComponent>();

        engine::Entity light = active_scene_->create_entity("Directional Light");
        light.get<engine::TransformComponent>().position = glm::vec3(5.0f, 10.0f, 5.0f);
        auto& lc = light.add<engine::LightComponent>();
        lc.type = engine::LightType::Directional;

        engine::Entity camera = active_scene_->create_entity("Main Camera");
        camera.get<engine::TransformComponent>().position = glm::vec3(0.0f, 2.0f, 6.0f);
        camera.add<engine::CameraComponent>();
    }

    if (!backend_.initialize("Slrye", 1280, 800))
    {
        save_editor_preferences();
        return 1;
    }

    if (!imgui_backend_.initialize(backend_))
    {
        fprintf(stderr, "Unable to initialize the Dear ImGui Vulkan backend\n");
        save_editor_preferences();
        backend_.shutdown();
        return 1;
    }

    // plugin_manager_.load_all(platform::plugins_directory(launch_options_.executable_path));

    ui_.initialize(settings_);
    ui_initialized_ = true;
    ui_.set_viewport_camera_state(
        settings_.camera_position,
        settings_.camera_yaw_degrees,
        settings_.camera_pitch_degrees);
    ui_.set_viewport_camera_speed(settings_.camera_move_speed);
    ui_.set_viewport_grid_visible(settings_.show_viewport_grid);

    std::string viewport_error;
    if (!ui_.initialize_viewport(
            backend_.context(),
            assets_directory(launch_options_.executable_path) / "shaders",
            *active_scene_,
            &viewport_error))
    {
        fprintf(stderr, "Viewport warning: %s\n", viewport_error.c_str());
    }

    while (!backend_.should_close())
    {
        if (!backend_.begin_frame() || !imgui_backend_.begin_frame())
        {
            continue;
        }

        const ImVec4 clear_color = ui_.render();
        imgui_backend_.render(clear_color);
    }

    save_editor_preferences();
    ui_.shutdown();
    ui_initialized_ = false;
    // plugin_manager_.shutdown();
    imgui_backend_.shutdown();
    backend_.shutdown();
    return 0;
}

void Application::load_editor_preferences()
{
    settings_path_ = user_config_directory() / "editor.yaml";
    std::error_code filesystem_error;
    if (!std::filesystem::exists(settings_path_, filesystem_error))
    {
        if (filesystem_error)
        {
            fprintf(stderr, "Settings warning: %s\n", filesystem_error.message().c_str());
        }
        recent_projects_.replace(settings_.recent_projects);
        return;
    }

    std::string error;
    std::optional<EditorSettings> loaded = load_editor_settings(settings_path_, &error);
    if (!loaded.has_value())
    {
        fprintf(stderr, "Settings warning: %s\n", error.c_str());
        recent_projects_.replace(settings_.recent_projects);
        return;
    }

    settings_ = std::move(*loaded);
    recent_projects_.replace(settings_.recent_projects);
}

bool Application::load_startup_project(std::string& error)
{
    error.clear();

    std::filesystem::path project_path = *launch_options_.project_path;
    if (project_path.is_relative())
    {
        std::error_code filesystem_error;
        project_path = std::filesystem::absolute(project_path, filesystem_error);
        if (filesystem_error)
        {
            error = "Unable to resolve project path: " + filesystem_error.message();
            return false;
        }
    }
    project_path = project_path.lexically_normal();

    std::optional<ProjectFile> project = load_project_file(project_path, &error);
    if (!project.has_value())
    {
        return false;
    }

    const std::filesystem::path scene_path =
        resolve_project_scene_path(project_path, project->start_scene);
    std::unique_ptr<Scene> scene = SceneSerializer::load(scene_path, &error);
    if (!scene)
    {
        return false;
    }

    active_project_path_ = project_path;
    active_scene_ = std::move(scene);
    (void)recent_projects_.add(active_project_path_);
    settings_.recent_projects = recent_projects_.entries();
    fprintf(stdout, "[project] Opened %s\n", project_path.string().c_str());
    return true;
}

bool Application::save_editor_preferences()
{
    if (ui_initialized_)
    {
        const PerspectiveCamera& camera = ui_.viewport_camera();
        settings_.camera_position = camera.position();
        settings_.camera_yaw_degrees = camera.yaw_degrees();
        settings_.camera_pitch_degrees = camera.pitch_degrees();
        settings_.camera_move_speed = ui_.viewport_camera_speed();
        settings_.show_viewport_grid = ui_.is_viewport_grid_visible();
    }
    settings_.recent_projects = recent_projects_.entries();

    std::string error;
    if (!save_editor_settings(settings_path_, settings_, &error))
    {
        fprintf(stderr, "Settings warning: %s\n", error.c_str());
        return false;
    }
    return true;
}

}
