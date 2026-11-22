#pragma once

#include <optional>

#include <engine/renderer/camera.h>
// #include <engine/scene/entity_id.h>

namespace slrye
{

class Scene;

struct PickResult
{
    EntityId entity;
    float distance = 0.0f;
};

[[nodiscard]] std::optional<PickResult> pick_entity(
    const Scene& scene,
    const Ray& ray);

}
