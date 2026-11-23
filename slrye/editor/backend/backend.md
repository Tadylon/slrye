# Editor Backend 模块 (`slrye/editor/backend`)

## 1. 模块定位与职责
`backend` 模块负责沟通底层 Vulkan 硬件平台与上层 Dear ImGui UI 渲染系统。它借用 `VulkanContext` 中的物理设备、逻辑设备与队列，独立管理 Dear ImGui 专属的 Vulkan Swapchain、RenderPass、Framebuffer、CommandBuffer、专有描述符池（DescriptorPool）以及双信号量同步与栅栏对象（Fences）。

---

## 2. 文件清单与深度解析

### 2.1 `imgui_backend.h` / `imgui_backend.cpp`
* **定位**：Dear ImGui Vulkan/GLFW 渲染适配后端单例。
* **主要内容**：
  * **专属描述符池构建 (`create_imgui_descriptor_pool`)**：
    创建带 `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT` 标志的高容量描述符池（支持 1000 个采样图像与 680 个采样器），供 ImGui 字体纹理及视口面板绑定的 `ImTextureID` 动态分配；
  * **主窗口交换链管理 (`setup_vulkan_window` 与 `ensure_swapchain`)**：
    * 提取物理表面格式与最佳呈现模式（`VK_PRESENT_MODE_FIFO_KHR` 等）；
    * 采用 `ImGui_ImplVulkanH_CreateOrResizeWindow` 创建与窗口尺寸匹配的交换链图像、视图、渲染通道与帧缓冲；
    * 支持窗口尺寸改变（Resize）或最小化时的安全重建；
  * **双缓冲流水线与命令提交 (`render_frame` 与 `present_frame`)**：
    * `render_frame`：执行 `vkAcquireNextImageKHR` 申请可用交换链图像，通过 Fence 等待上一帧完毕并重置命令池，开启 RenderPass 记录 `ImGui_ImplVulkan_RenderDrawData`，并在信号量保护下提交至图形队列；
    * `present_frame`：通过 `vkQueuePresentKHR` 呈现渲染完毕的最终 UI 画布，并推进信号量循环索膜；
  * **优雅关闭 (`shutdown`)**：在释放资源前调用 `vkDeviceWaitIdle`，安全清理描述符池、ImGui 内部上下文、GLFW 钩子与交换链资源。
