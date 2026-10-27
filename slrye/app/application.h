#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <app/launch_options.h>
#include <services/project/recent_projects.h>
#include <services/plugin/plugin_manager.h>
#include <services/settings/editor_settings.h>

namespace slrye
{

class VulkanBackend;
class EditorUI;
class Scene;

class Application
{
public:
    Application(int argc, char** argv);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run();

private:
    void load_editor_preferences();
    bool load_startup_project(std::string& error);
    bool save_editor_preferences();

    ApplicationLaunchOptions launch_options_;
    std::unique_ptr<Scene> active_scene_;
    EditorSettings settings_;
    RecentProjects recent_projects_;
    std::filesystem::path settings_path_;
    std::filesystem::path active_project_path_;
    PluginManager plugin_manager_;
    std::unique_ptr<VulkanBackend> backend_;
    std::unique_ptr<EditorUI> ui_;
};

}
