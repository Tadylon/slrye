#include <app/application.h>

#include <cstdio>
#include <string>

#include <core/vulkan_backend.h>
#include <ui/editor_ui.h>

namespace slrye
{

Application::Application(int argc, char** argv)
    : executable_path_(resolve_executable_path(argc, argv)),
      plugin_manager_([](const std::string& message)
      {
          fprintf(stderr, "[plugin] %s\n", message.c_str());
      })
{
}

Application::~Application() = default;

int Application::run()
{
    backend_ = std::make_unique<VulkanBackend>();
    if (!backend_->initialize("Slrye", 1280, 800))
    {
        return 1;
    }

    plugin_manager_.load_all(executable_path_.parent_path() / "plugins");

    ui_ = std::make_unique<EditorUI>();
    ui_->initialize();

    while (!backend_->should_close())
    {
        if (!backend_->begin_frame())
        {
            continue;
        }

        const ImVec4 clear_color = ui_->render(backend_->io());
        backend_->render(clear_color);
    }

    plugin_manager_.shutdown();
    backend_->shutdown();
    return 0;
}

std::filesystem::path Application::resolve_executable_path(int argc, char** argv)
{
    if (argc > 0 && argv != nullptr && argv[0] != nullptr)
    {
        return std::filesystem::absolute(argv[0]);
    }

    return std::filesystem::current_path() / "slrye.exe";
}

}
