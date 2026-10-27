cmake_minimum_required(VERSION 3.20)
project(slrye CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

set(SLRYE_RUNTIME_SOURCES
    app/application.cpp
    app/launch_options.cpp
    core/imgui_backend.cpp
    core/vulkan_backend.cpp
    domain/camera.cpp
    domain/scene.cpp
    domain/scene_transform.cpp
    platform/file_io.cpp
    platform/path_encoding.cpp
    platform/paths.cpp
    services/plugin/plugin_manager.cpp
    services/plugin/plugin_manifest.cpp
    services/project/project_file.cpp
    services/project/recent_projects.cpp
    services/picking/picking.cpp
    services/rendering/scene_geometry.cpp
    services/rendering/scene_renderer.cpp
    services/serialization/scene_serializer.cpp
    services/settings/editor_settings.cpp
    ui/editor_ui.cpp
    ui/theme_manager.cpp
    ui/styles/spectrum.cpp
    ui/styles/style.cpp
    views/camera_controller.cpp
    views/view.cpp
    views/viewport.cpp
)

add_library(slrye_runtime STATIC
    ${SLRYE_RUNTIME_SOURCES}
)

target_link_libraries(slrye_runtime PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
    imgui::imgui
    EnTT::EnTT
    spdlog::spdlog
    yaml-cpp::yaml-cpp
)

target_include_directories(slrye_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(slrye app/main.cpp)
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
