#include <engine/resources/mesh.h>
#include <engine/resources/material.h>
#include <engine/resources/texture.h>
#include <engine/resources/model.h>
#include <engine/scene/scene.h>
#include <engine/scene/components.h>
#include <engine/rhi/vulkan/vulkan_context.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Vertex layout binding and attributes meet Vulkan specifications")
{
    auto binding = slrye::Vertex::get_binding_description();
    CHECK(binding.binding == 0);
    CHECK(binding.stride == sizeof(slrye::Vertex));
    CHECK(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX);

    auto attributes = slrye::Vertex::get_attribute_descriptions();
    REQUIRE(attributes.size() == 5);
    CHECK(attributes[0].location == 0); // Position
    CHECK(attributes[1].location == 1); // Normal
    CHECK(attributes[2].location == 2); // UV
    CHECK(attributes[3].location == 3); // Tangent
    CHECK(attributes[4].location == 4); // Color
}

TEST_CASE("Mesh built-in geometric shapes generate proper topology and AABB")
{
    slrye::engine::VulkanContext empty_context{};

    // 1. 立方体 (Cube)
    auto cube = slrye::Mesh::create_cube(empty_context, 2.0f);
    REQUIRE(cube != nullptr);
    CHECK(cube->is_valid());
    CHECK(cube->vertex_count() == 24);
    CHECK(cube->index_count() == 36);
    CHECK(cube->submeshes().size() == 1);
    CHECK(cube->aabb_min().x == -1.0f);
    CHECK(cube->aabb_max().x == 1.0f);

    // 2. 平面 (Quad)
    auto quad = slrye::Mesh::create_quad(empty_context, 4.0f, 2.0f);
    REQUIRE(quad != nullptr);
    CHECK(quad->is_valid());
    CHECK(quad->vertex_count() == 4);
    CHECK(quad->index_count() == 6);

    // 3. 球体 (Sphere)
    auto sphere = slrye::Mesh::create_sphere(empty_context, 1.0f, 16, 16);
    REQUIRE(sphere != nullptr);
    CHECK(sphere->is_valid());
    CHECK(sphere->vertex_count() == 16 * 16);
    CHECK(sphere->index_count() > 0);
}

TEST_CASE("Material handles PBR factors and default creation")
{
    auto default_mat = slrye::Material::create_default();
    REQUIRE(default_mat != nullptr);
    CHECK(default_mat->name == "DefaultMaterial");
    CHECK(default_mat->base_color_factor.r == 1.0f);
    CHECK(default_mat->metallic_factor == 1.0f);
    CHECK(default_mat->roughness_factor == 1.0f);
    CHECK(default_mat->alpha_mode == slrye::AlphaMode::Opaque);

    default_mat->alpha_mode = slrye::AlphaMode::Mask;
    default_mat->alpha_cutoff = 0.33f;
    CHECK(default_mat->alpha_cutoff == 0.33f);
}

TEST_CASE("Texture and Model safely handle missing file paths")
{
    slrye::engine::VulkanContext empty_context{};

    std::string tex_err;
    auto tex = slrye::Texture::create_from_file(empty_context, "missing_texture.png", &tex_err);
    CHECK(tex == nullptr);
    CHECK_FALSE(tex_err.empty());

    std::string model_err;
    auto model = slrye::Model::load_from_file(empty_context, "missing_model.gltf", &model_err);
    CHECK(model == nullptr);
    CHECK_FALSE(model_err.empty());
}

TEST_CASE("Scene ECS attaches Mesh, Material, and Model components")
{
    slrye::Scene scene("ResourceScene");

    auto entity = scene.create_entity("RenderableObject");
    auto& mesh_comp = entity.add<slrye::MeshComponent>();
    mesh_comp.asset_path = "cube";

    auto& mat_comp = entity.add<slrye::MaterialComponent>();
    mat_comp.material = slrye::Material::create_default();

    auto& model_comp = entity.add<slrye::ModelComponent>();
    model_comp.file_path = "assets/models/character.glb";

    REQUIRE(entity.has<slrye::MeshComponent>());
    REQUIRE(entity.has<slrye::MaterialComponent>());
    REQUIRE(entity.has<slrye::ModelComponent>());
    CHECK(entity.get<slrye::MeshComponent>().asset_path == "cube");
    CHECK(entity.get<slrye::MaterialComponent>().material != nullptr);
}
