#include <app/launch_options.h>

#include <platform/paths.h>

namespace slrye
{

ApplicationLaunchOptions parse_launch_options(int argc, char** argv)
{
    ApplicationLaunchOptions options;
    options.executable_path = platform::resolve_executable_path(argc, argv);

    for (int index = 1; index < argc; ++index)
    {
        if (argv == nullptr || argv[index] == nullptr)
        {
            options.error = "Invalid command-line argument";
            return options;
        }

        const std::string argument = argv[index];
        if (argument == "--project")
        {
            if (index + 1 >= argc || argv[index + 1] == nullptr)
            {
                options.error = "--project requires a path";
                return options;
            }

            options.project_path = std::filesystem::path(argv[++index]);
            continue;
        }

        if (!argument.empty() && argument.front() == '-')
        {
            options.error = "Unknown option: " + argument;
            return options;
        }

        if (options.project_path.has_value())
        {
            options.error = "Only one project path can be opened at startup";
            return options;
        }

        options.project_path = std::filesystem::path(argument);
    }

    return options;
}

}
