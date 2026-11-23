# Editor App 模块 (`slrye/editor/app`)

## 1. 模块定位与职责
`app` 是整个编辑器桌面应用程序的顶层启动与生命周期总控中心。包含主入口函数 `main()`，负责命令行参数解析、全局异常防护拦截、偏好设置载入与写回、默认场景自举填充、各子系统（Vulkan 硬件后端、Dear ImGui 渲染后端、EditorUI 界面壳层）的依序初始化、主循环调度与安全关机。

---

## 2. 文件清单与深度解析

### 2.1 `main.cpp`
* **定位**：应用程序的 C++ 标准入口文件。
* **主要内容**：
  * 实例化单例 `Application::instance(argc, argv)`；
  * **全生命周期顶层防护**：使用 `try ... catch (const std::exception&)` 包裹 `application.run()`，在发生任何非预期异常时拦截并向标准错误输出可读日志（`[Fatal error] Unhandled exception: ...`），杜绝直接发生未捕获异常引发的无提示静默 `abort()` 崩溃。

### 2.2 `launch_options.h` / `launch_options.cpp`
* **定位**：启动命令行参数解析组件。
* **主要内容**：
  * `ApplicationLaunchOptions`：存储解析后的绝对执行路径 `executable_path`、可选的指定工程文件路径 `project_path` 与解析错误提示 `error`；
  * `parse_launch_options(argc, argv)`：支持通过 `--project <path>` 或直接拖拽工程文件到可执行文件上启动引擎。

### 2.3 `application.h` / `application.cpp`
* **定位**：编辑器主应用程序中枢（`Application` 单例）。
* **主要内容**：
  * **主启动流 (`run`)**：
    1. 载入用户配置偏好（`load_editor_preferences`）；
    2. 加载命令行传入的工程；若为全新启动且场景为空，自动自举创建基准测试场景（Cube、Directional Light、Main Camera）；
    3. 初始化底层物理窗口与 Vulkan 设备（`VulkanBackend::initialize`）；
    4. 初始化 Dear ImGui Vulkan/GLFW 渲染后端（`ImGuiBackend::initialize`）；
    5. 初始化 EditorUI 停靠工作区、视口与面板（`EditorUI::initialize` 与 `initialize_viewport`）；
  * **主循环 (`while (!backend_.should_close())`)**：
    * 每帧轮询窗口事件与 ImGui 状态（`begin_frame()`）；
    * 驱动 `EditorUI::render()` 绘制 Docking 菜单、各个子面板与 3D 视口；
    * 驱动 `ImGuiBackend::render()` 执行交换链命令提交与画面呈现；
  * **安全关机**：将编辑器当前的摄像机状态、速度、视口网格开关、最近工程写入用户目录 `editor.yaml`，最后按拓扑逆序优雅清理各模块。
