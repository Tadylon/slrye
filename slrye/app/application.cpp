#include <app/application.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include <core/vulkan_backend.h>
#include <domain/scene.h>
#include <platform/paths.h>
#include <services/project/project_file.h>
#include <services/serialization/scene_serializer.h>
#include <ui/editor_ui.h>

namespace slrye
{

Application::Application(int argc, char** argv)
    : launch_options_(parse_launch_options(argc, argv)),
      active_scene_(std::make_unique<Scene>("Untitled")),
      plugin_manager_([](const std::string& message)
      {
          fprintf(stderr, "[plugin] %s\n", message.c_str());
      })
{
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

    backend_ = std::make_unique<VulkanBackend>();
    if (!backend_->initialize("Slrye", 1280, 800))
    {
        save_editor_preferences();
        return 1;
    }

    plugin_manager_.load_all(platform::plugins_directory(launch_options_.executable_path));

    ui_ = std::make_unique<EditorUI>();
    ui_->initialize();
    ui_->set_theme(settings_.theme);

    while (!backend_->should_close())
    {
        if (!backend_->begin_frame())
        {
            continue;
        }

        const ImVec4 clear_color = ui_->render(backend_->io());
        backend_->render(clear_color);
    }

    save_editor_preferences();
    plugin_manager_.shutdown();
    backend_->shutdown();
    return 0;
}

void Application::load_editor_preferences()
{
    settings_path_ = platform::user_config_directory() / "editor.yaml";
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
    recent_projects_.add(active_project_path_);
    settings_.recent_projects = recent_projects_.entries();
    fprintf(stdout, "[project] Opened %s\n", project_path.string().c_str());
    return true;
}

bool Application::save_editor_preferences()
{
    if (ui_ != nullptr)
    {
        settings_.theme = ui_->theme();
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
