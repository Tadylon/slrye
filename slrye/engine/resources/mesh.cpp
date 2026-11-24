#include <engine/resources/mesh.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numbers>

#include <glm/glm.hpp>

#include <engine/rhi/vulkan/vulkan_context.h>
#include <engine/rhi/vulkan/vulkan_backend.h>

namespace slrye::engine
{

namespace
{

bool execute_single_time_commands(
    const VulkanContext& context,
    const std::function<void(VkCommandBuffer)>& record_func)
{
    if (context.device == VK_NULL_HANDLE || context.graphics_queue == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = context.graphics_queue_family;

    VkCommandPool command_pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(context.device, &pool_info, context.allocator, &command_pool) != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(context.device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        vkDestroyCommandPool(context.device, command_pool, context.allocator);
        return false;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        vkDestroyCommandPool(context.device, command_pool, context.allocator);
        return false;
    }

    record_func(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        vkDestroyCommandPool(context.device, command_pool, context.allocator);
        return false;
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphics_queue);

    vkDestroyCommandPool(context.device, command_pool, context.allocator);
    return true;
}

} // namespace

VkVertexInputBindingDescription Vertex::get_binding_description()
{
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

std::vector<VkVertexInputAttributeDescription> Vertex::get_attribute_descriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributes(5);

    // Location 0: Position
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = static_cast<uint32_t>(offsetof(Vertex, position));

    // Location 1: Normal
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = static_cast<uint32_t>(offsetof(Vertex, normal));

    // Location 2: TexCoords
    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[2].offset = static_cast<uint32_t>(offsetof(Vertex, tex_coords));

    // Location 3: Tangent
    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[3].offset = static_cast<uint32_t>(offsetof(Vertex, tangent));

    // Location 4: Color
    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[4].offset = static_cast<uint32_t>(offsetof(Vertex, color));

    return attributes;
}

struct Mesh::Impl
{
    VulkanContext context{};
    AllocatedBuffer vertex_buffer{};
    AllocatedBuffer index_buffer{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> submeshes;
    glm::vec3 aabb_min{0.0f};
    glm::vec3 aabb_max{0.0f};
    bool valid = false;

    void destroy()
    {
        if (context.vma_allocator != VK_NULL_HANDLE)
        {
            if (vertex_buffer.is_valid())
            {
                vertex_buffer.destroy(context.vma_allocator);
            }
            if (index_buffer.is_valid())
            {
                index_buffer.destroy(context.vma_allocator);
            }
        }
        valid = false;
    }
};

Mesh::Mesh()
    : impl_(std::make_unique<Impl>())
{
}

Mesh::~Mesh()
{
    destroy();
}

Mesh::Mesh(Mesh&&) noexcept = default;
Mesh& Mesh::operator=(Mesh&&) noexcept = default;

std::shared_ptr<Mesh> Mesh::create(
    const VulkanContext& context,
    std::span<const Vertex> vertices,
    std::span<const uint32_t> indices,
    std::vector<SubMesh> submeshes,
    std::string* error)
{
    if (vertices.empty())
    {
        if (error != nullptr)
        {
            *error = "Cannot create mesh with zero vertices";
        }
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();
    mesh->impl_->context = context;
    mesh->impl_->vertices.assign(vertices.begin(), vertices.end());
    mesh->impl_->indices.assign(indices.begin(), indices.end());

    // 计算全局 AABB
    glm::vec3 min_pt = vertices[0].position;
    glm::vec3 max_pt = vertices[0].position;
    for (const auto& v : vertices)
    {
        min_pt = glm::min(min_pt, v.position);
        max_pt = glm::max(max_pt, v.position);
    }
    mesh->impl_->aabb_min = min_pt;
    mesh->impl_->aabb_max = max_pt;

    // 若未显式传入 submeshes，默认构建覆盖整个网格的单一 submesh
    if (submeshes.empty())
    {
        SubMesh sm{};
        sm.first_index = 0;
        sm.index_count = static_cast<uint32_t>(indices.size());
        sm.first_vertex = 0;
        sm.vertex_count = static_cast<uint32_t>(vertices.size());
        sm.material_index = 0;
        sm.aabb_min = min_pt;
        sm.aabb_max = max_pt;
        mesh->impl_->submeshes.push_back(sm);
    }
    else
    {
        mesh->impl_->submeshes = std::move(submeshes);
    }

    // 若拥有合法的 Vulkan 硬件上下文，则将几何数据上传至显存 Device Local Buffer
    if (context.device != VK_NULL_HANDLE && context.vma_allocator != VK_NULL_HANDLE)
    {
        const VkDeviceSize vertex_bytes = sizeof(Vertex) * vertices.size();
        AllocatedBuffer vertex_staging = AllocatedBuffer::create(
            context.vma_allocator,
            vertex_bytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        if (!vertex_staging.is_valid())
        {
            if (error != nullptr) *error = "Failed to allocate vertex staging buffer";
            return nullptr;
        }

        void* v_map = vertex_staging.map(context.vma_allocator);
        std::memcpy(v_map, vertices.data(), vertex_bytes);
        vertex_staging.unmap(context.vma_allocator);

        mesh->impl_->vertex_buffer = AllocatedBuffer::create(
            context.vma_allocator,
            vertex_bytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_AUTO,
            0);

        if (!mesh->impl_->vertex_buffer.is_valid())
        {
            vertex_staging.destroy(context.vma_allocator);
            if (error != nullptr) *error = "Failed to allocate device local vertex buffer";
            return nullptr;
        }

        AllocatedBuffer index_staging{};
        const VkDeviceSize index_bytes = sizeof(uint32_t) * indices.size();
        if (!indices.empty())
        {
            index_staging = AllocatedBuffer::create(
                context.vma_allocator,
                index_bytes,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_AUTO,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

            if (!index_staging.is_valid())
            {
                vertex_staging.destroy(context.vma_allocator);
                mesh->impl_->vertex_buffer.destroy(context.vma_allocator);
                if (error != nullptr) *error = "Failed to allocate index staging buffer";
                return nullptr;
            }

            void* i_map = index_staging.map(context.vma_allocator);
            std::memcpy(i_map, indices.data(), index_bytes);
            index_staging.unmap(context.vma_allocator);

            mesh->impl_->index_buffer = AllocatedBuffer::create(
                context.vma_allocator,
                index_bytes,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_AUTO,
                0);

            if (!mesh->impl_->index_buffer.is_valid())
            {
                vertex_staging.destroy(context.vma_allocator);
                index_staging.destroy(context.vma_allocator);
                mesh->impl_->vertex_buffer.destroy(context.vma_allocator);
                if (error != nullptr) *error = "Failed to allocate device local index buffer";
                return nullptr;
            }
        }

        // 使用单次命令缓冲将临时暂存缓冲拷贝到设备本地显存
        bool copy_ok = execute_single_time_commands(
            context,
            [&](VkCommandBuffer cmd)
            {
                VkBufferCopy vertex_copy{};
                vertex_copy.srcOffset = 0;
                vertex_copy.dstOffset = 0;
                vertex_copy.size = vertex_bytes;
                vkCmdCopyBuffer(cmd, vertex_staging.buffer, mesh->impl_->vertex_buffer.buffer, 1, &vertex_copy);

                if (!indices.empty())
                {
                    VkBufferCopy index_copy{};
                    index_copy.srcOffset = 0;
                    index_copy.dstOffset = 0;
                    index_copy.size = index_bytes;
                    vkCmdCopyBuffer(cmd, index_staging.buffer, mesh->impl_->index_buffer.buffer, 1, &index_copy);
                }
            });

        vertex_staging.destroy(context.vma_allocator);
        if (!indices.empty())
        {
            index_staging.destroy(context.vma_allocator);
        }

        if (!copy_ok)
        {
            mesh->impl_->destroy();
            if (error != nullptr) *error = "Failed to copy vertex/index buffers to GPU memory";
            return nullptr;
        }
    }

    mesh->impl_->valid = true;
    return mesh;
}

std::shared_ptr<Mesh> Mesh::create(
    const VulkanBackend& backend,
    std::span<const Vertex> vertices,
    std::span<const uint32_t> indices,
    std::vector<SubMesh> submeshes,
    std::string* error)
{
    return create(backend.context(), vertices, indices, std::move(submeshes), error);
}

std::shared_ptr<Mesh> Mesh::create_cube(const VulkanContext& context, float size)
{
    const float h = size * 0.5f;

    // 6 个面，每个面 4 个顶点，独立法线与 UV
    std::vector<Vertex> vertices = {
        // Front face (+Z)
        {{-h, -h,  h}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h, -h,  h}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h,  h,  h}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-h,  h,  h}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},

        // Back face (-Z)
        {{ h, -h, -h}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-h, -h, -h}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{-h,  h, -h}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h,  h, -h}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},

        // Top face (+Y)
        {{-h,  h,  h}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h,  h,  h}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h,  h, -h}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-h,  h, -h}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},

        // Bottom face (-Y)
        {{-h, -h, -h}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h, -h, -h}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ h, -h,  h}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-h, -h,  h}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},

        // Right face (+X)
        {{ h, -h,  h}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{ h, -h, -h}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{ h,  h, -h}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
        {{ h,  h,  h}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},

        // Left face (-X)
        {{-h, -h, -h}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-h, -h,  h}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-h,  h,  h}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
        {{-h,  h, -h}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
        0,  1,  2,   2,  3,  0,  // Front
        4,  5,  6,   6,  7,  4,  // Back
        8,  9,  10,  10, 11, 8,  // Top
        12, 13, 14,  14, 15, 12, // Bottom
        16, 17, 18,  18, 19, 16, // Right
        20, 21, 22,  22, 23, 20  // Left
    };

    return create(context, vertices, indices);
}

std::shared_ptr<Mesh> Mesh::create_quad(const VulkanContext& context, float width, float height)
{
    const float hw = width * 0.5f;
    const float hh = height * 0.5f;

    std::vector<Vertex> vertices = {
        {{-hw, 0.0f, -hh}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ hw, 0.0f, -hh}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ hw, 0.0f,  hh}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-hw, 0.0f,  hh}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
        0, 2, 1,
        0, 3, 2
    };

    return create(context, vertices, indices);
}

std::shared_ptr<Mesh> Mesh::create_sphere(
    const VulkanContext& context,
    float radius,
    uint32_t rings,
    uint32_t sectors)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const float R = 1.0f / static_cast<float>(rings - 1);
    const float S = 1.0f / static_cast<float>(sectors - 1);

    vertices.reserve(rings * sectors);
    indices.reserve(rings * sectors * 6);

    for (uint32_t r = 0; r < rings; ++r)
    {
        for (uint32_t s = 0; s < sectors; ++s)
        {
            const float y = std::sin(-std::numbers::pi_v<float> * 0.5f + std::numbers::pi_v<float> * static_cast<float>(r) * R);
            const float x = std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S) * std::sin(std::numbers::pi_v<float> * static_cast<float>(r) * R);
            const float z = std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S) * std::sin(std::numbers::pi_v<float> * static_cast<float>(r) * R);

            Vertex v{};
            v.position = glm::vec3(x, y, z) * radius;
            v.normal = glm::normalize(glm::vec3(x, y, z));
            v.tex_coords = glm::vec2(static_cast<float>(s) * S, static_cast<float>(r) * R);
            v.tangent = glm::vec4(-std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S), 0.0f, std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(s) * S), 1.0f);
            vertices.push_back(v);
        }
    }

    for (uint32_t r = 0; r < rings - 1; ++r)
    {
        for (uint32_t s = 0; s < sectors - 1; ++s)
        {
            const uint32_t cur = r * sectors + s;
            const uint32_t next = cur + sectors;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(next + 1);

            indices.push_back(cur);
            indices.push_back(next + 1);
            indices.push_back(cur + 1);
        }
    }

    return create(context, vertices, indices);
}

void Mesh::destroy()
{
    if (impl_)
    {
        impl_->destroy();
    }
}

bool Mesh::is_valid() const noexcept
{
    return impl_ && impl_->valid;
}

VkBuffer Mesh::vertex_buffer() const noexcept
{
    return impl_ ? impl_->vertex_buffer.buffer : VK_NULL_HANDLE;
}

VkBuffer Mesh::index_buffer() const noexcept
{
    return impl_ ? impl_->index_buffer.buffer : VK_NULL_HANDLE;
}

uint32_t Mesh::vertex_count() const noexcept
{
    return impl_ ? static_cast<uint32_t>(impl_->vertices.size()) : 0;
}

uint32_t Mesh::index_count() const noexcept
{
    return impl_ ? static_cast<uint32_t>(impl_->indices.size()) : 0;
}

const std::vector<SubMesh>& Mesh::submeshes() const noexcept
{
    static const std::vector<SubMesh> empty;
    return impl_ ? impl_->submeshes : empty;
}

const std::vector<Vertex>& Mesh::vertices() const noexcept
{
    static const std::vector<Vertex> empty;
    return impl_ ? impl_->vertices : empty;
}

const std::vector<uint32_t>& Mesh::indices() const noexcept
{
    static const std::vector<uint32_t> empty;
    return impl_ ? impl_->indices : empty;
}

const glm::vec3& Mesh::aabb_min() const noexcept
{
    static const glm::vec3 zero{0.0f};
    return impl_ ? impl_->aabb_min : zero;
}

const glm::vec3& Mesh::aabb_max() const noexcept
{
    static const glm::vec3 zero{0.0f};
    return impl_ ? impl_->aabb_max : zero;
}

} // namespace slrye::engine
