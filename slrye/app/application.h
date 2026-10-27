#pragma once

#include <memory>

#include <app/launch_options.h>
#include <services/plugin/plugin_manager.h>

namespace slrye
{

class VulkanBackend;
class EditorUI;

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
    ApplicationLaunchOptions launch_options_;
    PluginManager plugin_manager_;
    std::unique_ptr<VulkanBackend> backend_;
    std::unique_ptr<EditorUI> ui_;
};

}
