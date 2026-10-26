#pragma once

#include <filesystem>
#include <memory>

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
    static std::filesystem::path resolve_executable_path(int argc, char** argv);

    std::filesystem::path executable_path_;
    PluginManager plugin_manager_;
    std::unique_ptr<VulkanBackend> backend_;
    std::unique_ptr<EditorUI> ui_;
};

}
