#include <engine/rhi/vulkan/vulkan_backend.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
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

}

struct VulkanBackend::Impl
{
    vkb::Instance vkb_instance;
    vkb::Device vkb_device;
};

VulkanBackend& VulkanBackend::instance()
{
    static VulkanBackend backend;
    return backend;
}

VulkanBackend::VulkanBackend()
    : impl_(std::make_unique<Impl>())
{
}

VulkanBackend::~VulkanBackend()
{
    shutdown();
}

bool VulkanBackend::initialize(const char* title, int width, int height)
{
    if (initialized_)
    {
        return false;
    }

    // 1. 初始化 GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return false;
    }
    glfw_initialized_ = true;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    float content_scale_x = 1.0f;
    float content_scale_y = 1.0f;
    glfwGetMonitorContentScale(
        glfwGetPrimaryMonitor(),
        &content_scale_x,
        &content_scale_y);

    window_ = glfwCreateWindow(
        static_cast<int>(static_cast<float>(width) * std::max(content_scale_x, 1.0f)),
        static_cast<int>(static_cast<float>(height) * std::max(content_scale_y, 1.0f)),
        title,
        nullptr,
        nullptr);
    if (window_ == nullptr)
    {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        shutdown();
        return false;
    }

    // 2. 使用 vk-bootstrap 构建 Instance
    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder
        .set_app_name(title)
        .set_engine_name("Slrye")
        .require_api_version(1, 2, 0)
#if defined(_DEBUG)
        .request_validation_layers(true)
        .use_default_debug_messenger()
#endif
        .build();

    if (!instance_ret)
    {
        std::fprintf(stderr, "Failed to create Vulkan instance: %s\n", instance_ret.error().message().c_str());
        shutdown();
        return false;
    }
    impl_->vkb_instance = instance_ret.value();
    context_.instance = impl_->vkb_instance.instance;

    // 3. 创建 Window Surface
    if (glfwCreateWindowSurface(context_.instance, window_, nullptr, &context_.surface) != VK_SUCCESS)
    {
        std::fprintf(stderr, "Failed to create window surface\n");
        shutdown();
        return false;
    }

    // 4. 使用 vk-bootstrap 选择最优物理设备（优选独显、满足 Swapchain）
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
        shutdown();
        return false;
    }
    vkb::PhysicalDevice vkb_phys_device = phys_ret.value();
    context_.physical_device = vkb_phys_device.physical_device;

    // 5. 使用 vk-bootstrap 创建逻辑设备并提取队列
    vkb::DeviceBuilder device_builder{vkb_phys_device};
    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
        std::fprintf(stderr, "Failed to create logical device: %s\n", dev_ret.error().message().c_str());
        shutdown();
        return false;
    }
    impl_->vkb_device = dev_ret.value();
    context_.device = impl_->vkb_device.device;

    auto graphics_queue_ret = impl_->vkb_device.get_queue(vkb::QueueType::graphics);
    auto graphics_family_ret = impl_->vkb_device.get_queue_index(vkb::QueueType::graphics);
    if (!graphics_queue_ret || !graphics_family_ret)
    {
        std::fprintf(stderr, "Failed to get graphics queue\n");
        shutdown();
        return false;
    }
    context_.graphics_queue = graphics_queue_ret.value();
    context_.graphics_queue_family = graphics_family_ret.value();

    auto present_queue_ret = impl_->vkb_device.get_queue(vkb::QueueType::present);
    auto present_family_ret = impl_->vkb_device.get_queue_index(vkb::QueueType::present);
    if (present_queue_ret && present_family_ret)
    {
        context_.present_queue = present_queue_ret.value();
        context_.present_queue_family = present_family_ret.value();
    }
    else
    {
        context_.present_queue = context_.graphics_queue;
        context_.present_queue_family = context_.graphics_queue_family;
    }

    // 6. 初始化 VMA 统一内存分配器
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.physicalDevice = context_.physical_device;
    allocator_info.device = context_.device;
    allocator_info.instance = context_.instance;

    if (vmaCreateAllocator(&allocator_info, &context_.vma_allocator) != VK_SUCCESS)
    {
        std::fprintf(stderr, "Failed to initialize VMA allocator\n");
        shutdown();
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

    if (context_.vma_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(context_.vma_allocator);
        context_.vma_allocator = VK_NULL_HANDLE;
    }

    if (impl_)
    {
        if (context_.device != VK_NULL_HANDLE)
        {
            vkb::destroy_device(impl_->vkb_device);
            context_.device = VK_NULL_HANDLE;
            impl_->vkb_device = {};
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
            impl_->vkb_instance = {};
        }
    }

    context_ = {};

    if (window_ != nullptr)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if (glfw_initialized_)
    {
        glfwTerminate();
        glfw_initialized_ = false;
    }

    initialized_ = false;
}

GLFWwindow* VulkanBackend::window() const noexcept
{
    return window_;
}

const VulkanContext& VulkanBackend::context() const noexcept
{
    return context_;
}

}
