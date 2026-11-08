#include <app/application.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Application exposes one process-wide instance")
{
    char executable[] = "slrye_tests.exe";
    char* arguments[] = {executable};

    slrye::Application& application = slrye::Application::instance(1, arguments);
    slrye::Application& same_application = slrye::Application::instance(1, arguments);

    CHECK(&application == &same_application);
}
