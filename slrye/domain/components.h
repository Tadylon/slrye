#pragma once

#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <domain/entity_id.h>

namespace slrye
{

struct NameComponent
{
    std::string value = "Entity";
};

struct TransformComponent
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct HierarchyComponent
{
    EntityId parent;
    std::vector<EntityId> children;
};

struct CameraComponent
{
    float vertical_fov_degrees = 60.0f;
    float near_clip = 0.1f;
    float far_clip = 1000.0f;
    bool primary = true;
};

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct LightComponent
{
    LightType type = LightType::Point;
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float inner_cone_degrees = 20.0f;
    float outer_cone_degrees = 30.0f;
};

enum class ColliderType
{
    Box,
    Sphere,
    Capsule
};

struct ColliderComponent
{
    ColliderType type = ColliderType::Box;
    glm::vec3 half_extents{0.5f, 0.5f, 0.5f};
    float radius = 0.5f;
    float height = 2.0f;
    bool enabled = true;
};

}
