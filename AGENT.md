
以 C++20 + GLFW + Vulkan + Dear ImGui 架构自研游戏引擎编辑器


实时参数调优： 动态调整光照强度、物理重力、摄像机 FOV、着色器参数等
实体/场景树查看： 类似 Unity Inspector 或 Unreal Details 面板，查看场景中物体属性并直接编辑。
关卡与动画调试： 配合节点编辑器或时间轴，制作轻量级骨骼调试、路径编辑工具。

## 目录结构
```
slrye/
├── CMakeLists.txt
├── vcpkg.json                          # 声明依赖
├── vcpkg-configuration.json
├── assets/
│   ├── shaders/                        # GLSL 源文件及编译后的 SPIR-V
│   └── fonts/                          # 适用于 ImGui 的等宽/字体图标文件
│── slrye/
    |── app/                 Application、启动参数、主循环
    |── core/                Vulkan/GLFW 后端、窗口和交换链封装
    |── ui/                  菜单栏、DockSpace、状态栏、主题
    |── views/               具体页面或工具面板
    |── domain/              业务模型和核心规则
    |── services/            文件、配置、日志、后台任务
    |── platform/            Windows 资源、文件关联、托盘等
```

app/       存放的有application ,launch_options ,main
core/      存放Vulkan/GLFW 后端、窗口和交换链封装，目前是由vulkan_backend ,imgui_backend
domain/    放不依赖 ImGui、GLFW、Vulkan 的业务模型，例如 Scene、Entity、Component、CommandStack、序列化模型等
platform/  这里放 Windows 资源、文件关联、托盘、进程启动、系统路径等平台相关能力。
services/  这里存放的是文件、配置、日志、后台任务
ui/        菜单栏、DockSpace、状态栏、主题
views/     具体页面或工具面板



## 技术栈

技术栈：C++20 + GLFW + Vulkan + Dear ImGui。

使用的第三方依赖
```json
{
"dependencies": [
    "catch2",
    "glfw3",
    "glm",
    "entt",
    "spdlog",
    "yaml-cpp",
    {
      "name": "imgui",
      "features": [
        "glfw-binding",
        "vulkan-binding",
        "docking-experimental"
      ]
    }
  ]
}
```


构建方式：使用CMake + Ninja + vcpkg 来进行构建这个项目并且管理依赖项目

版本控制：Git


执行：不要帮我执行 `CMake` 的构建任务