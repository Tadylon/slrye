#include <services/picking/picking.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

#include <domain/components.h>
#include <domain/scene.h>
#include <domain/scene_transform.h>

namespace slrye
{
namespace
{

constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr float kDirectionEpsilon = 0.000001f;

float intersect_box(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& extents)
{
    float minimum = -kInfinity;
    float maximum = kInfinity;

    for (int axis = 0; axis < 3; ++axis)
    {
        const float sign = direction[axis] < 0.0f ? -1.0f : 1.0f;
        const float inverse_direction = 1.0f / (direction[axis] + sign * kDirectionEpsilon);
        float near_distance = (-extents[axis] - origin[axis]) * inverse_direction;
        float far_distance = (extents[axis] - origin[axis]) * inverse_direction;
        if (near_distance > far_distance)
        {
            std::swap(near_distance, far_distance);
        }

        minimum = std::max(minimum, near_distance);
        maximum = std::min(maximum, far_distance);
        if (minimum > maximum)
        {
            return kInfinity;
        }
    }

    if (maximum < 0.0f)
    {
        return kInfinity;
    }
    return minimum >= 0.0f ? minimum : maximum;
}

float intersect_sphere(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::vec3& center,
    float radius)
{
    const glm::vec3 offset = origin - center;
    const float a = glm::dot(direction, direction);
    const float b = 2.0f * glm::dot(offset, direction);
    const float c = glm::dot(offset, offset) - radius * radius;
    const float discriminant = b * b - 4.0f * a * c;
    if (a <= kDirectionEpsilon || discriminant < 0.0f)
    {
        return kInfinity;
    }

    const float root = std::sqrt(discriminant);
    const float first = (-b - root) / (2.0f * a);
    const float second = (-b + root) / (2.0f * a);
    if (first >= 0.0f)
    {
        return first;
    }
    return second >= 0.0f ? second : kInfinity;
}

float intersect_capsule(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float radius,
    float height)
{
    const float segment_half_height = std::max(height * 0.5f - radius, 0.0f);
    float closest = std::min(
        intersect_sphere(
            origin,
            direction,
            glm::vec3(0.0f, segment_half_height, 0.0f),
            radius),
        intersect_sphere(
            origin,
            direction,
            glm::vec3(0.0f, -segment_half_height, 0.0f),
            radius));

    const float a = direction.x * direction.x + direction.z * direction.z;
    const float b = 2.0f * (origin.x * direction.x + origin.z * direction.z);
    const float c = origin.x * origin.x + origin.z * origin.z - radius * radius;
    const float discriminant = b * b - 4.0f * a * c;
    if (a <= kDirectionEpsilon || discriminant < 0.0f)
    {
        return closest;
    }

    const float root = std::sqrt(discriminant);
    const std::array<float, 2> candidates{
        (-b - root) / (2.0f * a),
        (-b + root) / (2.0f * a)};
    for (float candidate : candidates)
    {
        const float y = origin.y + candidate * direction.y;
        if (candidate >= 0.0f && y >= -segment_half_height && y <= segment_half_height)
        {
            closest = std::min(closest, candidate);
        }
    }

    return closest;
}

float intersect_collider(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const ColliderComponent& collider)
{
    switch (collider.type)
    {
    case ColliderType::Box:
        return intersect_box(origin, direction, collider.half_extents);
    case ColliderType::Sphere:
        return intersect_sphere(origin, direction, glm::vec3(0.0f), collider.radius);
    case ColliderType::Capsule:
        return intersect_capsule(origin, direction, collider.radius, collider.height);
    }
    return kInfinity;
}

}

std::optional<PickResult> pick_entity(const Scene& scene, const Ray& ray)
{
    const float direction_length = glm::length(ray.direction);
    if (direction_length <= kDirectionEpsilon)
    {
        return std::nullopt;
    }

    const Ray normalized_ray{
        ray.origin,
        ray.direction / direction_length};
    std::optional<PickResult> closest;

    for (EntityId entity_id : scene.entities())
    {
        const entt::entity handle = scene.entity_handle(entity_id);
        if (handle == entt::null)
        {
            continue;
        }

        const ColliderComponent* collider =
            scene.registry().try_get<ColliderComponent>(handle);
        if (collider != nullptr && !collider->enabled)
        {
            continue;
        }

        glm::mat4 world_transform;
        if (!world_transform_matrix(scene, entity_id, world_transform))
        {
            continue;
        }

        const float determinant = glm::determinant(world_transform);
        if (!std::isfinite(determinant) || std::abs(determinant) < kDirectionEpsilon)
        {
            continue;
        }

        const glm::mat4 inverse_world = glm::inverse(world_transform);
        const glm::vec4 local_origin = inverse_world * glm::vec4(normalized_ray.origin, 1.0f);
        const glm::vec4 local_direction = inverse_world * glm::vec4(normalized_ray.direction, 0.0f);
        const glm::vec3 origin(local_origin);
        const glm::vec3 direction(local_direction);

        const ColliderComponent default_collider;
        const float distance = intersect_collider(
            origin,
            direction,
            collider != nullptr ? *collider : default_collider);
        if (std::isfinite(distance) &&
            (!closest.has_value() || distance < closest->distance))
        {
            closest = PickResult{entity_id, distance};
        }
    }

    return closest;
}

}
