cmake_minimum_required(VERSION 3.20)
project(slrye CXX)

# 寻找依赖
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SLRYE_SOURCES CONFIGURE_DEPENDS *.cpp *.cc *.c)
file(GLOB_RECURSE SLRYE_HEADERS CONFIGURE_DEPENDS *.h *.hpp)

# 创建你的主程序 EXE
add_executable(slrye
    ${SLRYE_SOURCES}
    ${SLRYE_HEADERS}
)

# 将 ImGui、GLFW 和 OpenGL 链接到你的主程序上
target_link_libraries(slrye PRIVATE 
    imgui::imgui           # 我们刚刚编译的 ImGui 库
    glfw
    Vulkan::Vulkan
)

target_include_directories(slrye
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# (可选) 在 Windows 下隐藏黑色的控制台窗口
# if(WIN32)
#     # 将子系统改为 Windows，而不是 Console
#     set_target_properties(slrye PROPERTIES WIN32_EXECUTABLE TRUE)
# endif()
