# Engine RHI 模块 (`slrye/engine/rhi`)

## 1. 模块定位与职责
`rhi`（Rendering Hardware Interface，渲染硬件接口）是引擎与物理显卡及底层图形 API 交互的基石。在本项目中，RHI 全面拥抱现代跨平台高性能图形标准 **Vulkan 1.2+**，整合了 `vk-bootstrap`（自动化安全初始引导）与 `Vulkan Memory Allocator (VMA)`（工业级显存分配器），负责管理物理窗口表面创建、最优 GPU 选择、逻辑设备构建、图形/呈现队列调度与全生命周期安全销毁。

---

## 2. 核心架构设计与层级分工（Owner vs Borrowed View）

### 2.1 为什么既有 `VulkanBackend` 又有 `VulkanContext`？
- **`VulkanBackend` 是“物理硬件宿主与资源独占所有者（Owner）”**：
  - 它全局独占创建并持有唯一的 OS 物理窗口（`GLFWwindow*`）；
  - 拥有 Vulkan 实例（`VkInstance`）、物理显卡（`VkPhysicalDevice`）、逻辑设备（`VkDevice`）、绘制表面（`VkSurfaceKHR`）和显存分配器（`VmaAllocator`）；
  - 负责物理生命周期管控（`initialize`、`begin_frame`、`wait_idle`、`shutdown`）。
- **`VulkanContext` 是“非拥有型的句柄切片视图（Borrowed View）”**：
  - 它是一个轻量级的 POD 聚合体，**不持有任何资源的析构所有权**，仅存放上述核心原生句柄的指针/值拷贝；
  - **核心解耦价值**：渲染服务（如 `SceneRenderer`）、UI 中间件（如 `ImGuiBackend`）以及离线测试用例，**完全不需要也不应该依赖庞大的物理窗口或 GLFW 驱动**，只需要借用 `VulkanContext` 即可完成显存分配、指令录制与队列提交。

### 2.2 上层到底应该暴露什么？
- **对应用宿主层（Application）**：`Application` 只需要面向 `VulkanBackend` 即可（统一由 `VulkanBackend` 协调生命周期），并将 `VulkanBackend` 整体传递给 `ImGuiBackend`，**不再需要 Application 手动解构并暴露底层的 `GLFWwindow*` 和 `VulkanContext`**；
- **对渲染器与离屏管道（Renderer/Tests）**：直接借用 `VulkanContext`，实现彻底的高内聚与低耦合。

---

## 3. 文件清单与深度解析

### 3.1 `vma_resources.h`
* **定位**：基于 AMD VMA 的高级显存资源封装器。
* **主要内容**：
  * `AllocatedBuffer`：封装 `VkBuffer`、`VmaAllocation` 与分配元数据。
    * 提供 `create(...)` 静态工厂，一行代码完成显存属性配置与缓冲区构建；
    * 提供 `map(...)` 与 `unmap(...)`，实现主机可见显存的安全映射与数据更新；
    * 提供 `is_valid()` 与 `destroy(...)` 安全析构。
  * `AllocatedImage`：封装 `VkImage`、`VkImageView`、`VmaAllocation` 与维度格式信息，支持离屏纹理与深度缓冲的高性能显存池分配。

### 3.2 `vulkan/vulkan_context.h`
* **定位**：Vulkan 共享硬件句柄上下文（非拥有视图）。
* **成员变量**：
  * `instance`：Vulkan 驱动与应用实例句柄；
  * `physical_device`：选定的物理显卡（独显优先）；
  * `device`：逻辑设备（Logical Device）；
  * `graphics_queue` & `graphics_queue_family`：图形命令执行队列与族索引；
  * `present_queue` & `present_queue_family`：屏幕呈现队列与族索引；
  * `allocator`：内存分配器回调（通常为 nullptr）；
  * `surface`：GLFW 关联的 `VkSurfaceKHR` 物理窗口表面；
  * `vma_allocator`：全局统一 VMA 显存分配器；
* **校验方法**：
  * `is_valid()`：状态完整性断言自检函数，确保任何渲染器在使用该上下文之前句柄全部就绪。

### 3.3 `vulkan/vulkan_backend.h` / `vulkan_backend.cpp`
* **定位**：物理窗口与 Vulkan 设备全生命周期宿主。
* **主要成员变量**：
  * `impl_`：PImpl 私有结构体，隔离 `vk-bootstrap` 内部类型；
  * `window_`：唯一持有的物理 GLFW 操作系统窗口指针；
  * `context_`：统一维护并供外部借用的 `VulkanContext` 切片；
  * `glfw_initialized_` / `initialized_`：各阶段初始化状态标志。
* **核心功能**：
  * **GLFW 窗口构建**：设置 `GLFW_CLIENT_API, GLFW_NO_API` 并支持透明帧缓冲与 DPI 感知缩放；
  * **自动化安全引导（VkBootstrap）**：
    * 启动 Vulkan 验证层（Validation Layers，在 Debug 模式自动启用 Debug Messenger）；
    * 智能物理设备选择（自动筛选满足 Swapchain 支持、API 1.2、离散显卡优先的 GPU）；
    * 提取匹配的 Graphics 和 Present 队列族；
  * **统一显存与图像工厂**：提供 `create_buffer(...)` 与 `create_image(...)`，让上层无需直接接触裸 Vulkan API 即可分配 GPU 资源；
  * **主帧心跳与事件分发**：
    * `begin_frame()`：轮询 GLFW 窗口事件（`glfwPollEvents`），若窗口最小化自动休眠降频；
    * `wait_idle()`：阻塞同步 GPU 队列；
    * `shutdown()`：在销毁设备前等待空闲，严格按照 `VMA -> Device -> Surface -> Instance -> Window` 逆序彻底释放资源，杜绝句柄悬垂与显存泄漏。
