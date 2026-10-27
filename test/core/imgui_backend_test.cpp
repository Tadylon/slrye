#include <core/imgui_backend.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ImGui backend rejects initialization without a window")
{
    slrye::ImGuiBackend backend;
    ImGui_ImplVulkan_InitInfo init_info{};

    CHECK_FALSE(backend.is_initialized());
    CHECK_FALSE(backend.initialize(nullptr, init_info));
    CHECK_FALSE(backend.begin_frame());
    CHECK_FALSE(backend.is_initialized());
}

TEST_CASE("ImGui backend can be shut down repeatedly before successful initialization")
{
    slrye::ImGuiBackend backend;

    backend.shutdown();
    backend.render_platform_windows();
    backend.shutdown();

    CHECK_FALSE(backend.is_initialized());
}
