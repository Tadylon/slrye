#include <services/rendering/scene_renderer.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

#include <glm/mat4x4.hpp>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include <core/vulkan_context.h>
#include <domain/camera.h>
#include <domain/scene.h>
#include <platform/file_io.h>
#include <services/rendering/scene_geometry.h>

namespace slrye
{
namespace
{

constexpr std::uint32_t kMaximumViewportDimension = 8192;

void set_error(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

bool check_result(VkResult result, const char* operation, std::string* error)
{
    if (result == VK_SUCCESS)
    {
        return true;
    }
    set_error(error, std::string(operation) + " failed with VkResult " + std::to_string(result));
    return false;
}

}

class SceneRenderer::Impl
{
public:
    bool initialize(
        const VulkanContext& new_context,
        const std::filesystem::path& shader_directory,
        std::string* error);
    bool render(
        const Scene& scene,
        const PerspectiveCamera& camera,
        std::uint32_t requested_width,
        std::uint32_t requested_height,
        std::optional<EntityId> selected_entity,
        bool show_grid,
        std::string* error);
    void shutdown();

    [[nodiscard]] ImTextureID texture_id() const noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

private:
    bool create_shader_module(
        const std::filesystem::path& path,
        VkShaderModule& shader_module,
        std::string* error);
    bool select_formats(std::string* error);
    bool create_render_pass(std::string* error);
    bool create_pipeline(const std::filesystem::path& shader_directory, std::string* error);
    bool create_command_resources(std::string* error);
    bool create_render_target(std::uint32_t width, std::uint32_t height, std::string* error);
    void destroy_render_target() noexcept;
    bool ensure_render_target(std::uint32_t width, std::uint32_t height, std::string* error);
    bool ensure_vertex_capacity(VkDeviceSize required_size, std::string* error);
    bool upload_geometry(const SceneGeometry& geometry, std::string* error);
    bool record_and_submit(const PerspectiveCamera& camera, std::size_t vertex_count, std::string* error);
    std::uint32_t find_memory_type(
        std::uint32_t type_filter,
        VkMemoryPropertyFlags properties) const noexcept;
    bool create_image(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImage& image,
        VkDeviceMemory& memory,
        std::string* error);
    bool create_image_view(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect,
        VkImageView& image_view,
        std::string* error);

    VulkanContext context_;
    VkFormat color_format_ = VK_FORMAT_UNDEFINED;
    VkFormat depth_format_ = VK_FORMAT_UNDEFINED;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkImage color_image_ = VK_NULL_HANDLE;
    VkDeviceMemory color_memory_ = VK_NULL_HANDLE;
    VkImageView color_view_ = VK_NULL_HANDLE;
    VkImage depth_image_ = VK_NULL_HANDLE;
    VkDeviceMemory depth_memory_ = VK_NULL_HANDLE;
    VkImageView depth_view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkDescriptorSet texture_descriptor_ = VK_NULL_HANDLE;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory_ = VK_NULL_HANDLE;
    void* mapped_vertices_ = nullptr;
    VkDeviceSize vertex_capacity_ = 0;
    bool initialized_ = false;
};

bool SceneRenderer::Impl::initialize(
    const VulkanContext& new_context,
    const std::filesystem::path& shader_directory,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    if (initialized_)
    {
        set_error(error, "Scene renderer is already initialized");
        return false;
    }
    if (!new_context.is_valid())
    {
        set_error(error, "Scene renderer requires an initialized Vulkan context");
        return false;
    }

    context_ = new_context;
    if (!select_formats(error) ||
        !create_render_pass(error) ||
        !create_pipeline(shader_directory, error) ||
        !create_command_resources(error))
    {
        shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

bool SceneRenderer::Impl::render(
    const Scene& scene,
    const PerspectiveCamera& camera,
    std::uint32_t requested_width,
    std::uint32_t requested_height,
    std::optional<EntityId> selected_entity,
    bool show_grid,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    if (!initialized_)
    {
        set_error(error, "Scene renderer is not initialized");
        return false;
    }

    const std::uint32_t width = std::clamp(requested_width, 1u, kMaximumViewportDimension);
    const std::uint32_t height = std::clamp(requested_height, 1u, kMaximumViewportDimension);
    if (!ensure_render_target(width, height, error))
    {
        return false;
    }

    const SceneGeometry geometry = build_scene_geometry(scene, selected_entity, show_grid);
    return upload_geometry(geometry, error) &&
        record_and_submit(camera, geometry.vertices.size(), error);
}

void SceneRenderer::Impl::shutdown()
{
    if (context_.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(context_.device);
        destroy_render_target();

        if (mapped_vertices_ != nullptr)
        {
            vkUnmapMemory(context_.device, vertex_memory_);
            mapped_vertices_ = nullptr;
        }
        if (vertex_buffer_ != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(context_.device, vertex_buffer_, context_.allocator);
            vertex_buffer_ = VK_NULL_HANDLE;
        }
        if (vertex_memory_ != VK_NULL_HANDLE)
        {
            vkFreeMemory(context_.device, vertex_memory_, context_.allocator);
            vertex_memory_ = VK_NULL_HANDLE;
        }
        if (command_pool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(context_.device, command_pool_, context_.allocator);
            command_pool_ = VK_NULL_HANDLE;
            command_buffer_ = VK_NULL_HANDLE;
        }
        if (pipeline_ != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(context_.device, pipeline_, context_.allocator);
            pipeline_ = VK_NULL_HANDLE;
        }
        if (pipeline_layout_ != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(context_.device, pipeline_layout_, context_.allocator);
            pipeline_layout_ = VK_NULL_HANDLE;
        }
        if (render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(context_.device, render_pass_, context_.allocator);
            render_pass_ = VK_NULL_HANDLE;
        }
    }

    vertex_capacity_ = 0;
    context_ = {};
    initialized_ = false;
}

ImTextureID SceneRenderer::Impl::texture_id() const noexcept
{
    return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(texture_descriptor_));
}

bool SceneRenderer::Impl::is_initialized() const noexcept
{
    return initialized_;
}

}

namespace slrye
{

bool SceneRenderer::Impl::create_shader_module(
    const std::filesystem::path& path,
    VkShaderModule& shader_module,
    std::string* error)
{
    std::string code;
    if (!platform::read_text_file(path, code, error))
    {
        return false;
    }
    if (code.empty() || code.size() % sizeof(std::uint32_t) != 0)
    {
        set_error(error, "Shader file is empty or not valid SPIR-V: " + path.string());
        return false;
    }

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
    return check_result(
        vkCreateShaderModule(
            context_.device,
            &create_info,
            context_.allocator,
            &shader_module),
        "vkCreateShaderModule",
        error);
}

bool SceneRenderer::Impl::select_formats(std::string* error)
{
    const std::array<VkFormat, 2> color_candidates{
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM};
    for (VkFormat format : color_candidates)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(context_.physical_device, format, &properties);
        const VkFormatFeatureFlags required =
            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if ((properties.optimalTilingFeatures & required) == required)
        {
            color_format_ = format;
            break;
        }
    }
    if (color_format_ == VK_FORMAT_UNDEFINED)
    {
        set_error(error, "No supported color format is available for the scene viewport");
        return false;
    }

    const std::array<VkFormat, 3> depth_candidates{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};
    for (VkFormat format : depth_candidates)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(context_.physical_device, format, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
        {
            depth_format_ = format;
            break;
        }
    }
    if (depth_format_ == VK_FORMAT_UNDEFINED)
    {
        set_error(error, "No supported depth format is available for the scene viewport");
        return false;
    }

    return true;
}

bool SceneRenderer::Impl::create_render_pass(std::string* error)
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = color_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = depth_format_;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = dependencies[0].srcStageMask;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    const std::array<VkAttachmentDescription, 2> attachments{
        color_attachment,
        depth_attachment};
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
    create_info.pDependencies = dependencies.data();
    return check_result(
        vkCreateRenderPass(
            context_.device,
            &create_info,
            context_.allocator,
            &render_pass_),
        "vkCreateRenderPass",
        error);
}

}

namespace slrye
{

bool SceneRenderer::Impl::create_pipeline(
    const std::filesystem::path& shader_directory,
    std::string* error)
{
    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;
    if (!create_shader_module(shader_directory / "viewport.vert.spv", vertex_shader, error))
    {
        return false;
    }
    if (!create_shader_module(shader_directory / "viewport.frag.spv", fragment_shader, error))
    {
        vkDestroyShaderModule(context_.device, vertex_shader, context_.allocator);
        return false;
    }

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant.size = sizeof(glm::mat4);
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;
    if (!check_result(
            vkCreatePipelineLayout(
                context_.device,
                &layout_info,
                context_.allocator,
                &pipeline_layout_),
            "vkCreatePipelineLayout",
            error))
    {
        vkDestroyShaderModule(context_.device, vertex_shader, context_.allocator);
        vkDestroyShaderModule(context_.device, fragment_shader, context_.allocator);
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex_shader;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment_shader;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(SceneVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    std::array<VkVertexInputAttributeDescription, 3> attributes{};
    attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, position)};
    attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, normal)};
    attributes[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SceneVertex, color)};
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState color_attachment{};
    color_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &color_attachment;

    const std::array<VkDynamicState, 2> dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = static_cast<std::uint32_t>(stages.size());
    pipeline_info.pStages = stages.data();
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;

    const VkResult result = vkCreateGraphicsPipelines(
        context_.device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        context_.allocator,
        &pipeline_);
    vkDestroyShaderModule(context_.device, fragment_shader, context_.allocator);
    vkDestroyShaderModule(context_.device, vertex_shader, context_.allocator);
    return check_result(result, "vkCreateGraphicsPipelines", error);
}

bool SceneRenderer::Impl::create_command_resources(std::string* error)
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = context_.graphics_queue_family;
    if (!check_result(
            vkCreateCommandPool(
                context_.device,
                &pool_info,
                context_.allocator,
                &command_pool_),
            "vkCreateCommandPool",
            error))
    {
        return false;
    }

    VkCommandBufferAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation.commandPool = command_pool_;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    return check_result(
        vkAllocateCommandBuffers(context_.device, &allocation, &command_buffer_),
        "vkAllocateCommandBuffers",
        error);
}

}

namespace slrye
{

bool SceneRenderer::Impl::create_render_target(
    std::uint32_t width,
    std::uint32_t height,
    std::string* error)
{
    if (!create_image(
            width,
            height,
            color_format_,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            color_image_,
            color_memory_,
            error) ||
        !create_image_view(
            color_image_,
            color_format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            color_view_,
            error) ||
        !create_image(
            width,
            height,
            depth_format_,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            depth_image_,
            depth_memory_,
            error) ||
        !create_image_view(
            depth_image_,
            depth_format_,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            depth_view_,
            error))
    {
        destroy_render_target();
        return false;
    }

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.maxLod = 0.0f;
    if (!check_result(
            vkCreateSampler(context_.device, &sampler_info, context_.allocator, &sampler_),
            "vkCreateSampler",
            error))
    {
        destroy_render_target();
        return false;
    }

    const std::array<VkImageView, 2> attachments{color_view_, depth_view_};
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.layers = 1;
    if (!check_result(
            vkCreateFramebuffer(
                context_.device,
                &framebuffer_info,
                context_.allocator,
                &framebuffer_),
            "vkCreateFramebuffer",
            error))
    {
        destroy_render_target();
        return false;
    }

    texture_descriptor_ = ImGui_ImplVulkan_AddTexture(
        sampler_,
        color_view_,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (texture_descriptor_ == VK_NULL_HANDLE)
    {
        set_error(error, "Unable to register the viewport texture with ImGui");
        destroy_render_target();
        return false;
    }

    width_ = width;
    height_ = height;
    return true;
}

void SceneRenderer::Impl::destroy_render_target() noexcept
{
    if (context_.device == VK_NULL_HANDLE)
    {
        return;
    }

    if (texture_descriptor_ != VK_NULL_HANDLE)
    {
        if (ImGui::GetCurrentContext() != nullptr)
        {
            ImGui_ImplVulkan_RemoveTexture(texture_descriptor_);
        }
        texture_descriptor_ = VK_NULL_HANDLE;
    }
    if (framebuffer_ != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(context_.device, framebuffer_, context_.allocator);
        framebuffer_ = VK_NULL_HANDLE;
    }
    if (sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(context_.device, sampler_, context_.allocator);
        sampler_ = VK_NULL_HANDLE;
    }
    if (depth_view_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(context_.device, depth_view_, context_.allocator);
        depth_view_ = VK_NULL_HANDLE;
    }
    if (depth_image_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(context_.device, depth_image_, context_.allocator);
        depth_image_ = VK_NULL_HANDLE;
    }
    if (depth_memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device, depth_memory_, context_.allocator);
        depth_memory_ = VK_NULL_HANDLE;
    }
    if (color_view_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(context_.device, color_view_, context_.allocator);
        color_view_ = VK_NULL_HANDLE;
    }
    if (color_image_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(context_.device, color_image_, context_.allocator);
        color_image_ = VK_NULL_HANDLE;
    }
    if (color_memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device, color_memory_, context_.allocator);
        color_memory_ = VK_NULL_HANDLE;
    }

    width_ = 0;
    height_ = 0;
}

bool SceneRenderer::Impl::ensure_render_target(
    std::uint32_t width,
    std::uint32_t height,
    std::string* error)
{
    if (framebuffer_ != VK_NULL_HANDLE && width_ == width && height_ == height)
    {
        return true;
    }

    vkDeviceWaitIdle(context_.device);
    destroy_render_target();
    return create_render_target(width, height, error);
}

}

namespace slrye
{

bool SceneRenderer::Impl::ensure_vertex_capacity(
    VkDeviceSize required_size,
    std::string* error)
{
    if (required_size <= vertex_capacity_ && mapped_vertices_ != nullptr)
    {
        return true;
    }

    vkDeviceWaitIdle(context_.device);
    if (mapped_vertices_ != nullptr)
    {
        vkUnmapMemory(context_.device, vertex_memory_);
        mapped_vertices_ = nullptr;
    }
    if (vertex_buffer_ != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(context_.device, vertex_buffer_, context_.allocator);
        vertex_buffer_ = VK_NULL_HANDLE;
    }
    if (vertex_memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device, vertex_memory_, context_.allocator);
        vertex_memory_ = VK_NULL_HANDLE;
    }

    vertex_capacity_ = std::max<VkDeviceSize>(required_size, 4096);
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = vertex_capacity_;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!check_result(
            vkCreateBuffer(
                context_.device,
                &buffer_info,
                context_.allocator,
                &vertex_buffer_),
            "vkCreateBuffer",
            error))
    {
        vertex_capacity_ = 0;
        return false;
    }

    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(context_.device, vertex_buffer_, &memory_requirements);
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = memory_requirements.size;
    allocation.memoryTypeIndex = find_memory_type(
        memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (allocation.memoryTypeIndex == std::numeric_limits<std::uint32_t>::max() ||
        !check_result(
            vkAllocateMemory(
                context_.device,
                &allocation,
                context_.allocator,
                &vertex_memory_),
            "vkAllocateMemory",
            error) ||
        !check_result(
            vkBindBufferMemory(context_.device, vertex_buffer_, vertex_memory_, 0),
            "vkBindBufferMemory",
            error) ||
        !check_result(
            vkMapMemory(
                context_.device,
                vertex_memory_,
                0,
                VK_WHOLE_SIZE,
                0,
                &mapped_vertices_),
            "vkMapMemory",
            error))
    {
        set_error(error, "Unable to allocate the viewport vertex buffer");
        return false;
    }

    return true;
}

bool SceneRenderer::Impl::upload_geometry(
    const SceneGeometry& geometry,
    std::string* error)
{
    if (geometry.vertices.empty())
    {
        return true;
    }

    const VkDeviceSize required_size =
        sizeof(SceneVertex) * static_cast<VkDeviceSize>(geometry.vertices.size());
    if (!ensure_vertex_capacity(required_size, error))
    {
        return false;
    }

    std::memcpy(mapped_vertices_, geometry.vertices.data(), static_cast<std::size_t>(required_size));
    return true;
}

bool SceneRenderer::Impl::record_and_submit(
    const PerspectiveCamera& camera,
    std::size_t vertex_count,
    std::string* error)
{
    if (!check_result(
            vkResetCommandPool(context_.device, command_pool_, 0),
            "vkResetCommandPool",
            error))
    {
        return false;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!check_result(
            vkBeginCommandBuffer(command_buffer_, &begin_info),
            "vkBeginCommandBuffer",
            error))
    {
        return false;
    }

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.075f, 0.085f, 0.105f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo render_pass_begin{};
    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderPass = render_pass_;
    render_pass_begin.framebuffer = framebuffer_;
    render_pass_begin.renderArea.extent = VkExtent2D{width_, height_};
    render_pass_begin.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
    render_pass_begin.pClearValues = clear_values.data();
    vkCmdBeginRenderPass(command_buffer_, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    if (vertex_count > 0)
    {
        VkViewport viewport{};
        viewport.width = static_cast<float>(width_);
        viewport.height = static_cast<float>(height_);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = VkExtent2D{width_, height_};
        vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

        const VkDeviceSize offset = 0;
        vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertex_buffer_, &offset);
        const glm::mat4 view_projection = camera.view_projection_matrix();
        vkCmdPushConstants(
            command_buffer_,
            pipeline_layout_,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(view_projection),
            &view_projection);
        vkCmdDraw(
            command_buffer_,
            static_cast<std::uint32_t>(vertex_count),
            1,
            0,
            0);
    }

    vkCmdEndRenderPass(command_buffer_);
    if (!check_result(vkEndCommandBuffer(command_buffer_), "vkEndCommandBuffer", error))
    {
        return false;
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;
    if (!check_result(
            vkQueueSubmit(context_.graphics_queue, 1, &submit_info, VK_NULL_HANDLE),
            "vkQueueSubmit",
            error))
    {
        return false;
    }
    return check_result(vkQueueWaitIdle(context_.graphics_queue), "vkQueueWaitIdle", error);
}

}

namespace slrye
{

std::uint32_t SceneRenderer::Impl::find_memory_type(
    std::uint32_t type_filter,
    VkMemoryPropertyFlags properties) const noexcept
{
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(context_.physical_device, &memory_properties);
    for (std::uint32_t index = 0; index < memory_properties.memoryTypeCount; ++index)
    {
        if ((type_filter & (1u << index)) != 0 &&
            (memory_properties.memoryTypes[index].propertyFlags & properties) == properties)
        {
            return index;
        }
    }
    return std::numeric_limits<std::uint32_t>::max();
}

bool SceneRenderer::Impl::create_image(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImage& image,
    VkDeviceMemory& memory,
    std::string* error)
{
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent = VkExtent3D{width, height, 1};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!check_result(
            vkCreateImage(context_.device, &image_info, context_.allocator, &image),
            "vkCreateImage",
            error))
    {
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(context_.device, image, &requirements);
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = find_memory_type(
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocation.memoryTypeIndex == std::numeric_limits<std::uint32_t>::max() ||
        !check_result(
            vkAllocateMemory(context_.device, &allocation, context_.allocator, &memory),
            "vkAllocateMemory",
            error) ||
        !check_result(
            vkBindImageMemory(context_.device, image, memory, 0),
            "vkBindImageMemory",
            error))
    {
        set_error(error, "Unable to allocate viewport image memory");
        return false;
    }
    return true;
}

bool SceneRenderer::Impl::create_image_view(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspect,
    VkImageView& image_view,
    std::string* error)
{
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    return check_result(
        vkCreateImageView(
            context_.device,
            &create_info,
            context_.allocator,
            &image_view),
        "vkCreateImageView",
        error);
}

SceneRenderer::SceneRenderer()
    : impl_(std::make_unique<Impl>())
{
}

SceneRenderer::~SceneRenderer()
{
    shutdown();
}

bool SceneRenderer::initialize(
    const VulkanContext& context,
    const std::filesystem::path& shader_directory,
    std::string* error)
{
    return impl_->initialize(context, shader_directory, error);
}

bool SceneRenderer::render(
    const Scene& scene,
    const PerspectiveCamera& camera,
    std::uint32_t width,
    std::uint32_t height,
    std::optional<EntityId> selected_entity,
    bool show_grid,
    std::string* error)
{
    return impl_->render(scene, camera, width, height, selected_entity, show_grid, error);
}

void SceneRenderer::shutdown()
{
    impl_->shutdown();
}

ImTextureID SceneRenderer::texture_id() const noexcept
{
    return impl_->texture_id();
}

bool SceneRenderer::is_initialized() const noexcept
{
    return impl_->is_initialized();
}

}
