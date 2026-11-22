
# Core

`VulkanBackend` 独占 GLFW 窗口以及 Vulkan Instance、Surface、Physical/Logical Device 和 Queue。`VulkanContext` 只是这些共享句柄的非拥有视图，供渲染服务与 ImGui 后端借用。

`ImGuiBackend` 借用 `VulkanContext`，独占 Dear ImGui 专用 Vulkan 资源：DescriptorPool、Swapchain、RenderPass、Framebuffer、CommandBuffer 和同步对象。销毁顺序固定为 UI、插件、ImGui 后端、Vulkan 后端，确保 ImGui 资源先于共享设备释放。
