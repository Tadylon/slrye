#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <editor/app/launch_options.h>

#include <engine/services/project/recent_projects.h>
#include <editor/shell/editor_settings.h>

namespace slrye::engine
{

class VulkanBackend;
class Scene;
}
namespace slrye {
class EditorUI;
class ImGuiBackend;

class Application
{
public:
    [[nodiscard]] static Application& instance(int argc, char** argv);

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run();

private:
    Application(int argc, char** argv);
    ~Application();

    void load_editor_preferences();
    bool load_startup_project(std::string& error);
    bool save_editor_preferences();

    ApplicationLaunchOptions launch_options_;
    std::unique_ptr<engine::Scene> active_scene_;
    EditorSettings settings_;
    engine::RecentProjects recent_projects_;
    std::filesystem::path settings_path_;
    std::filesystem::path active_project_path_;
    // PluginManager& plugin_manager_;
    engine::VulkanBackend& backend_;
    ImGuiBackend& imgui_backend_;
    EditorUI& ui_;
    bool ui_initialized_ = false;
};

}
