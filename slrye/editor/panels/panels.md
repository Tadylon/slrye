# Editor Panels 模块 (`slrye/editor/panels`)

## 1. 模块定位与职责
`panels` 包含了构成游戏引擎编辑器核心 UI 交互界面的所有子面板窗口。包括负责 3D 视觉呈现与交互导航的视口（Viewport）、相机控制器、场景拓扑树（Scene Hierarchy）、实体属性检查器（Inspector）、日志输出控制台（Console）等。各个面板独立封装，在主界面停靠（Docking）体系中协同工作。

---

## 2. 文件清单与深度解析

### 2.1 `viewport.h` / `viewport.cpp`
* **定位**：3D 核心视口面板。
* **主要内容**：
  * **离屏纹理呈现**：通过 `ImGui::Image` 将 `SceneRenderer` 绘制生成的 `ImTextureID` 以原始高保真色彩投影在 ImGui 视口窗口中，并自动跟踪窗口尺寸以动态重建后台渲染目标；
  * **顶部轻量工具栏 (`draw_toolbar`)**：展示当前视角模式（Perspective）、相机实时世界坐标、漫游速度调节滑动条（Speed）、网格显隐复选框（Grid）以及一键对焦按钮（Focus）；
  * **视口交互与射线拾取 (`process_input`)**：
    * 鼠标点击视口（左/右/中键）自动激活视口窗口焦点；
    * 鼠标右键拖动触发自由视角旋转，中键拖动触发平移，滚轮滑动触发变焦；
    * 鼠标左键点击视口内容时，将屏幕点击点反变换为视口空间射线，调用底层 `pick_entity` 挑选命中实体并与 Hierarchy/Inspector 双向同步选中态。

### 2.2 `camera_controller.h` / `camera_controller.cpp`
* **定位**：编辑器摄像机输入操控器。
* **主要内容**：
  * `CameraControllerInput`：封装当前帧视口收集到的交互状态（旋转标位 `rotate`、平移标位 `pan`、WASD 漫游键态、Shift 加速键 `boost`、鼠标像素位移与滚轮增量）；
  * `update(camera, input)`：
    * 处理右键视角旋转（更新摄像机 `yaw` 与 `pitch`）；
    * 处理中键拖拽平移（沿摄像机局部 `right` 与 `up` 平面平移）；
    * 处理独立滚轮缩放（沿正向推进/后退）；
    * 处理经典 FPS 漫游按键（`W` 前进、`S` 后退、`A` 左移、`D` 右移、`E` 垂直上升、`Q` 垂直下降）；
  * `focus(camera, target)`：聚焦物体算法，保持现有视角朝向不变，将相机瞬间移动到以目标为中心、距离为 `focus_distance` 的最佳观测点。

### 2.3 `scene_hierarchy_panel.h` / `scene_hierarchy_panel.cpp`
* **定位**：场景实体层级面板。
* **主要内容**：
  * **树形层级展示 (`draw_entity_node`)**：使用 `ImGui::TreeNodeEx` 递归呈现场景中的所有根实体及其嵌套子节点；
  * **稳定选中逻辑**：支持单击选中实体；仅在点击无条目空白区域且未悬浮任何 UI 控件时才取消选中；
  * **拖拽重设父子层级（Drag & Drop）**：
    * 基于 ImGui 拖拽有效负载（Payload `"HIERARCHY_ENTITY"`），支持将一个实体拖拽释放到另一个实体上，自动调用 `scene.set_parent` 重新挂载；
  * **上下文快捷菜单 (`draw_context_menu`)**：支持右键弹出菜单，一键创建“空实体”、“点光源”或“带碰撞体立方体”。

### 2.4 `inspector_panel.h` / `inspector_panel.cpp`
* **定位**：实体属性检查器面板。
* **主要内容**：
  * **实体名称与 ID 编辑**：支持重命名实体并显示其底层唯一的 64 位 `EntityId`；
  * **Transform 组件编辑**：
    * 封装 `draw_vec3_control`：以红（X）、绿（Y）、蓝（Z）标签呈现三维向量拖拽条与输入框，支持重置为默认值；
    * 实时编辑 Position、Rotation（四元数与欧拉角安全互转）与 Scale；
  * **其它组件可视化**：展示并调整 Light 组件（类型、色彩选择器、强度、范围）、Collider 组件（类型、包围盒半长宽、半径、开关状态）与 Camera 组件；
  * **动态挂载组件 (`AddComponentPopup`)**：支持在实体上动态添加尚未拥有的组件。

### 2.5 `console_panel.h` / `console_panel.cpp`
* **定位**：编辑器内部运行时控制台面板。
* **主要内容**：
  * 支持记录信息（Info - 绿/灰）、警告（Warning - 黄）与错误（Error - 红）等级的日志；
  * 提供一键清空日志（Clear）与自动滚动到底部（Auto-scroll）开关。

### 2.6 `view.h` / `view.cpp`
* **定位**：编辑器辅助视图状态展示。
* **主要内容**：
  * `EditorViewState`：控制 Demo 窗口显隐与主视口清屏底色（`clear_color`）；
  * `render(state)`：在开发调试期可方便调出 Dear ImGui 官方全功能控件展示窗口。
