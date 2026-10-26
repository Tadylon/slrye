cmake_minimum_required(VERSION 3.20)
project(slrye CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SLRYE_SOURCES CONFIGURE_DEPENDS *.cpp *.cc *.c)
file(GLOB_RECURSE SLRYE_HEADERS CONFIGURE_DEPENDS *.h *.hpp)

add_executable(slrye
    ${SLRYE_SOURCES}
    ${SLRYE_HEADERS}
)

target_link_libraries(slrye PRIVATE
    Vulkan::Vulkan
    glfw
    imgui::imgui
    EnTT::EnTT
    spdlog::spdlog
    yaml-cpp::yaml-cpp
)

target_include_directories(slrye
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    if(NOT SLRYE_ENABLE_CONSOLE)
        set_target_properties(slrye PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
