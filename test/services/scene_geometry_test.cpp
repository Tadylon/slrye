#include <services/rendering/scene_geometry.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include <domain/scene.h>

TEST_CASE("Scene geometry always contains a ground grid")
{
    slrye::Scene scene;

    const slrye::SceneGeometry geometry = slrye::build_scene_geometry(scene);

    CHECK_FALSE(geometry.vertices.empty());
    CHECK(geometry.vertices.size() % 3 == 0);

    const slrye::SceneGeometry without_grid =
        slrye::build_scene_geometry(scene, std::nullopt, false);
    CHECK(without_grid.vertices.empty());
}

TEST_CASE("Scene geometry transforms entity proxies and honors disabled colliders")
{
    slrye::Scene scene;
    slrye::Entity parent = scene.create_entity("Parent");
    parent.get<slrye::TransformComponent>().position = glm::vec3(10.0f, 0.0f, 0.0f);
    slrye::Entity child = scene.create_entity("Child");
    REQUIRE(child.set_parent(parent));

    const slrye::SceneGeometry with_child = slrye::build_scene_geometry(scene, child.id());
    const bool has_selected_color = std::any_of(
        with_child.vertices.begin(),
        with_child.vertices.end(),
        [](const slrye::SceneVertex& vertex)
        {
            return vertex.color.r > 0.9f && vertex.color.g < 0.7f;
        });
    CHECK(has_selected_color);

    child.add<slrye::ColliderComponent>().enabled = false;
    const slrye::SceneGeometry without_child = slrye::build_scene_geometry(scene, child.id());
    CHECK(without_child.vertices.size() < with_child.vertices.size());
}
