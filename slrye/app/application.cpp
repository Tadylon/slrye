#include <app/application.h>

#include <cstdio>
#include <string>

#include <core/vulkan_backend.h>
#include <platform/paths.h>
#include <ui/editor_ui.h>

namespace slrye
{

Application::Application(int argc, char** argv)
    : launch_options_(parse_launch_options(argc, argv)),
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

    backend_ = std::make_unique<VulkanBackend>();
    if (!backend_->initialize("Slrye", 1280, 800))
    {
        return 1;
    }

    plugin_manager_.load_all(platform::plugins_directory(launch_options_.executable_path));

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

}
