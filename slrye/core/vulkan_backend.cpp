#include <core/vulkan_backend.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <thread>
#include <vector>

namespace slrye
{
namespace
{

void glfw_error_callback(int error, const char* description)
{
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

bool check_result(VkResult result, const char* operation)
{
    if (result == VK_SUCCESS)
    {
        return true;
    }
    std::fprintf(stderr, "%s failed with VkResult %d\n", operation, static_cast<int>(result));
    return false;
}

bool has_instance_extension(const char* name)
{
    std::uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS)
    {
        return false;
    }
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [name](const VkExtensionProperties& extension)
        {
            return std::strcmp(extension.extensionName, name) == 0;
        });
}

bool has_layer(const char* name)
{
    std::uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkLayerProperties> layers(count);
    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS)
    {
        return false;
    }
    return std::any_of(
        layers.begin(),
        layers.end(),
        [name](const VkLayerProperties& layer)
        {
            return std::strcmp(layer.layerName, name) == 0;
        });
}

bool has_device_extension(VkPhysicalDevice device, const char* name)
{
    std::uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS)
    {
        return false;
    }
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [name](const VkExtensionProperties& extension)
        {
            return std::strcmp(extension.extensionName, name) == 0;
        });
}

std::optional<std::uint32_t> find_graphics_present_queue(
    VkPhysicalDevice device,
    VkSurfaceKHR surface)
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (std::uint32_t index = 0; index < count; ++index)
    {
        if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
        {
            continue;
        }

        VkBool32 present_supported = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &present_supported) == VK_SUCCESS &&
            present_supported == VK_TRUE)
        {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<VkPhysicalDevice> select_physical_device(
    VkInstance instance,
    VkSurfaceKHR surface,
    std::uint32_t& queue_family)
{
    std::uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS || count == 0)
    {
        return std::nullopt;
    }

    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(instance, &count, devices.data()) != VK_SUCCESS)
    {
        return std::nullopt;
    }

    for (VkPhysicalDevice device : devices)
    {
        const std::optional<std::uint32_t> family =
            find_graphics_present_queue(device, surface);
        if (family.has_value() &&
            has_device_extension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            queue_family = *family;
            return device;
        }
    }
    return std::nullopt;
}

#if defined(_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(
    VkDebugReportFlagsEXT,
    VkDebugReportObjectTypeEXT,
    std::uint64_t,
    std::size_t,
    std::int32_t,
    const char* layer_prefix,
    const char* message,
    void*)
{
    std::fprintf(
        stderr,
        "[vulkan] %s: %s\n",
        layer_prefix != nullptr ? layer_prefix : "validation",
        message != nullptr ? message : "");
    return VK_FALSE;
}
#endif

}

VulkanBackend& VulkanBackend::instance()
{
    static VulkanBackend backend;
    return backend;
}

VulkanBackend::VulkanBackend() = default;

VulkanBackend::~VulkanBackend()
{
    shutdown();
}

bool VulkanBackend::initialize(const char* title, int width, int height)
{
    // 1. 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    // 2. 使用 vk-bootstrap 构建 Instance（启用现代 Debug Utils）
    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder
        .set_app_name(title)
        .set_engine_name("Slrye")
        .require_api_version(1, 3, 0) // 或 1.2.0
#if defined(_DEBUG)
        .request_validation_layers(true)
        .use_default_debug_messenger()
#endif
        .build();
    if (!instance_ret)
    {
        std::fprintf(stderr, "Failed to create Vulkan instance: %s\n", instance_ret.error().message().c_str());
        return false;
    }
    impl_->vkb_instance = instance_ret.value();
    context_.instance = impl_->vkb_instance.instance;
    // 3. 创建 Window Surface
    if (glfwCreateWindowSurface(context_.instance, window_, nullptr, &context_.surface) != VK_SUCCESS)
    {
        return false;
    }
    // 4. 使用 vk-bootstrap 选择最优物理设备（优先选择独立显卡，自动满足 Swapchain 扩展）
    vkb::PhysicalDeviceSelector selector{impl_->vkb_instance};
    auto phys_ret = selector
        .set_surface(context_.surface)
        .set_minimum_version(1, 2)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .select();
    if (!phys_ret)
    {
        std::fprintf(stderr, "Failed to select physical device: %s\n", phys_ret.error().message().c_str());
        return false;
    }
    vkb::PhysicalDevice vkb_phys_device = phys_ret.value();
    context_.physical_device = vkb_phys_device.physical_device;
    // 5. 使用 vk-bootstrap 创建逻辑设备
    vkb::DeviceBuilder device_builder{vkb_phys_device};
    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
        std::fprintf(stderr, "Failed to create logical device: %s\n", dev_ret.error().message().c_str());
        return false;
    }
    impl_->vkb_device = dev_ret.value();
    context_.device = impl_->vkb_device.device;
    // 获取图形与呈现队列
    context_.graphics_queue = impl_->vkb_device.get_queue(vkb::QueueType::graphics).value();
    context_.graphics_queue_family = impl_->vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    context_.present_queue = impl_->vkb_device.get_queue(vkb::QueueType::present).value();
    context_.present_queue_family = impl_->vkb_device.get_queue_index(vkb::QueueType::present).value();
    // 6. 初始化 VMA (Vulkan Memory Allocator)
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice = context_.physical_device;
    allocator_info.device = context_.device;
    allocator_info.instance = context_.instance;
    // 如果启用了 Buffer Device Address 等特性，可以配置 flags:
    // allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    if (vmaCreateAllocator(&allocator_info, &context_.vma_allocator) != VK_SUCCESS)
    {
        std::fprintf(stderr, "Failed to initialize VMA allocator\n");
        return false;
    }
    initialized_ = true;
    return true;
}

bool VulkanBackend::should_close() const
{
    return window_ != nullptr && glfwWindowShouldClose(window_) != 0;
}

bool VulkanBackend::begin_frame()
{
    if (!initialized_)
    {
        return false;
    }

    glfwPollEvents();
    if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return false;
    }
    return true;
}

void VulkanBackend::shutdown()
{
    if (context_.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(context_.device);
    }
    // 先销毁 VMA 分配器
    if (context_.vma_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(context_.vma_allocator);
        context_.vma_allocator = VK_NULL_HANDLE;
    }
    // 使用 vkb 安全销毁设备与实例
    if (context_.device != VK_NULL_HANDLE)
    {
        vkb::destroy_device(impl_->vkb_device);
        context_.device = VK_NULL_HANDLE;
    }
    if (context_.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context_.instance, context_.surface, nullptr);
        context_.surface = VK_NULL_HANDLE;
    }
    if (context_.instance != VK_NULL_HANDLE)
    {
        vkb::destroy_instance(impl_->vkb_instance);
        context_.instance = VK_NULL_HANDLE;
    }
    if (window_ != nullptr)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

}
