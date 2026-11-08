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
    if (initialized_)
    {
        return false;
    }

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
        shutdown();
        return false;
    }

    if (!glfwVulkanSupported())
    {
        std::fprintf(stderr, "GLFW: Vulkan is not supported\n");
        shutdown();
        return false;
    }

    std::uint32_t extension_count = 0;
    const char** required_extensions =
        glfwGetRequiredInstanceExtensions(&extension_count);
    if (required_extensions == nullptr)
    {
        shutdown();
        return false;
    }

    std::vector<const char*> extensions(
        required_extensions,
        required_extensions + extension_count);
    std::vector<const char*> layers;
#if defined(_DEBUG)
    constexpr const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    if (has_layer(validation_layer))
    {
        layers.push_back(validation_layer);
        if (has_instance_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        {
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
    }
#endif

    VkApplicationInfo application_info{};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = title;
    application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.pEngineName = "Slrye";
    application_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &application_info;
    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();
    instance_info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    instance_info.ppEnabledLayerNames = layers.data();
    if (!check_result(
            vkCreateInstance(&instance_info, context_.allocator, &context_.instance),
            "vkCreateInstance"))
    {
        shutdown();
        return false;
    }

#if defined(_DEBUG)
    if (std::find(layers.begin(), layers.end(), validation_layer) != layers.end())
    {
        const auto create_debug_report =
            reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(context_.instance, "vkCreateDebugReportCallbackEXT"));
        if (create_debug_report != nullptr)
        {
            VkDebugReportCallbackCreateInfoEXT callback_info{};
            callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            callback_info.flags =
                VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            callback_info.pfnCallback = debug_report_callback;
            check_result(
                create_debug_report(
                    context_.instance,
                    &callback_info,
                    context_.allocator,
                    &debug_report_),
                "vkCreateDebugReportCallbackEXT");
        }
    }
#endif

    if (!check_result(
            glfwCreateWindowSurface(
                context_.instance,
                window_,
                context_.allocator,
                &context_.surface),
            "glfwCreateWindowSurface"))
    {
        shutdown();
        return false;
    }

    const std::optional<VkPhysicalDevice> physical_device =
        select_physical_device(
            context_.instance,
            context_.surface,
            context_.graphics_queue_family);
    if (!physical_device.has_value())
    {
        std::fprintf(stderr, "No Vulkan device supports graphics, presentation, and swapchain\n");
        shutdown();
        return false;
    }
    context_.physical_device = *physical_device;

    std::vector<const char*> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
    if (has_device_extension(
            context_.physical_device,
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
    {
        device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
#endif

    constexpr float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = context_.graphics_queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    if (!check_result(
            vkCreateDevice(
                context_.physical_device,
                &device_info,
                context_.allocator,
                &context_.device),
            "vkCreateDevice"))
    {
        shutdown();
        return false;
    }

    vkGetDeviceQueue(
        context_.device,
        context_.graphics_queue_family,
        0,
        &context_.graphics_queue);
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

    if (context_.device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(context_.device, context_.allocator);
        context_.device = VK_NULL_HANDLE;
    }

#if defined(_DEBUG)
    if (context_.instance != VK_NULL_HANDLE && debug_report_ != VK_NULL_HANDLE)
    {
        const auto destroy_debug_report =
            reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(context_.instance, "vkDestroyDebugReportCallbackEXT"));
        if (destroy_debug_report != nullptr)
        {
            destroy_debug_report(
                context_.instance,
                debug_report_,
                context_.allocator);
        }
        debug_report_ = VK_NULL_HANDLE;
    }
#endif

    if (context_.instance != VK_NULL_HANDLE && context_.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context_.instance, context_.surface, context_.allocator);
        context_.surface = VK_NULL_HANDLE;
    }
    if (context_.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(context_.instance, context_.allocator);
        context_.instance = VK_NULL_HANDLE;
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
