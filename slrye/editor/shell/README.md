

编辑器外壳体系 (原 ui)


 "Workbench Shell"（工作台外壳） 模式。它是编辑器整个顶层 UI 界面的总装宿主，组织桌面环境中的通用交互框架：

文件 / 类	角色与定位	内部成员与职责
editor_ui.h / .cpp
(EditorUI)	顶层 UI 界面外壳协调器
(Master Orchestrator)	1. 负责构建顶层主菜单栏 draw_menu_bar()（File/Edit/View/Help）；
2. 负责通过 ImGui::DockBuilder 构建全屏停靠工作区 draw_workspace()（划分左层级树、中央视口、右属性、下控制台的标准 4 分栏）；
3. 负责底部状态栏 draw_status_bar() 与关于弹窗 draw_about_window()；
4. 持有并协调四大子面板（Viewport、SceneHierarchyPanel、InspectorPanel、ConsolePanel），并在它们之间进行“选中实体”的双向同步。
editor_settings.h / .cpp
(EditorSettings)	全局偏好与布局持久化	1. 存储用户偏好主题、3D 视口相机启动坐标/偏航俯仰角/漫游速度、网格地面开关、最近工程列表；
2. 提供 load_editor_settings 与 save_editor_settings 将配置安全写入 editor.yaml，保证下次启动时完全恢复先前的视口姿态与编辑环境。
theme_manager.h / .cpp
(ThemeManager)	运行时视觉主题管理器	负责动态切换 Dear ImGui 的全局调色板、圆角和几何间距，支持 SpectrumLight（浅色）、SpectrumDark（暗黑）、EnemyMouse（高对比度）三种风格，并提供人类可读的菜单显示名。
styles/ 目录	微观主题样式配置	深度定制 Dear ImGui 的 55 个 ImGuiCol_* 颜色常量与无缝停靠几何属性（如 WindowRounding = 0.0f）。


