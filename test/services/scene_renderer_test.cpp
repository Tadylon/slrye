#include <services/rendering/scene_renderer.h>

#include <catch2/catch_test_macros.hpp>

#include <string>

#include <core/vulkan_context.h>
#include <domain/camera.h>
#include <domain/scene.h>

TEST_CASE("Scene renderer rejects missing Vulkan resources without throwing")
{
    slrye::SceneRenderer renderer;
    slrye::VulkanContext context;
    std::string error;

    CHECK_FALSE(renderer.is_initialized());
    CHECK_FALSE(renderer.initialize(context, "assets/shaders", &error));
    CHECK_FALSE(error.empty());

    slrye::Scene scene;
    slrye::PerspectiveCamera camera;
    CHECK_FALSE(renderer.render(scene, camera, 1280, 720, std::nullopt, true, &error));
    CHECK_FALSE(error.empty());
}
