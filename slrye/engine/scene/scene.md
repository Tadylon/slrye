# Engine Scene 模块 (`slrye/engine/scene`)

## 1. 模块定位与职责
`scene` 是游戏引擎的数据核心与 ECS（Entity-Component-System，实体组件系统）架构实现层。底层基于高性能的 **EnTT 3.x** 注册表，负责组织游戏世界中的所有实体、挂载各类业务组件、维护基于父子关系的变换树（Hierarchy Graph）、计算局部与世界空间变换矩阵、追踪场景修改版本号（`revision`），并提供空间射线拾取（Ray-Casting Picking）。

---

## 2. 文件清单与深度解析

### 2.1 `entity_id.h`
* **定位**：强类型 64 位实体唯一标识符。
* **主要内容**：
  * `EntityId` 类：封装 64 位无符号整型 ID，避免了原始整数类型安全隐患；
  * 提供 `is_valid()`、哈希特化（`std::hash<EntityId>`）与比较操作符，可在 `std::unordered_map` 或集合中作为高性能 Key 使用。

### 2.2 `components.h`
* **定位**：引擎基础纯数据组件（POD）定义集合。
* **主要内容**：
  * `NameComponent`：实体易读名称（`std::string value`）；
  * `TransformComponent`：局部空间变换：平移 `position (vec3)`、旋转四元数 `rotation (quat)`、缩放 `scale (vec3)`；
  * `HierarchyComponent`：场景树拓扑信息：存储父实体 `parent` 与所有直接子实体列表 `children (vector<EntityId>)`；
  * `CameraComponent`：场景相机参数（`vertical_fov_degrees`, `near_clip`, `far_clip`, `primary`）；
  * `LightComponent`：光源参数（支持 `Directional`, `Point`, `Spot`，含色彩 `color`、强度 `intensity`、作用半径 `range` 与聚光灯内外锥角）；
  * `ColliderComponent`：碰撞体几何参数（支持 `Box`, `Sphere`, `Capsule`，含半长宽 `half_extents`、半径 `radius` 与高度 `height`）。

### 2.3 `entity.h`
* **定位**：面向上层开发的易用句柄（Facade Wrapper）。
* **主要内容**：
  * 将 `Scene*` 与 `EntityId` 打包成一个轻量对象；
  * 提供直观的面向对象 API：
    * `add<T>(...)`：添加组件（若已有则抛出安全异常）；
    * `get<T>()`：获取已有组件的直接引用；
    * `try_get<T>()`：安全获取组件指针（不存在返回 `nullptr`，不抛异常）；
    * `has<T>()`：判断是否携带指定组件；
    * `set_parent(...)` / `clear_parent()`：建立或解除父子层级关系；
    * `destroy()`：将实体从场景中销毁（级联销毁所有子节点）。

### 2.4 `scene.h` / `scene.cpp`
* **定位**：场景主容器与 EnTT 封装桥梁。
* **主要内容**：
  * 封装 `entt::registry`，维护 `EntityId` 与 `entt::entity` 之间的双向映射关系；
  * **实体创建与默认组件保证**：
    `create_entity(name)` 创建时自动为其保底附加 `NameComponent`、`TransformComponent` 和 `HierarchyComponent`；
  * **层级拓扑与循环防范**：`set_parent` 严格执行拓扑排序自检，杜绝将自身作为子节点或发生循环引用；
  * **脏标记与版本迭代 (`revision`)**：任何属性修改或实体变动均自动递增 `revision_`，视口渲染器通过比对 revision 决定是否需要重录绘制缓冲，极致降低空闲帧开销。

### 2.5 `scene_transform.h` / `scene_transform.cpp`
* **定位**：层级空间变换矩阵运算器。
* **主要内容**：
  * `local_transform_matrix(transform)`：将平移、四元数旋转、缩放合成为 $4 \times 4$ 局部空间 TRS 仿射矩阵；
  * `world_transform_matrix(scene, entity, result)`：沿父子拓扑链由根至叶递推相乘，计算实体的最终世界空间变换矩阵。

### 2.6 `picking.h` / `picking.cpp`
* **定位**：3D 视口光线投射拾取器。
* **主要内容**：
  * `pick_entity(scene, ray)`：将摄像机投射的射线与场景中具有几何代理（Box、Sphere 等）的实体进行空间求交检测；
  * 自动筛选出沿射线方向距离相机最近的有效实体，返回 `PickResult{ entity, distance }`，实现鼠标点击 3D 物体精准选中。
