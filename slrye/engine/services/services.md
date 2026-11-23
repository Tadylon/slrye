# Engine Services 模块 (`slrye/engine/services`)

## 1. 模块定位与职责
`services` 模块封装了引擎面向工作流与资产持久化的高阶业务服务。它负责工程描述文件（`.slproj`）的解析与存储、最近打开工程（MRU）列表的维护与去重，以及整个 ECS 场景树与所有挂载组件的深度 YAML 序列化与反序列化（`.slscene`）。

---

## 2. 文件清单与深度解析

### 2.1 `project/project_file.h` / `project_file.cpp`
* **定位**：工程元数据读写器。
* **主要内容**：
  * `ProjectFile`：定义工程名称 `name` 与默认启动场景相对路径 `start_scene`；
  * `load_project_file(path, error)`：解析 `.slproj` 文本文件，读取工程配置；
  * `save_project_file(path, project, error)`：原子保存工程配置；
  * `resolve_project_scene_path(project_path, scene_path)`：辅助函数，将工程中记录的场景相对路径解析为绝对物理路径。

### 2.2 `project/recent_projects.h` / `project/recent_projects.cpp`
* **定位**：最近打开项目（MRU - Most Recently Used）管理器。
* **主要内容**：
  * `RecentProjects` 类：
    * `add(path)`：加入新打开的项目路径。自动将该项置顶，若已存在自动去重，若超出上限（默认 10 项）自动裁汰末尾最旧项；
    * `replace(...)` / `remove(...)` / `clear()`：批量同步、移除失效项目与清空缓存；
    * 与编辑器的最近项目菜单与启动欢迎向导紧密协作。

### 2.3 `serialization/scene_serializer.h` / `serialization/scene_serializer.cpp`
* **定位**：场景与 ECS 组件深度序列化器。
* **主要内容**：
  * `SceneSerializer` 类：
    * `save(path, scene, error)`：遍历场景中所有实体，采用结构化 YAML 格式输出：包含场景名、版本号 `kSceneFormatVersion`、实体的 64 位 `EntityId`、名称，以及可选的 `TransformComponent`（位姿/四元数/缩放）、`CameraComponent`、`LightComponent`、`ColliderComponent` 和层级父子关联；
    * `load(path, error)`：解析 YAML 节点，在内存中重新构建 `Scene` 实例，恢复各组件属性并正确连接父子拓扑指针。
