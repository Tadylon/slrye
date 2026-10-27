#include <services/picking/picking.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <domain/scene.h>

TEST_CASE("Picking returns the closest entity along a ray")
{
    slrye::Scene scene;
    slrye::Entity far_entity = scene.create_entity("Far");
    far_entity.get<slrye::TransformComponent>().position = glm::vec3(0.0f, 0.0f, -8.0f);
    slrye::Entity near_entity = scene.create_entity("Near");
    near_entity.get<slrye::TransformComponent>().position = glm::vec3(0.0f, 0.0f, -3.0f);

    const std::optional<slrye::PickResult> result = slrye::pick_entity(
        scene,
        slrye::Ray{glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)});

    REQUIRE(result.has_value());
    CHECK(result->entity == near_entity.id());
    CHECK(result->distance == Catch::Approx(2.5f));
}

TEST_CASE("Picking respects parent transforms and collider shapes")
{
    slrye::Scene scene;
    slrye::Entity root = scene.create_entity("Root");
    root.get<slrye::TransformComponent>().position = glm::vec3(2.0f, 1.0f, 0.0f);
    root.add<slrye::ColliderComponent>().enabled = false;
    slrye::Entity sphere = scene.create_entity("Sphere");
    sphere.get<slrye::TransformComponent>().position = glm::vec3(0.0f, 0.0f, -5.0f);
    slrye::ColliderComponent& collider = sphere.add<slrye::ColliderComponent>();
    collider.type = slrye::ColliderType::Sphere;
    collider.radius = 1.0f;
    REQUIRE(sphere.set_parent(root));

    const std::optional<slrye::PickResult> result = slrye::pick_entity(
        scene,
        slrye::Ray{glm::vec3(2.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)});

    REQUIRE(result.has_value());
    CHECK(result->entity == sphere.id());
    CHECK(result->distance == Catch::Approx(4.0f));
}

TEST_CASE("Disabled colliders do not participate in viewport picking")
{
    slrye::Scene scene;
    slrye::Entity entity = scene.create_entity("Disabled");
    slrye::ColliderComponent& collider = entity.add<slrye::ColliderComponent>();
    collider.enabled = false;

    const std::optional<slrye::PickResult> result = slrye::pick_entity(
        scene,
        slrye::Ray{glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)});

    CHECK_FALSE(result.has_value());
}
