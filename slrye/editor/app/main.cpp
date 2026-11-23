#include <editor/app/application.h>

#include <cstdio>
#include <exception>

int main(int argc, char** argv)
{
    try
    {
        return slrye::Application::instance(argc, argv).run();
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "[Fatal error] Unhandled exception: %s\n", exception.what());
        return 1;
    }
    catch (...)
    {
        std::fprintf(stderr, "[Fatal error] Unknown unhandled exception occurred.\n");
        return 1;
    }
}
