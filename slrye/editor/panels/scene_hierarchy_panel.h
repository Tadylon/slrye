#pragma once

#include <optional>
#include <string>

#include <engine/scene/entity_id.h>

namespace slrye
{

class Scene;

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;

    void set_scene(Scene* scene) noexcept;
    void render();

    void set_selected_entity(std::optional<EntityId> entity) noexcept;
    [[nodiscard]] std::optional<EntityId> selected_entity() const noexcept;

    void set_visible(bool visible) noexcept;
    [[nodiscard]] bool is_visible() const noexcept;

private:
    void draw_entity_node(EntityId id);
    void draw_context_menu();

    Scene* scene_ = nullptr;
    std::optional<EntityId> selected_entity_;
    bool visible_ = true;
};

}
