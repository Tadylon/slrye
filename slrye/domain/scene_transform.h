#pragma once

#include <glm/mat4x4.hpp>

#include <domain/entity_id.h>

namespace slrye
{

class Scene;

[[nodiscard]] glm::mat4 local_transform_matrix(const struct TransformComponent& transform) noexcept;
[[nodiscard]] bool world_transform_matrix(
    const Scene& scene,
    EntityId entity,
    glm::mat4& result) noexcept;

}
