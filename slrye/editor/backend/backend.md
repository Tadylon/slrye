# Editor Backend 模块 (`slrye/editor/backend`)

## 1. 模块定位与职责
`backend` 模块负责沟通底层 Vulkan 硬件平台与上层 Dear ImGui UI 渲染系统。它在设计上作为 **“借用者 / 中间件”**，借用 `VulkanBackend` 持有的 OS 窗口与核心设备句柄，独立管理 Dear ImGui 专属的 Vulkan 主交换链（Swapchain）、主 RenderPass、Framebuffer、CommandBuffer、专有描述符池（DescriptorPool）以及双信号量同步与栅栏对象（Fences）。

---

## 2. 为什么 `ImGuiBackend` 会有 `GLFWwindow*` 和 `VulkanContext` 指针？是否重复创建？

### 2.1 绝无重复创建窗口
- **整个应用程序自始至终只有一个物理窗口**（由 `VulkanBackend` 独占创建与销毁）；
- `ImGuiBackend` 中的 `GLFWwindow* window_` 是一个**非拥有型的借用裸指针**；
- **为什么必须持有该指针？**
  Dear ImGui 具备平台无关性，它通过 `imgui_impl_glfw.cpp` 适配器捕获操作系统的鼠标移动、点击、滚轮、按键、剪贴板与焦点事件。`ImGui_ImplGlfw_InitForVulkan(window_, true)` 必须传入现有的 `GLFWwindow*` 才能在窗口上安装事件回调钩子。

### 2.2 为什么需要借用 `VulkanContext`？
- ImGui 的最终 UI 元素需要绘制在物理窗口对应的 Vulkan 主交换链图像上，并提交至图形队列（`vkQueueSubmit`）再呈现给显示器（`vkQueuePresentKHR`）；
- `context_` 提供了所需的 `VkDevice`、`VkQueue` 与 `VkSurfaceKHR`，ImGuiBackend 只借用这些句柄，不负责它们的销毁。

### 2.3 接口封装收敛
- 现已提供 `ImGuiBackend::initialize(const engine::VulkanBackend& backend)` 高层重载；
- 上层调用者（`Application`）无需再手动拆解 `backend.window()` 与 `backend.context()`，整体架构实现了高内聚。

---

## 3. 文件清单与深度解析

### 3.1 `imgui_backend.h` / `imgui_backend.cpp`
* **定位**：Dear ImGui Vulkan/GLFW 渲染适配后端。
* **主要成员变量**：
  * `context_`：借用的 `VulkanContext` 句柄集（非拥有，供交换链和管线使用）；
  * `window_`：借用的 `GLFWwindow*` 窗口指针（非拥有，供输入钩子使用）；
  * `descriptor_pool_`：ImGui 专用描述符池（用于字体和 Viewport 纹理分配）；
  * `window_data_`：`ImGui_ImplVulkanH_Window`，官方封装的主交换链、RenderPass、帧缓冲与信号量集合；
  * `swapchain_rebuild_`：交换链重建标志；
  * `initialized_`：初始化状态。
* **主要工作流程**：
  * **专属描述符池构建 (`create_imgui_descriptor_pool`)**：
    创建带 `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT` 标志的高容量描述符池（支持 1000 个采样图像与 680 个采样器），供 ImGui 字体纹理及视口面板绑定的 `ImTextureID` 动态分配；
  * **主窗口交换链管理 (`setup_vulkan_window` 与 `ensure_swapchain`)**：
    * 提取物理表面格式与最佳呈现模式（`VK_PRESENT_MODE_FIFO_KHR` 等）；
    * 采用 `ImGui_ImplVulkanH_CreateOrResizeWindow` 创建与窗口尺寸匹配的交换链图像、视图、渲染通道与帧缓冲；
    * 支持窗口尺寸改变（Resize）或最小化时的安全重建；
  * **双缓冲流水线与命令提交 (`render_frame` 与 `present_frame`)**：
    * `render_frame`：执行 `vkAcquireNextImageKHR` 申请可用交换链图像，通过 Fence 等待上一帧完毕并重置命令池，开启 RenderPass 记录 `ImGui_ImplVulkan_RenderDrawData`，并在信号量保护下提交至图形队列；
    * `present_frame`：通过 `vkQueuePresentKHR` 呈现渲染完毕的最终 UI 画布，并推进信号量循环索引；
  * **优雅关闭 (`shutdown`)**：在释放资源前调用 `vkDeviceWaitIdle`，安全清理描述符池、ImGui 内部上下文、GLFW 钩子与交换链资源。
