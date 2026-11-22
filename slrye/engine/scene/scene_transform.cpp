#include <engine/scene/scene_transform.h>

#include <optional>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <engine/scene/components.h>
#include <engine/scene/scene.h>

namespace slrye::engine
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

        const TransformComponent* transform =
            scene.try_get_component<TransformComponent>(current);
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
