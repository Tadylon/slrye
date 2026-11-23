#include <editor/backend/imgui_backend.h>
#include <engine/rhi/vulkan/vulkan_backend.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ImGui backend rejects initialization without a window")
{
    slrye::ImGuiBackend& backend = slrye::ImGuiBackend::instance();
    slrye::ImGuiBackend& same_backend = slrye::ImGuiBackend::instance();
    slrye::VulkanContext context;

    CHECK(&backend == &same_backend);
    CHECK_FALSE(backend.is_initialized());
    CHECK_FALSE(backend.initialize(nullptr, context));
    
    slrye::VulkanBackend uninit_vk;
    CHECK_FALSE(backend.initialize(uninit_vk));
    
    CHECK_FALSE(backend.begin_frame());
    CHECK_FALSE(backend.is_initialized());
}

TEST_CASE("ImGui backend can be shut down repeatedly before successful initialization")
{
    slrye::ImGuiBackend& backend = slrye::ImGuiBackend::instance();

    backend.shutdown();
    backend.render(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
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

TEST_CASE("Vulkan backend can be directly instantiated as an owned object")
{
    slrye::VulkanBackend owned_backend;
    CHECK_FALSE(owned_backend.context().is_valid());
    owned_backend.wait_idle();
}
