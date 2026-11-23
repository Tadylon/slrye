# Engine Renderer 模块 (`slrye/engine/renderer`)

## 1. 模块定位与职责
`renderer` 是引擎的 3D 渲染与几何构建核心。它负责将 ECS 场景数据转换提取为 3D 顶点几何流，驱动底层的 Vulkan Graphics Pipeline 进行离屏渲染（Offscreen Rendering），实现相机透视矩阵变换、定向光漫反射着色、深度测试、地面坐标参考网格绘制，并将渲染产出的离屏颜色附件无缝注册为 Dear ImGui 可直接采样的纹理句柄（`ImTextureID`）。

---

## 2. 文件清单与深度解析

### 2.1 `camera.h` / `camera.cpp`
* **定位**：3D 透视摄像机与射线生成器。
* **主要内容**：
  * `Ray`：包含原点 `origin` 与归一化方向 `direction` 的 3D 射线；
  * `PerspectiveCamera` 类：
    * **参数调节**：位置 `position`、俯仰角 `pitch`（自动限制在 [-89°, 89°]）、偏航角 `yaw`、垂直视场角 `fov`、宽高比 `aspect_ratio` 以及近/远裁剪平面（`near_clip` / `far_clip`）；
    * **局部基向量**：自动推导正向 `forward()`、右向 `right()`、上向 `up()`；
    * **矩阵合成**：
      * `view_matrix()`：基于 `glm::lookAt` 生成视图矩阵；
      * `projection_matrix()`：生成符合 **Vulkan 标准** 的右手中指 NDC 投影矩阵（深度范围 `[0, 1]` 配合 `glm::perspectiveRH_ZO`，且应用 `projection[1][1] *= -1.0f` 进行 Vulkan Y 轴反转）；
      * `view_projection_matrix()`：合并视图与投影变换。
    * **屏幕拾取射线求值**：`ray_from_viewport(x, y, width, height)` 将 2D 视口像素坐标反变换为世界空间拾取射线 `Ray`。

### 2.2 `scene_geometry.h` / `scene_geometry.cpp`
* **定位**：场景几何网格动态生成与提取器。
* **主要内容**：
  * `SceneVertex`：标准顶点布局（位置 `position`、法线 `normal`、顶点色 `color`）；
  * `build_scene_geometry(scene, selected_entity, show_grid)`：
    1. **参考网格生成 (`append_grid`)**：生成带明确主轴标识的水平地面网格（X 轴为亮红、Z 轴为亮蓝、细分辅助线带适宜线宽并具漫反射法线）；
    2. **实体几何代理提取**：遍历场景内所有携带 `TransformComponent` 的实体，结合其层级世界矩阵进行顶点变换。如果有 `ColliderComponent` 则绘制其形状代理（立方体 Box、圆球 Sphere、胶囊体 Capsule）；如果被选中（`selected_entity`）则渲染鲜艳的高亮橙色轮廓色。

### 2.3 `scene_renderer.h` / `scene_renderer.cpp`
* **定位**：Vulkan 离屏场景渲染管线实现（采用 PIMPL 模式完全隐藏 Vulkan 底层头文件依赖）。
* **主要内容**：
  * **管线搭建**：
    * 动态载入编译好的 SPIR-V 着色器（`viewport.vert.spv` 与 `viewport.frag.spv`）；
    * 创建带深度缓冲的 `VkRenderPass`（Color 为 `VK_FORMAT_B8G8R8A8_UNORM` 等，Depth 为 `VK_FORMAT_D32_SFLOAT` 等）；
    * 配置 Push Constant 实时传递 `ViewProjection` 矩阵；
    * 配置深度测试、混合与光栅化状态。
  * **离屏渲染目标管理 (`ensure_render_target`)**：
    * 当视口面板尺寸发生变化时，自动销毁并重建离屏 `color_image_` 与 `depth_image_`；
    * 通过 `ImGui_ImplVulkan_AddTexture` 将离屏色彩图转换为 `VkDescriptorSet` 并强转为 `ImTextureID`，供 ImGui 视口面板像普通图片一样显示。
  * **顶点上传与绘制 (`render`)**：
    * 采用动态扩容的 Host-Visible/Host-Coherent 映射顶点缓冲；
    * 录制 Command Buffer 并提交到图形队列执行渲染。
