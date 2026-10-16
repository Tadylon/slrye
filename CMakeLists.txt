# 1. 声明 CMake 的最低版本要求
cmake_minimum_required(VERSION 4.3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# 2. 定义项目名称
project(Slrye CXX)

# 寻找通过 vcpkg 安装的包
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
# 寻找系统安装的 Vulkan SDK（会利用你已经配置好的 Vulkan SDK 环境变量）
find_package(Vulkan REQUIRED)

# 3. 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SLRYE_BUILD_EXAMPLES "Build examples" OFF)
option(SLRYE_BUILD_TESTS    "Build tests"    OFF)
option(SLRYE_BUILD_TOOLS    "Build tools"    OFF)


add_subdirectory(slrye)

if(SLRYE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SLRYE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(SLRYE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()