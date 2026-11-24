#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <engine/resources/material.h>
#include <engine/resources/mesh.h>
#include <engine/resources/texture.h>

namespace slrye::engine
{

struct VulkanContext;
class VulkanBackend;

/**
 * @brief 场景图树状模型节点（Model Scene Graph Node）
 */
struct ModelNode
{
    std::string name;                ///< 节点名称（来自 glTF 节点定义）
    int mesh_index = -1;             ///< 绑定的网格索引（对应 Model::meshes()，-1 表示纯分组或骨骼节点）
    glm::mat4 local_transform{1.0f}; ///< 局部几何变换矩阵 (Translation, Rotation, Scale)
    std::vector<int> children;       ///< 子节点索引列表（对应 Model::nodes() 中的下标）
};

/**
 * @brief 3D 模型高阶资产（Model Asset）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Resources 资源层，代表由 glTF 2.0 / GLB 文件解析生成的复合 3D 几何与材质资产；
 * 2. 内部基于现代超快 SIMD JSON 库 fastgltf 解析并解构 glTF 2.0 规格标准；
 * 3. 聚合包含：
 *    - 几何网格资产列表（Mesh Assets）
 *    - PBR 材质资产列表（Material Assets）
 *    - 2D 贴图纹理资产列表（Texture Assets，支持外部相对文件及 GLB 嵌入式二进制图块）
 *    - 树状层级节点与变换矩阵（ModelNode 场景图）
 *    - 全局复合 AABB 包围盒；
 * 4. 完美对接 ECS 架构中的 ModelComponent 与 MeshComponent，实现一键实例化与层级展开。
 */
class Model
{
public:
    Model();
    ~Model();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) noexcept;
    Model& operator=(Model&&) noexcept;

    /// @brief 从本地 glTF / GLB 文件路径解析并构建完整 3D 模型
    [[nodiscard]] static std::shared_ptr<Model> load_from_file(
        const VulkanContext& context,
        const std::filesystem::path& path,
        std::string* error = nullptr);

    [[nodiscard]] static std::shared_ptr<Model> load_from_file(
        const VulkanBackend& backend,
        const std::filesystem::path& path,
        std::string* error = nullptr);

    void destroy();

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& meshes() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<Material>>& materials() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<Texture>>& textures() const noexcept;
    [[nodiscard]] const std::vector<ModelNode>& nodes() const noexcept;
    [[nodiscard]] const std::vector<int>& root_nodes() const noexcept;
    [[nodiscard]] const glm::vec3& aabb_min() const noexcept;
    [[nodiscard]] const glm::vec3& aabb_max() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}

namespace slrye
{
using engine::ModelNode;
using engine::Model;
}
