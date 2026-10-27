cmake_minimum_required(VERSION 3.20)
project(slrye CXX)

find_package(glfw3 CONFIG REQUIRED)
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
    platform/paths.cpp
    services/plugin/plugin_manager.cpp
    services/plugin/plugin_manifest.cpp
    ui/editor_ui.cpp
    ui/theme_manager.cpp
    ui/styles/spectrum.cpp
    ui/styles/style.cpp
    views/view.cpp
)

add_library(slrye_runtime STATIC
    ${SLRYE_RUNTIME_SOURCES}
)

target_link_libraries(slrye_runtime PUBLIC
    Vulkan::Vulkan
    glfw
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

if(WIN32)
    if(NOT SLRYE_ENABLE_CONSOLE)
        set_target_properties(slrye PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
