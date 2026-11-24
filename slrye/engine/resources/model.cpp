#include <engine/resources/model.h>

#include <cstring>
#include <iostream>
#include <unordered_set>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <engine/rhi/vulkan/vulkan_context.h>
#include <engine/rhi/vulkan/vulkan_backend.h>

namespace slrye::engine
{

struct Model::Impl
{
    VulkanContext context{};
    std::filesystem::path path;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<ModelNode> nodes;
    std::vector<int> root_nodes;
    glm::vec3 aabb_min{0.0f};
    glm::vec3 aabb_max{0.0f};
    bool valid = false;

    void destroy()
    {
        for (auto& mesh : meshes)
        {
            if (mesh)
            {
                mesh->destroy();
            }
        }
        meshes.clear();

        for (auto& tex : textures)
        {
            if (tex)
            {
                tex->destroy();
            }
        }
        textures.clear();

        materials.clear();
        nodes.clear();
        root_nodes.clear();
        valid = false;
    }
};

Model::Model()
    : impl_(std::make_unique<Impl>())
{
}

Model::~Model()
{
    destroy();
}

Model::Model(Model&&) noexcept = default;
Model& Model::operator=(Model&&) noexcept = default;

std::shared_ptr<Model> Model::load_from_file(
    const VulkanContext& context,
    const std::filesystem::path& path,
    std::string* error)
{
    if (!std::filesystem::exists(path))
    {
        if (error != nullptr)
        {
            *error = "Model file does not exist: " + path.string();
        }
        return nullptr;
    }

    auto data_buffer = fastgltf::GltfDataBuffer::FromPath(path);
    if (data_buffer.error() != fastgltf::Error::None)
    {
        if (error != nullptr)
        {
            *error = "Failed to open glTF file: " + std::string(fastgltf::getErrorMessage(data_buffer.error()));
        }
        return nullptr;
    }

    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform);
    constexpr auto options =
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices |
        fastgltf::Options::DecomposeNodeMatrices;

    auto asset_expected = parser.loadGltf(data_buffer.get(), path.parent_path(), options);
    if (asset_expected.error() != fastgltf::Error::None)
    {
        if (error != nullptr)
        {
            *error = "Failed to parse glTF: " + std::string(fastgltf::getErrorMessage(asset_expected.error()));
        }
        return nullptr;
    }

    auto& asset = asset_expected.get();
    auto model = std::make_shared<Model>();
    model->impl_->context = context;
    model->impl_->path = path;

    const std::filesystem::path base_dir = path.parent_path();

    // 1. 加载所有图片为 2D 纹理 (Textures)
    model->impl_->textures.reserve(asset.images.size());
    for (const auto& image : asset.images)
    {
        std::shared_ptr<Texture> tex = nullptr;
        std::visit(fastgltf::visitor {
            [](auto&) {},
            [&](const fastgltf::sources::URI& uri) {
                if (uri.uri.isLocalPath())
                {
                    tex = Texture::create_from_file(context, base_dir / uri.uri.fspath());
                }
            },
            [&](const fastgltf::sources::Array& arr) {
                tex = Texture::create_from_memory(context, arr.bytes.data(), arr.bytes.size());
            },
            [&](const fastgltf::sources::Vector& vec) {
                tex = Texture::create_from_memory(context, vec.bytes.data(), vec.bytes.size());
            },
            [&](const fastgltf::sources::ByteView& byteView) {
                tex = Texture::create_from_memory(context, byteView.bytes.data(), byteView.bytes.size());
            },
            [&](const fastgltf::sources::BufferView& view) {
                if (view.bufferViewIndex < asset.bufferViews.size())
                {
                    const auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                    if (bufferView.bufferIndex < asset.buffers.size())
                    {
                        const auto& buffer = asset.buffers[bufferView.bufferIndex];
                        std::visit(fastgltf::visitor {
                            [](auto&) {},
                            [&](const fastgltf::sources::Array& arr) {
                                tex = Texture::create_from_memory(
                                    context,
                                    arr.bytes.data() + bufferView.byteOffset,
                                    bufferView.byteLength);
                            },
                            [&](const fastgltf::sources::Vector& vec) {
                                tex = Texture::create_from_memory(
                                    context,
                                    vec.bytes.data() + bufferView.byteOffset,
                                    bufferView.byteLength);
                            },
                            [&](const fastgltf::sources::ByteView& bv) {
                                tex = Texture::create_from_memory(
                                    context,
                                    bv.bytes.data() + bufferView.byteOffset,
                                    bufferView.byteLength);
                            }
                        }, buffer.data);
                    }
                }
            }
        }, image.data);

        model->impl_->textures.push_back(tex);
    }

    // 2. 加载所有 PBR 材质 (Materials)
    auto get_texture_from_index = [&](std::size_t tex_idx) -> std::shared_ptr<Texture>
    {
        if (tex_idx < asset.textures.size())
        {
            auto img_idx = asset.textures[tex_idx].imageIndex;
            if (img_idx.has_value() && *img_idx < model->impl_->textures.size())
            {
                return model->impl_->textures[*img_idx];
            }
        }
        return nullptr;
    };

    model->impl_->materials.reserve(asset.materials.size());
    for (const auto& material : asset.materials)
    {
        auto mat = std::make_shared<Material>();
        mat->name = std::string(material.name);
        if (mat->name.empty())
        {
            mat->name = "Material_" + std::to_string(model->impl_->materials.size());
        }

        mat->base_color_factor = glm::vec4(
            material.pbrData.baseColorFactor[0],
            material.pbrData.baseColorFactor[1],
            material.pbrData.baseColorFactor[2],
            material.pbrData.baseColorFactor[3]);

        mat->metallic_factor = material.pbrData.metallicFactor;
        mat->roughness_factor = material.pbrData.roughnessFactor;

        mat->emissive_factor = glm::vec3(
            material.emissiveFactor[0],
            material.emissiveFactor[1],
            material.emissiveFactor[2]);

        mat->double_sided = material.doubleSided;

        if (material.alphaMode == fastgltf::AlphaMode::Opaque)
        {
            mat->alpha_mode = AlphaMode::Opaque;
        }
        else if (material.alphaMode == fastgltf::AlphaMode::Mask)
        {
            mat->alpha_mode = AlphaMode::Mask;
            mat->alpha_cutoff = material.alphaCutoff;
        }
        else if (material.alphaMode == fastgltf::AlphaMode::Blend)
        {
            mat->alpha_mode = AlphaMode::Blend;
        }

        if (material.pbrData.baseColorTexture.has_value())
        {
            mat->base_color_texture = get_texture_from_index(material.pbrData.baseColorTexture->textureIndex);
        }
        if (material.pbrData.metallicRoughnessTexture.has_value())
        {
            mat->metallic_roughness_texture = get_texture_from_index(material.pbrData.metallicRoughnessTexture->textureIndex);
        }
        if (material.normalTexture.has_value())
        {
            mat->normal_texture = get_texture_from_index(material.normalTexture->textureIndex);
        }
        if (material.occlusionTexture.has_value())
        {
            mat->occlusion_texture = get_texture_from_index(material.occlusionTexture->textureIndex);
        }
        if (material.emissiveTexture.has_value())
        {
            mat->emissive_texture = get_texture_from_index(material.emissiveTexture->textureIndex);
        }

        model->impl_->materials.push_back(mat);
    }

    if (model->impl_->materials.empty())
    {
        model->impl_->materials.push_back(Material::create_default(&context));
    }

    // 3. 解析网格几何体 (Meshes & SubMeshes)
    model->impl_->meshes.reserve(asset.meshes.size());
    for (const auto& gltf_mesh : asset.meshes)
    {
        std::vector<Vertex> mesh_vertices;
        std::vector<uint32_t> mesh_indices;
        std::vector<SubMesh> mesh_submeshes;

        for (const auto& primitive : gltf_mesh.primitives)
        {
            auto pos_it = primitive.findAttribute("POSITION");
            if (pos_it == primitive.attributes.end())
            {
                continue;
            }

            const uint32_t first_vertex = static_cast<uint32_t>(mesh_vertices.size());
            const uint32_t first_index = static_cast<uint32_t>(mesh_indices.size());

            auto& pos_accessor = asset.accessors[pos_it->accessorIndex];
            const std::size_t prim_vert_count = pos_accessor.count;

            std::vector<Vertex> prim_vertices(prim_vert_count);
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset, pos_accessor,
                [&](glm::vec3 pos, std::size_t idx)
                {
                    prim_vertices[idx].position = pos;
                });

            auto norm_it = primitive.findAttribute("NORMAL");
            if (norm_it != primitive.attributes.end())
            {
                auto& norm_accessor = asset.accessors[norm_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset, norm_accessor,
                    [&](glm::vec3 norm, std::size_t idx)
                    {
                        if (idx < prim_vertices.size()) prim_vertices[idx].normal = norm;
                    });
            }

            auto uv_it = primitive.findAttribute("TEXCOORD_0");
            if (uv_it != primitive.attributes.end())
            {
                auto& uv_accessor = asset.accessors[uv_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    asset, uv_accessor,
                    [&](glm::vec2 uv, std::size_t idx)
                    {
                        if (idx < prim_vertices.size()) prim_vertices[idx].tex_coords = uv;
                    });
            }

            auto tan_it = primitive.findAttribute("TANGENT");
            if (tan_it != primitive.attributes.end())
            {
                auto& tan_accessor = asset.accessors[tan_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    asset, tan_accessor,
                    [&](glm::vec4 tan, std::size_t idx)
                    {
                        if (idx < prim_vertices.size()) prim_vertices[idx].tangent = tan;
                    });
            }

            std::vector<uint32_t> prim_indices;
            if (primitive.indicesAccessor.has_value())
            {
                auto& index_accessor = asset.accessors[*primitive.indicesAccessor];
                prim_indices.reserve(index_accessor.count);
                fastgltf::iterateAccessorWithIndex<uint32_t>(
                    asset, index_accessor,
                    [&](uint32_t idx, std::size_t)
                    {
                        prim_indices.push_back(idx);
                    });
            }
            else
            {
                prim_indices.resize(prim_vert_count);
                for (uint32_t i = 0; i < prim_vert_count; ++i)
                {
                    prim_indices[i] = i;
                }
            }

            glm::vec3 sub_min = prim_vertices[0].position;
            glm::vec3 sub_max = prim_vertices[0].position;
            for (const auto& pv : prim_vertices)
            {
                sub_min = glm::min(sub_min, pv.position);
                sub_max = glm::max(sub_max, pv.position);
            }

            SubMesh submesh{};
            submesh.first_index = first_index;
            submesh.index_count = static_cast<uint32_t>(prim_indices.size());
            submesh.first_vertex = first_vertex;
            submesh.vertex_count = static_cast<uint32_t>(prim_vertices.size());
            submesh.material_index = static_cast<uint32_t>(primitive.materialIndex.value_or(0));
            submesh.aabb_min = sub_min;
            submesh.aabb_max = sub_max;
            mesh_submeshes.push_back(submesh);

            mesh_vertices.insert(mesh_vertices.end(), prim_vertices.begin(), prim_vertices.end());
            for (uint32_t idx : prim_indices)
            {
                mesh_indices.push_back(idx + first_vertex);
            }
        }

        if (!mesh_vertices.empty())
        {
            auto created_mesh = Mesh::create(context, mesh_vertices, mesh_indices, mesh_submeshes);
            model->impl_->meshes.push_back(created_mesh);
        }
        else
        {
            model->impl_->meshes.push_back(nullptr);
        }
    }

    // 4. 解析场景图节点结构 (Nodes)
    model->impl_->nodes.reserve(asset.nodes.size());
    std::unordered_set<int> child_nodes_set;

    for (const auto& fastgltf_node : asset.nodes)
    {
        ModelNode node{};
        node.name = std::string(fastgltf_node.name);
        if (fastgltf_node.meshIndex.has_value())
        {
            node.mesh_index = static_cast<int>(*fastgltf_node.meshIndex);
        }

        for (auto child_idx : fastgltf_node.children)
        {
            node.children.push_back(static_cast<int>(child_idx));
            child_nodes_set.insert(static_cast<int>(child_idx));
        }

        if (std::holds_alternative<fastgltf::TRS>(fastgltf_node.transform))
        {
            const auto& trs = std::get<fastgltf::TRS>(fastgltf_node.transform);
            const glm::vec3 t(trs.translation[0], trs.translation[1], trs.translation[2]);
            const glm::quat q(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
            const glm::vec3 s(trs.scale[0], trs.scale[1], trs.scale[2]);
            node.local_transform = glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.0f), s);
        }
        else if (std::holds_alternative<fastgltf::math::fmat4x4>(fastgltf_node.transform))
        {
            const auto& m = std::get<fastgltf::math::fmat4x4>(fastgltf_node.transform);
            std::memcpy(&node.local_transform, m.data(), sizeof(glm::mat4));
        }

        model->impl_->nodes.push_back(node);
    }

    // 确定根节点集合
    if (asset.defaultScene.has_value() && *asset.defaultScene < asset.scenes.size())
    {
        for (auto root_idx : asset.scenes[*asset.defaultScene].nodeIndices)
        {
            model->impl_->root_nodes.push_back(static_cast<int>(root_idx));
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(model->impl_->nodes.size()); ++i)
        {
            if (child_nodes_set.find(i) == child_nodes_set.end())
            {
                model->impl_->root_nodes.push_back(i);
            }
        }
    }

    // 5. 组合计算模型全局复合 AABB
    if (!model->impl_->meshes.empty())
    {
        bool aabb_init = false;
        glm::vec3 m_min{0.0f};
        glm::vec3 m_max{0.0f};

        for (const auto& mesh : model->impl_->meshes)
        {
            if (mesh && mesh->is_valid() && mesh->vertex_count() > 0)
            {
                if (!aabb_init)
                {
                    m_min = mesh->aabb_min();
                    m_max = mesh->aabb_max();
                    aabb_init = true;
                }
                else
                {
                    m_min = glm::min(m_min, mesh->aabb_min());
                    m_max = glm::max(m_max, mesh->aabb_max());
                }
            }
        }
        model->impl_->aabb_min = m_min;
        model->impl_->aabb_max = m_max;
    }

    model->impl_->valid = true;
    return model;
}

std::shared_ptr<Model> Model::load_from_file(
    const VulkanBackend& backend,
    const std::filesystem::path& path,
    std::string* error)
{
    return load_from_file(backend.context(), path, error);
}

void Model::destroy()
{
    if (impl_)
    {
        impl_->destroy();
    }
}

bool Model::is_valid() const noexcept
{
    return impl_ && impl_->valid;
}

const std::filesystem::path& Model::path() const noexcept
{
    static const std::filesystem::path empty;
    return impl_ ? impl_->path : empty;
}

const std::vector<std::shared_ptr<Mesh>>& Model::meshes() const noexcept
{
    static const std::vector<std::shared_ptr<Mesh>> empty;
    return impl_ ? impl_->meshes : empty;
}

const std::vector<std::shared_ptr<Material>>& Model::materials() const noexcept
{
    static const std::vector<std::shared_ptr<Material>> empty;
    return impl_ ? impl_->materials : empty;
}

const std::vector<std::shared_ptr<Texture>>& Model::textures() const noexcept
{
    static const std::vector<std::shared_ptr<Texture>> empty;
    return impl_ ? impl_->textures : empty;
}

const std::vector<ModelNode>& Model::nodes() const noexcept
{
    static const std::vector<ModelNode> empty;
    return impl_ ? impl_->nodes : empty;
}

const std::vector<int>& Model::root_nodes() const noexcept
{
    static const std::vector<int> empty;
    return impl_ ? impl_->root_nodes : empty;
}

const glm::vec3& Model::aabb_min() const noexcept
{
    static const glm::vec3 zero{0.0f};
    return impl_ ? impl_->aabb_min : zero;
}

const glm::vec3& Model::aabb_max() const noexcept
{
    static const glm::vec3 zero{0.0f};
    return impl_ ? impl_->aabb_max : zero;
}

} // namespace slrye::engine
