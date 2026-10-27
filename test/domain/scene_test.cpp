#include <domain/scene.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

TEST_CASE("Scene creates entities with stable identity and default components")
{
    slrye::Scene scene("Main");
    slrye::Entity player = scene.create_entity("Player");

    REQUIRE(player.is_valid());
    REQUIRE(scene.entity_count() == 1);
    CHECK(scene.find(player.id()).id() == player.id());
    CHECK(player.name() == "Player");
    CHECK(player.has<slrye::NameComponent>());
    CHECK(player.has<slrye::TransformComponent>());
    CHECK(player.has<slrye::HierarchyComponent>());

    const slrye::TransformComponent& transform = player.get<slrye::TransformComponent>();
    CHECK(transform.position.x == 0.0f);
    CHECK(transform.position.y == 0.0f);
    CHECK(transform.position.z == 0.0f);
    CHECK(transform.scale.x == 1.0f);
    CHECK(transform.scale.y == 1.0f);
    CHECK(transform.scale.z == 1.0f);
}

TEST_CASE("Entity components can be added, edited, queried, and removed through EnTT")
{
    slrye::Scene scene;
    slrye::Entity camera_entity = scene.create_entity("Main Camera");

    slrye::CameraComponent& camera = camera_entity.add<slrye::CameraComponent>();
    camera.vertical_fov_degrees = 75.0f;
    camera.primary = false;

    REQUIRE(camera_entity.has<slrye::CameraComponent>());
    CHECK(camera_entity.get<slrye::CameraComponent>().vertical_fov_degrees == 75.0f);
    CHECK_FALSE(camera_entity.get<slrye::CameraComponent>().primary);
    REQUIRE(camera_entity.try_get<slrye::CameraComponent>() != nullptr);
    REQUIRE_THROWS(camera_entity.add<slrye::CameraComponent>());

    CHECK(camera_entity.remove<slrye::CameraComponent>());
    CHECK_FALSE(camera_entity.has<slrye::CameraComponent>());
    CHECK(camera_entity.try_get<slrye::CameraComponent>() == nullptr);
    CHECK_FALSE(camera_entity.remove<slrye::CameraComponent>());

    std::size_t named_entity_count = 0;
    scene.registry().view<slrye::NameComponent>().each(
        [&named_entity_count](slrye::NameComponent&)
        {
            ++named_entity_count;
        });
    CHECK(named_entity_count == 1);
}

TEST_CASE("Scene maintains parent-child relationships and rejects cycles")
{
    slrye::Scene scene;
    slrye::Entity root = scene.create_entity("Root");
    slrye::Entity child = scene.create_entity("Child");
    slrye::Entity grandchild = scene.create_entity("Grandchild");
    slrye::Entity other_root = scene.create_entity("Other Root");

    REQUIRE(child.set_parent(root));
    REQUIRE(grandchild.set_parent(child));
    CHECK(child.parent() == root.id());
    CHECK(grandchild.parent() == child.id());
    CHECK(root.children() == std::vector<slrye::EntityId>{child.id()});
    CHECK(child.children() == std::vector<slrye::EntityId>{grandchild.id()});

    CHECK_FALSE(root.set_parent(root));
    CHECK_FALSE(root.set_parent(grandchild));

    const std::vector<slrye::EntityId> roots = scene.root_entities();
    CHECK(roots.size() == 2);
    CHECK(std::find(roots.begin(), roots.end(), root.id()) != roots.end());
    CHECK(std::find(roots.begin(), roots.end(), other_root.id()) != roots.end());

    REQUIRE(child.clear_parent());
    CHECK_FALSE(child.parent().has_value());
    CHECK(root.children().empty());
    CHECK(child.set_parent(root));

    REQUIRE(child.set_parent(other_root));
    CHECK(child.parent() == other_root.id());
    CHECK(root.children().empty());
    CHECK(other_root.children() == std::vector<slrye::EntityId>{child.id()});
}

TEST_CASE("Destroying a parent destroys descendants and invalidates stale handles")
{
    slrye::Scene scene;
    slrye::Entity root = scene.create_entity("Root");
    slrye::Entity child = scene.create_entity("Child");
    slrye::Entity grandchild = scene.create_entity("Grandchild");
    REQUIRE(child.set_parent(root));
    REQUIRE(grandchild.set_parent(child));

    REQUIRE(root.destroy());

    CHECK_FALSE(root.is_valid());
    CHECK_FALSE(child.is_valid());
    CHECK_FALSE(grandchild.is_valid());
    CHECK(scene.entity_count() == 0);
    CHECK(scene.registry().storage<slrye::NameComponent>().size() == 0);

    slrye::Entity replacement = scene.create_entity("Replacement");
    CHECK_FALSE(root.is_valid());
    CHECK(replacement.is_valid());
    CHECK(replacement.id() != root.id());
}

TEST_CASE("Scene accepts fixed ids for future deserialization without allowing duplicates")
{
    slrye::Scene scene;
    const slrye::EntityId fixed_id(500);

    slrye::Entity entity = scene.create_entity(fixed_id, "Loaded Entity");
    slrye::Entity duplicate = scene.create_entity(fixed_id, "Duplicate");

    REQUIRE(entity.is_valid());
    CHECK(entity.id() == fixed_id);
    CHECK_FALSE(duplicate.is_valid());
    CHECK(scene.entity_count() == 1);
    CHECK_FALSE(scene.create_entity(slrye::EntityId{}, "Invalid Id").is_valid());
}

TEST_CASE("Scene tracks dirty state for persistent changes")
{
    slrye::Scene scene("Untitled");
    CHECK_FALSE(scene.is_dirty());
    const std::uint64_t initial_revision = scene.revision();

    slrye::Entity entity = scene.create_entity("Entity");
    CHECK(scene.is_dirty());
    CHECK(scene.revision() > initial_revision);
    scene.clear_dirty();
    const std::uint64_t saved_revision = scene.revision();

    entity.set_name("Renamed Entity");
    CHECK(scene.is_dirty());
    CHECK(scene.revision() > saved_revision);
    scene.clear_dirty();

    entity.add<slrye::LightComponent>();
    CHECK(scene.is_dirty());
}
