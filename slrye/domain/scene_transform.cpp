#include <domain/scene_transform.h>

#include <optional>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <domain/components.h>
#include <domain/scene.h>

namespace slrye
{

glm::mat4 local_transform_matrix(const TransformComponent& transform) noexcept
{
    glm::mat4 matrix = glm::translate(glm::mat4(1.0f), transform.position);
    matrix *= glm::mat4_cast(transform.rotation);
    matrix = glm::scale(matrix, transform.scale);
    return matrix;
}

bool world_transform_matrix(
    const Scene& scene,
    EntityId entity,
    glm::mat4& result) noexcept
{
    result = glm::mat4(1.0f);
    if (!entity.is_valid())
    {
        return false;
    }

    std::vector<EntityId> visited;
    visited.reserve(scene.entity_count());

    EntityId current = entity;
    while (current.is_valid())
    {
        if (!scene.contains(current))
        {
            return false;
        }

        for (EntityId previous : visited)
        {
            if (previous == current)
            {
                return false;
            }
        }
        visited.push_back(current);

        const entt::entity handle = scene.entity_handle(current);
        const TransformComponent* transform =
            scene.registry().try_get<TransformComponent>(handle);
        if (transform == nullptr)
        {
            return false;
        }

        result = local_transform_matrix(*transform) * result;
        const std::optional<EntityId> parent = scene.parent(current);
        current = parent.value_or(EntityId{});
    }

    return true;
}

}
