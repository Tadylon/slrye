# Engine RHI 模块 (`slrye/engine/rhi`)

## 1. 模块定位与职责
`rhi`（Rendering Hardware Interface，渲染硬件接口）是引擎与物理显卡及底层图形 API 交互的基石。在本项目中，RHI 全面拥抱现代现代跨平台高性能图形标准 **Vulkan 1.2+**，整合了 `vk-bootstrap`（自动化安全初始引导）与 `Vulkan Memory Allocator (VMA)`（工业级显存分配器），负责管理物理窗口表面创建、最优 GPU 选择、逻辑设备构建、图形/呈现队列调度与全生命周期安全销毁。

---

## 2. 文件清单与深度解析

### 2.1 `vma_resources.h`
* **定位**：基于 AMD VMA 的高级资源封装器。
* **主要内容**：
  * `AllocatedBuffer`：封装 `VkBuffer`、`VmaAllocation` 与分配元数据。
    * 提供 `create(...)` 静态工厂，一行代码完成显存属性配置与缓冲区构建；
    * 提供 `map(...)` 与 `unmap(...)`，实现主机可见显存的安全映射与数据更新；
    * 提供 `destroy(...)` 安全析构。
  * `AllocatedImage`：封装 `VkImage`、`VkImageView`、`VmaAllocation` 与维度格式信息，支持离屏纹理与深度缓冲的高性能显存池分配。

### 2.2 `vulkan/vulkan_context.h`
* **定位**：Vulkan 共享硬件句柄上下文（非拥有视图）。
* **主要内容**：
  * `VulkanContext` 结构体：汇集当前图形系统运行所需的一切核心句柄：
    * `instance`：Vulkan 实例；
    * `physical_device`：选定的物理显卡（独显优先）；
    * `device`：逻辑设备（Logical Device）；
    * `graphics_queue` & `graphics_queue_family`：图形命令执行队列；
    * `present_queue` & `present_queue_family`：呈现队列；
    * `surface`：GLFW 关联的 `VkSurfaceKHR` 窗口表面；
    * `vma_allocator`：全局统一 VMA 显存分配器；
  * `is_valid()`：状态完整性断言自检函数，确保任何渲染器在使用该上下文之前句柄全部就绪。

### 2.3 `vulkan/vulkan_backend.h` / `vulkan_backend.cpp`
* **定位**：Vulkan 基础设施单例生命周期控制器。
* **主要内容**：
  * **GLFW 窗口构建**：设置 `GLFW_CLIENT_API, GLFW_NO_API` 并支持透明帧缓冲与 DPI 感知缩放；
  * **自动化安全引导（VkBootstrap）**：
    * 启动 Vulkan 验证层（Validation Layers，在 Debug 模式自动启用 Debug Messenger）；
    * 智能物理设备选择（自动筛选满足 Swapchain 支持、API 1.2、离散显卡优先的 GPU）；
    * 提取匹配的 Graphics 和 Present 队列族；
  * **初始化 VMA 分配器**：构建全局内存池；
  * **主帧心跳与事件分发**：
    * `begin_frame()`：轮询 GLFW 窗口事件（`glfwPollEvents`），若窗口最小化自动休眠降频；
    * `shutdown()`：在销毁设备前调用 `vkDeviceWaitIdle`，严格按照 `VMA -> Device -> Surface -> Instance -> Window` 逆序彻底释放资源，杜绝句柄悬垂与内存泄漏。
