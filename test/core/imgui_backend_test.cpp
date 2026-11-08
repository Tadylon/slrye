#include <core/imgui_backend.h>
#include <core/vulkan_backend.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ImGui backend rejects initialization without a window")
{
    slrye::ImGuiBackend& backend = slrye::ImGuiBackend::instance();
    slrye::ImGuiBackend& same_backend = slrye::ImGuiBackend::instance();
    ImGui_ImplVulkan_InitInfo init_info{};

    CHECK(&backend == &same_backend);
    CHECK_FALSE(backend.is_initialized());
    CHECK_FALSE(backend.initialize(nullptr, init_info));
    CHECK_FALSE(backend.begin_frame());
    CHECK_FALSE(backend.is_initialized());
}

TEST_CASE("ImGui backend can be shut down repeatedly before successful initialization")
{
    slrye::ImGuiBackend& backend = slrye::ImGuiBackend::instance();

    backend.shutdown();
    backend.render_platform_windows();
    backend.shutdown();

    CHECK_FALSE(backend.is_initialized());
}

TEST_CASE("Vulkan backend exposes one process-wide instance")
{
    slrye::VulkanBackend& backend = slrye::VulkanBackend::instance();
    slrye::VulkanBackend& same_backend = slrye::VulkanBackend::instance();

    CHECK(&backend == &same_backend);
    CHECK_FALSE(backend.context().is_valid());
}
