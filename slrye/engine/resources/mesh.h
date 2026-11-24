#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

#include <engine/rhi/vma_resources.h>

namespace slrye::engine
{

struct VulkanContext;
class VulkanBackend;

/**
 * @brief 现代 3D 网格顶点数据结构（符合 PBR / glTF 2.0 顶点属性规范）
 */
struct Vertex
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};       ///< 局部坐标系位置 (X, Y, Z)
    glm::vec3 normal{0.0f, 1.0f, 0.0f};         ///< 顶点法线单位向量 (NX, NY, NZ)
    glm::vec2 tex_coords{0.0f, 0.0f};           ///< 第一套 UV 贴图坐标 (U, V)
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};  ///< 顶点切线与副切线符号 (TX, TY, TZ, Handedness)
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};    ///< 顶点色彩 (RGBA)

    [[nodiscard]] static VkVertexInputBindingDescription get_binding_description();
    [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
};

/**
 * @brief 网格子几何体切片信息（SubMesh / Primitive）
 */
struct SubMesh
{
    uint32_t first_index = 0;                    ///< 在索引缓冲中的起始偏移下标
    uint32_t index_count = 0;                    ///< 本子网格所包含的索引总数
    uint32_t first_vertex = 0;                   ///< 顶点基址偏移（baseVertex）
    uint32_t vertex_count = 0;                   ///< 顶点数量
    uint32_t material_index = 0;                 ///< 关联的材质索引（对应 Model 中的 materials 数组）
    glm::vec3 aabb_min{-0.5f, -0.5f, -0.5f};     ///< 本子网格局部 AABB 包围盒最小顶点
    glm::vec3 aabb_max{0.5f, 0.5f, 0.5f};        ///< 本子网格局部 AABB 包围盒最大顶点
};

/**
 * @brief 3D 几何网格资产（Mesh Asset）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Resources 资源层，代表存储在 GPU 显存中的静态几何体网格数据；
 * 2. 拥有顶点缓冲（AllocatedBuffer vertex_buffer）与索引缓冲（AllocatedBuffer index_buffer）；
 * 3. 支持多个子网格（SubMesh），每个子网格可单独绑定不同 PBR 材质槽位进行多批次绘制；
 * 4. 内置基础几何体生成工厂（立方体 Cube、平面 Quad、经纬球体 Sphere）；
 * 5. 在无 GPU 环境下安全降级保留 CPU 顶点与索引缓存，确保单元测试与脱机处理稳定运行。
 */
class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    /// @brief 从顶点与索引数组构造 Mesh 并上传到 GPU
    [[nodiscard]] static std::shared_ptr<Mesh> create(
        const VulkanContext& context,
        std::span<const Vertex> vertices,
        std::span<const uint32_t> indices,
        std::vector<SubMesh> submeshes = {},
        std::string* error = nullptr);

    [[nodiscard]] static std::shared_ptr<Mesh> create(
        const VulkanBackend& backend,
        std::span<const Vertex> vertices,
        std::span<const uint32_t> indices,
        std::vector<SubMesh> submeshes = {},
        std::string* error = nullptr);

    /// @brief 内置几何体工厂方法
    [[nodiscard]] static std::shared_ptr<Mesh> create_cube(
        const VulkanContext& context,
        float size = 1.0f);

    [[nodiscard]] static std::shared_ptr<Mesh> create_quad(
        const VulkanContext& context,
        float width = 1.0f,
        float height = 1.0f);

    [[nodiscard]] static std::shared_ptr<Mesh> create_sphere(
        const VulkanContext& context,
        float radius = 0.5f,
        uint32_t rings = 24,
        uint32_t sectors = 32);

    void destroy();

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] VkBuffer vertex_buffer() const noexcept;
    [[nodiscard]] VkBuffer index_buffer() const noexcept;
    [[nodiscard]] uint32_t vertex_count() const noexcept;
    [[nodiscard]] uint32_t index_count() const noexcept;
    [[nodiscard]] const std::vector<SubMesh>& submeshes() const noexcept;
    [[nodiscard]] const std::vector<Vertex>& vertices() const noexcept;
    [[nodiscard]] const std::vector<uint32_t>& indices() const noexcept;
    [[nodiscard]] const glm::vec3& aabb_min() const noexcept;
    [[nodiscard]] const glm::vec3& aabb_max() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}

namespace slrye
{
using engine::Vertex;
using engine::SubMesh;
using engine::Mesh;
}
