# Editor Shell 模块 (`slrye/editor/shell`)

## 1. 模块定位与职责
`shell` 是编辑器的视觉外壳与停靠框架（Docking Workspace）。它构建并维系着现代 IDE 风格的顶层主菜单栏（Menu Bar）、全窗口停靠工作区（DockSpace）、底部状态栏（Status Bar）、编辑器全局偏好与布局持久化（`editor.yaml`），以及多套精美视觉主题（Spectrum Light / Spectrum Dark / EnemyMouse）的无缝动态切换。

---

## 2. 文件清单与深度解析

### 2.1 `editor_ui.h` / `editor_ui.cpp`
* **定位**：编辑器界面总体协调器单例。
* **主要内容**：
  * **主菜单栏 (`draw_menu_bar`)**：
    * `File`：新建工程（New Project）、打开工程（Open Project）、保存（Save）、退出（Exit）；
    * `Edit`：撤销（Undo）、重做（Redo）；
    * `View`：主题切换（Spectrum Light, Spectrum Dark, EnemyMouse）、各面板显隐开关复选框（Viewport, Hierarchy, Inspector, Console）；
    * `Help`：关于对话框（About Slrye）。
  * **自适应停靠空间构建 (`draw_workspace`)**：
    * 提取主视口可用尺寸，在主窗口内部利用 `ImGui::DockBuilder` 构建标准 4 分栏预设布局（左侧 Scene Hierarchy、中央 Viewport、右侧 Inspector、下方 Console）；
  * **底部状态栏 (`draw_status_bar`)**：实时显示当前引擎运行状态、已激活主题名称与当前每秒渲染帧率（FPS）；
  * **面板状态协同 (`render`)**：协调 Viewport 与 Scene Hierarchy 之间选事实体的双向同步，随后按序调度各个子面板的 `render()`。

### 2.2 `editor_settings.h` / `editor_settings.cpp`
* **定位**：编辑器全局偏好配置结构体与 YAML 序列化组件。
* **主要内容**：
  * `EditorSettings`：存储偏好主题 `theme`、视口摄像机默认三维坐标 `camera_position`、俯仰偏航角、漫游速度 `camera_move_speed`、视口网格开关 `show_viewport_grid` 以及最近工程历史列表；
  * `load_editor_settings(path, error)`：从磁盘解析 YAML 配置，带格式版本号兼容校验；
  * `save_editor_settings(path, settings, error)`：将配置原子写回磁盘，实现用户下一次启动时完全恢复上一次的工作习惯。

### 2.3 `theme_manager.h` / `theme_manager.cpp`
* **定位**：视觉主题切换管理器。
* **主要内容**：
  * `apply(theme)`：在运行时一键应用指定主题：
    * `EditorTheme::SpectrumLight`：明亮 Adobe Spectrum 现代极简风格；
    * `EditorTheme::SpectrumDark`：深色夜间低疲劳专业设计风格；
    * `EditorTheme::EnemyMouse`：高对比度经典暗色风格；
  * `display_name(theme)`：获取主题的友好显示名称。

### 2.4 `styles/style.h` / `styles/style.cpp` & `styles/spectrum.h` / `styles/spectrum.cpp`
* **定位**：主题配色盘与控件几何微观参数定制。
* **主要内容**：
  * 深度定制 Dear ImGui 的 55 种颜色常量（`ImGuiCol_WindowBg`, `ImGuiCol_Header`, `ImGuiCol_ButtonActive`, `ImGuiCol_DockingPreview` 等）；
  * 微调窗口圆角（`WindowRounding = 0.0f` 适应无缝停靠）、滚动条粗细、控件边框大小与间距，使整个界面具有现代商业生产力工具的高级质感。
