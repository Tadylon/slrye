#pragma once

#include <optional>
#include <string>

#include <glm/vec3.hpp>

#include <engine/scene/entity_id.h>

namespace slrye
{

class Scene;
class Entity;

class InspectorPanel
{
public:
    InspectorPanel() = default;

    void set_scene(Scene* scene) noexcept;
    void render();

    void set_selected_entity(std::optional<engine::EntityId> entity) noexcept;
    [[nodiscard]] std::optional<engine::EntityId> selected_entity() const noexcept;

    void set_visible(bool visible) noexcept;
    [[nodiscard]] bool is_visible() const noexcept;

private:
    void draw_components(Entity entity);
    static bool draw_vec3_control(const char* label, glm::vec3& values, float reset_value = 0.0f);

    Scene* scene_ = nullptr;
    std::optional<engine::EntityId> selected_entity_;
    bool visible_ = true;
};

}
