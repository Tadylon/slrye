#include <domain/scene_transform.h>

#include <catch2/catch_test_macros.hpp>

#include <glm/vec4.hpp>

#include <domain/scene.h>

TEST_CASE("World transforms compose parent and child local transforms")
{
    slrye::Scene scene;
    slrye::Entity parent = scene.create_entity("Parent");
    slrye::Entity child = scene.create_entity("Child");
    parent.get<slrye::TransformComponent>().position = glm::vec3(3.0f, 1.0f, -2.0f);
    child.get<slrye::TransformComponent>().position = glm::vec3(0.0f, 2.0f, 0.0f);
    REQUIRE(child.set_parent(parent));

    glm::mat4 world;
    REQUIRE(slrye::world_transform_matrix(scene, child.id(), world));

    const glm::vec3 position(world[3]);
    CHECK(position == glm::vec3(3.0f, 3.0f, -2.0f));
}

TEST_CASE("World transform lookup rejects unknown entities")
{
    slrye::Scene scene;
    glm::mat4 world(1.0f);

    CHECK_FALSE(slrye::world_transform_matrix(scene, slrye::EntityId(42), world));
}
