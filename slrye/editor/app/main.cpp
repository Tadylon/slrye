#include <editor/app/application.h>

int main(int argc, char** argv)
{
    return slrye::Application::instance(argc, argv).run();
}
