
# slrye

## 项目介绍
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


## 介绍
启动形态： 用户双击可执行文件或通过命令行传入工程路径（如 slrye.exe path/to/project.slproj）。

初始呈现：
若未指定工程：弹出轻量级 Project Hub（新建工程、最近工程列表）。

打开已有工程后：全屏 DockSpace 布局，包含：

Viewport 面板： Vulkan 离屏渲染的 3D 场景交互视口（支持 Gizmo 操控、WASD+右键自由漫游）。
Scene Hierarchy（实体/场景树）： 当前激活场景的树状节点结构。
Inspector（属性检查器）： 当前选中实体的组件列表（Transform、Mesh、Light、Camera、Collider 等）。
Console / Log： 实时编译日志、Vulkan Validation Layer 输出、引擎警告。
Toolbox / Tools： 动画时间轴（Timeline）、曲线与节点编辑器面板、性能 Profiler 面板。

用户完成哪些操作
实体/场景树操控： 选中、重命名、拖拽成父子层级、快捷键 Ctrl+D 复制或 Delete 删除实体。

参数实时热调：

拖动 DragFloat / ColorPicker 调节光源颜色、强度、阴影偏置、天空盒 Exposure。
实时修改摄像机 FOV、后处理 Bloom/SSAO 阈值、物理重力常数。
关卡与动画调试：

在时间轴（Sequencer/Timeline）上拖动当前帧游标，触发关键帧插值并观察骨骼网格体变形。
在节点视口（Node Editor）中连线行为树、材质 Shader 节点或动画混合树。
引擎状态切换： 点击顶部工具栏的 Play / Pause / Stop。Play 时锁定部分编辑器功能，启用物理与脚本运行时仿真；Stop 时重置场景状态。

程序保存什么结果
场景描述文件： 人类可读的文本格式（YAML/JSON），记录所有 Entity、UUID、Component 数据。

工程配置文件（*.slproj）： 起始加载场景路径、物理层掩码设置、渲染管线预设（如 MSAA 采样率）。
编辑器布局缓存（imgui.ini）： 停靠面板布局、各窗口停靠位置、浮动大小。
二进制管线/资产缓存： SPIR-V 编译缓存、纹理压缩缓存（KTX2/BC7）、网格体优化后二进制包（.mesh），避免重复导入开销。

哪些操作必须很快，哪些可以后台执行
必须极速（同步执行，严禁掉帧）：

视口相机的 WASD 漫游与鼠标拾取（Raycasting）。
ImGui 控件拖拽调参（每帧响应，直接更新底层 ECS/UniformBuffer）。
帧循环中的 Vulkan Command Buffer 录制与提交。

必须在后台执行（异步线程池）：

外部模型导入与切分（Assimp/cgltf 解析、骨骼权重计算、生成优化顶点流）。
纹理压缩（如 PNG/JPG 转 GPU 压缩格式）、mipmap 生成。
GLSL/HLSL 到 SPIR-V 的离线编译（Shader 异步热重载）。
场景与工程的整包序列化写入磁盘。

是否需要记住配置或历史记录
必须记住：

编辑器 UI 状态： ImGui 原生接管 imgui.ini，保留用户自定义的 Dock 窗口分布。
工程历史： 最近打开工程路径（MRU 列表，存于操作系统的 AppData 或用户主目录配置文件）。
视口状态： 上次退出时相机的位置、朝向、移动速度和网格辅助线开关。
撤销/重做（Undo/Redo 历史）：
参数修改（如 Inspector 调整数值）、实体增删、父子层级改动，均通过 Command 模式（命令栈）记录，支持 Ctrl+Z / Ctrl+Y。






## 插件系统

插件采用原生动态库 + C ABI 的方式接入。宿主程序在启动时扫描可执行文件同级的 `plugins/` 目录，每个插件目录内包含一个 `plugin.yaml` 清单和一个平台动态库。

```text
plugins/
└── demo_plugin/
    ├── plugin.yaml
    └── demo_plugin.dll
```

当前插件 API 第一版支持：

- 插件发现、加载和卸载
- `id`、名称、版本、作者、描述和 capabilities 声明
- 初始化与关闭生命周期
- 通过宿主 API 输出日志

示例插件位于 `plugins/demo_plugin/`。构建时默认启用，可通过以下选项关闭：

```text
-DSLRYE_BUILD_PLUGINS=OFF
```

原生插件边界使用 `slrye/include/slrye/plugin_api.h` 定义。以后增加菜单、视口、命令和资产导入器时，应继续扩展 capabilities 与 versioned API struct，而不是把 C++ STL 对象跨动态库边界传递。

插件架构决策记录见 `docs/decisions-plugin-system.md`。


## 技术栈

技术栈：C++20 + GLFW + Vulkan + Dear ImGui。

使用的第三方依赖
```json
{
  "name": "slrye",
  "version": "0.1.0",
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


