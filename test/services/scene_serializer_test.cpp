#include <services/serialization/scene_serializer.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <domain/scene.h>
#include <support/temporary_directory.h>

TEST_CASE("Scene serializer preserves stable ids, components, and hierarchy")
{
    TemporaryDirectory directory;
    const std::filesystem::path path = directory.path() / "levels" / "main.slscene";

    slrye::Scene original("Main Level");
    slrye::Entity root = original.create_entity("Root");
    root.get<slrye::TransformComponent>().position = glm::vec3(1.0f, 2.0f, 3.0f);

    slrye::Entity camera = original.create_entity("Main Camera");
    slrye::CameraComponent& camera_component = camera.add<slrye::CameraComponent>();
    camera_component.vertical_fov_degrees = 72.0f;
    camera_component.near_clip = 0.05f;
    camera_component.far_clip = 500.0f;
    camera_component.primary = false;

    slrye::Entity light = original.create_entity("Sun");
    slrye::LightComponent& light_component = light.add<slrye::LightComponent>();
    light_component.type = slrye::LightType::Directional;
    light_component.color = glm::vec3(1.0f, 0.9f, 0.8f);
    light_component.intensity = 2.5f;

    slrye::Entity collider = original.create_entity("Floor");
    slrye::ColliderComponent& collider_component = collider.add<slrye::ColliderComponent>();
    collider_component.type = slrye::ColliderType::Box;
    collider_component.half_extents = glm::vec3(4.0f, 0.5f, 4.0f);
    collider_component.enabled = false;

    REQUIRE(camera.set_parent(root));
    REQUIRE(light.set_parent(root));

    std::string error;
    REQUIRE(slrye::SceneSerializer::save(path, original, &error));
    CHECK(error.empty());

    std::unique_ptr<slrye::Scene> loaded = slrye::SceneSerializer::load(path, &error);
    REQUIRE(loaded != nullptr);
    CHECK(error.empty());
    CHECK(loaded->name() == "Main Level");
    CHECK(loaded->entity_count() == 4);
    CHECK_FALSE(loaded->is_dirty());

    slrye::Entity loaded_root = loaded->find(root.id());
    slrye::Entity loaded_camera = loaded->find(camera.id());
    slrye::Entity loaded_light = loaded->find(light.id());
    slrye::Entity loaded_collider = loaded->find(collider.id());
    REQUIRE(loaded_root.is_valid());
    REQUIRE(loaded_camera.is_valid());
    REQUIRE(loaded_light.is_valid());
    REQUIRE(loaded_collider.is_valid());
    CHECK(loaded_root.name() == "Root");
    CHECK(loaded_root.get<slrye::TransformComponent>().position == glm::vec3(1.0f, 2.0f, 3.0f));
    CHECK(loaded_camera.parent() == root.id());
    CHECK(loaded_light.parent() == root.id());

    const slrye::CameraComponent& loaded_camera_component =
        loaded_camera.get<slrye::CameraComponent>();
    CHECK(loaded_camera_component.vertical_fov_degrees == Catch::Approx(72.0f));
    CHECK(loaded_camera_component.near_clip == Catch::Approx(0.05f));
    CHECK(loaded_camera_component.far_clip == Catch::Approx(500.0f));
    CHECK_FALSE(loaded_camera_component.primary);

    const slrye::LightComponent& loaded_light_component =
        loaded_light.get<slrye::LightComponent>();
    CHECK(loaded_light_component.type == slrye::LightType::Directional);
    CHECK(loaded_light_component.intensity == Catch::Approx(2.5f));

    const slrye::ColliderComponent& loaded_collider_component =
        loaded_collider.get<slrye::ColliderComponent>();
    CHECK(loaded_collider_component.type == slrye::ColliderType::Box);
    CHECK(loaded_collider_component.half_extents == glm::vec3(4.0f, 0.5f, 4.0f));
    CHECK_FALSE(loaded_collider_component.enabled);
}

TEST_CASE("Scene serializer rejects malformed files and unknown versions")
{
    TemporaryDirectory directory;
    std::string error;

    const std::filesystem::path malformed = directory.write_file(
        "malformed.slscene",
        "version: 1\nscene: [unterminated\n");
    CHECK(slrye::SceneSerializer::load(malformed, &error) == nullptr);
    CHECK_FALSE(error.empty());

    const std::filesystem::path future = directory.write_file(
        "future.slscene",
        "version: 99\nscene:\n  name: Future\n  entities: []\n");
    CHECK(slrye::SceneSerializer::load(future, &error) == nullptr);
    CHECK(error.find("version") != std::string::npos);
}
