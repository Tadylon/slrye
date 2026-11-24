cmake_minimum_required(VERSION 3.20)
project(slrye CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

set(SLRYE_ENGINE_SOURCES
    engine/core/log.cpp
    engine/platform/file_io.cpp
    engine/platform/path_encoding.cpp
    engine/platform/paths.cpp
    engine/renderer/camera.cpp
    engine/renderer/scene_geometry.cpp
    engine/renderer/scene_renderer.cpp
    engine/rhi/vulkan/vulkan_backend.cpp
    engine/scene/scene.cpp
    engine/scene/picking.cpp
    engine/scene/scene_transform.cpp
    engine/services/project/project_file.cpp
    engine/services/project/recent_projects.cpp
    engine/services/serialization/scene_serializer.cpp
)

set(SLRYE_EDITOR_SOURCES
    editor/app/application.cpp
    editor/app/launch_options.cpp
    editor/backend/imgui_backend.cpp
    editor/shell/editor_settings.cpp
    editor/shell/editor_ui.cpp
    editor/shell/theme_manager.cpp
    editor/shell/styles/spectrum.cpp
    editor/shell/styles/style.cpp
    editor/panels/viewport_camera_controller.cpp
    editor/panels/camera_controller.cpp
    editor/panels/console_panel.cpp
    editor/panels/inspector_panel.cpp
    editor/panels/scene_hierarchy_panel.cpp
    editor/panels/view.cpp
    editor/panels/viewport.cpp
)

add_library(slrye_runtime STATIC
    ${SLRYE_ENGINE_SOURCES}
    ${SLRYE_EDITOR_SOURCES}
)

target_link_libraries(slrye_runtime PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
    imgui::imgui
    EnTT::EnTT
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
)

target_include_directories(slrye_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/editor
)

add_executable(slrye editor/app/main.cpp)
target_link_libraries(slrye PRIVATE slrye_runtime)

add_custom_command(TARGET slrye POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
)

if(WIN32)
    if(NOT SLRYE_ENABLE_CONSOLE)
        set_target_properties(slrye PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
