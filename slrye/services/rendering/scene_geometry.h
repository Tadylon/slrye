#pragma once

#include <optional>
#include <vector>

#include <glm/vec3.hpp>

#include <domain/entity_id.h>

namespace slrye
{

class Scene;

struct SceneVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec3 color{1.0f};
};

struct SceneGeometry
{
    std::vector<SceneVertex> vertices;
};

[[nodiscard]] SceneGeometry build_scene_geometry(
    const Scene& scene,
    std::optional<EntityId> selected_entity = std::nullopt,
    bool show_grid = true);

}
