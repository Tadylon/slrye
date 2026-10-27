#include <services/rendering/scene_geometry.h>

#include <array>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <domain/components.h>
#include <domain/scene.h>
#include <domain/scene_transform.h>

namespace slrye
{
namespace
{

using VertexPositions = std::array<glm::vec3, 4>;

SceneVertex transform_vertex(
    const glm::vec3& position,
    const glm::vec3& normal,
    const glm::vec3& color,
    const glm::mat4& world)
{
    const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(world)));
    SceneVertex vertex;
    vertex.position = glm::vec3(world * glm::vec4(position, 1.0f));
    vertex.normal = glm::normalize(normal_matrix * normal);
    vertex.color = color;
    return vertex;
}

void append_triangle(
    SceneGeometry& geometry,
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& normal,
    const glm::vec3& color,
    const glm::mat4& world)
{
    geometry.vertices.push_back(transform_vertex(a, normal, color, world));
    geometry.vertices.push_back(transform_vertex(b, normal, color, world));
    geometry.vertices.push_back(transform_vertex(c, normal, color, world));
}

void append_quad(
    SceneGeometry& geometry,
    const VertexPositions& positions,
    const glm::vec3& normal,
    const glm::vec3& color,
    const glm::mat4& world)
{
    append_triangle(geometry, positions[0], positions[1], positions[2], normal, color, world);
    append_triangle(geometry, positions[0], positions[2], positions[3], normal, color, world);
}

void append_box(
    SceneGeometry& geometry,
    const glm::vec3& half_extents,
    const glm::vec3& color,
    const glm::mat4& world)
{
    const float x = half_extents.x;
    const float y = half_extents.y;
    const float z = half_extents.z;

    append_quad(
        geometry,
        {glm::vec3(x, -y, -z), glm::vec3(x, y, -z), glm::vec3(x, y, z), glm::vec3(x, -y, z)},
        glm::vec3(1.0f, 0.0f, 0.0f), color, world);
    append_quad(
        geometry,
        {glm::vec3(-x, -y, z), glm::vec3(-x, y, z), glm::vec3(-x, y, -z), glm::vec3(-x, -y, -z)},
        glm::vec3(-1.0f, 0.0f, 0.0f), color, world);
    append_quad(
        geometry,
        {glm::vec3(-x, y, -z), glm::vec3(-x, y, z), glm::vec3(x, y, z), glm::vec3(x, y, -z)},
        glm::vec3(0.0f, 1.0f, 0.0f), color, world);
    append_quad(
        geometry,
        {glm::vec3(-x, -y, z), glm::vec3(-x, -y, -z), glm::vec3(x, -y, -z), glm::vec3(x, -y, z)},
        glm::vec3(0.0f, -1.0f, 0.0f), color, world);
    append_quad(
        geometry,
        {glm::vec3(-x, -y, z), glm::vec3(x, -y, z), glm::vec3(x, y, z), glm::vec3(-x, y, z)},
        glm::vec3(0.0f, 0.0f, 1.0f), color, world);
    append_quad(
        geometry,
        {glm::vec3(x, -y, -z), glm::vec3(-x, -y, -z), glm::vec3(-x, y, -z), glm::vec3(x, y, -z)},
        glm::vec3(0.0f, 0.0f, -1.0f), color, world);
}

void append_sphere(
    SceneGeometry& geometry,
    float radius,
    const glm::vec3& color,
    const glm::mat4& world)
{
    constexpr int rings = 12;
    constexpr int segments = 18;

    for (int ring = 0; ring < rings; ++ring)
    {
        const float latitude_a = glm::pi<float>() * static_cast<float>(ring) / rings;
        const float latitude_b = glm::pi<float>() * static_cast<float>(ring + 1) / rings;
        for (int segment = 0; segment < segments; ++segment)
        {
            const float longitude_a =
                glm::two_pi<float>() * static_cast<float>(segment) / segments;
            const float longitude_b =
                glm::two_pi<float>() * static_cast<float>(segment + 1) / segments;

            const auto point = [radius](float latitude, float longitude)
            {
                const glm::vec3 normal(
                    std::sin(latitude) * std::cos(longitude),
                    std::cos(latitude),
                    std::sin(latitude) * std::sin(longitude));
                return std::pair<glm::vec3, glm::vec3>(normal * radius, normal);
            };

            const auto [position_a, normal_a] = point(latitude_a, longitude_a);
            const auto [position_b, normal_b] = point(latitude_a, longitude_b);
            const auto [position_c, normal_c] = point(latitude_b, longitude_b);
            const auto [position_d, normal_d] = point(latitude_b, longitude_a);

            geometry.vertices.push_back(transform_vertex(position_a, normal_a, color, world));
            geometry.vertices.push_back(transform_vertex(position_b, normal_b, color, world));
            geometry.vertices.push_back(transform_vertex(position_c, normal_c, color, world));
            geometry.vertices.push_back(transform_vertex(position_a, normal_a, color, world));
            geometry.vertices.push_back(transform_vertex(position_c, normal_c, color, world));
            geometry.vertices.push_back(transform_vertex(position_d, normal_d, color, world));
        }
    }
}

void append_grid(SceneGeometry& geometry)
{
    constexpr int half_size = 10;
    constexpr float width = 0.008f;
    const glm::mat4 identity(1.0f);

    for (int index = -half_size; index <= half_size; ++index)
    {
        const float offset = static_cast<float>(index);
        const glm::vec3 color = index == 0
            ? glm::vec3(0.25f, 0.45f, 0.75f)
            : glm::vec3(0.24f, 0.25f, 0.27f);
        append_quad(
            geometry,
            {
                glm::vec3(offset - width, 0.0f, -half_size),
                glm::vec3(offset - width, 0.0f, half_size),
                glm::vec3(offset + width, 0.0f, half_size),
                glm::vec3(offset + width, 0.0f, -half_size)},
            glm::vec3(0.0f, 1.0f, 0.0f), color, identity);

        const glm::vec3 opposite_color = index == 0
            ? glm::vec3(0.75f, 0.32f, 0.25f)
            : glm::vec3(0.24f, 0.25f, 0.27f);
        append_quad(
            geometry,
            {
                glm::vec3(-half_size, 0.0f, offset - width),
                glm::vec3(half_size, 0.0f, offset - width),
                glm::vec3(half_size, 0.0f, offset + width),
                glm::vec3(-half_size, 0.0f, offset + width)},
            glm::vec3(0.0f, 1.0f, 0.0f), opposite_color, identity);
    }
}

glm::vec3 entity_color(const Scene& scene, EntityId id, bool selected)
{
    if (selected)
    {
        return glm::vec3(0.95f, 0.55f, 0.16f);
    }

    const entt::entity handle = scene.entity_handle(id);
    if (scene.registry().all_of<LightComponent>(handle))
    {
        return glm::vec3(0.95f, 0.78f, 0.28f);
    }
    if (scene.registry().all_of<CameraComponent>(handle))
    {
        return glm::vec3(0.28f, 0.78f, 0.66f);
    }
    return glm::vec3(0.42f, 0.58f, 0.82f);
}

}

SceneGeometry build_scene_geometry(
    const Scene& scene,
    std::optional<EntityId> selected_entity,
    bool show_grid)
{
    SceneGeometry geometry;
    geometry.vertices.reserve(scene.entity_count() * 36 + 756);
    if (show_grid)
    {
        append_grid(geometry);
    }

    for (EntityId entity_id : scene.entities())
    {
        glm::mat4 world;
        if (!world_transform_matrix(scene, entity_id, world))
        {
            continue;
        }

        const bool selected =
            selected_entity.has_value() && *selected_entity == entity_id;
        const glm::vec3 color = entity_color(scene, entity_id, selected);
        const entt::entity handle = scene.entity_handle(entity_id);
        const ColliderComponent* collider =
            scene.registry().try_get<ColliderComponent>(handle);

        if (collider == nullptr)
        {
            append_box(geometry, glm::vec3(0.5f), color, world);
            continue;
        }

        if (!collider->enabled)
        {
            continue;
        }

        switch (collider->type)
        {
        case ColliderType::Box:
            append_box(geometry, collider->half_extents, color, world);
            break;
        case ColliderType::Sphere:
            append_sphere(geometry, collider->radius, color, world);
            break;
        case ColliderType::Capsule:
            append_box(
                geometry,
                glm::vec3(collider->radius, collider->height * 0.5f, collider->radius),
                color,
                world);
            break;
        }
    }

    return geometry;
}

}
