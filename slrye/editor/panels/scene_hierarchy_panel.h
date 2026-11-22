#pragma once

#include <optional>
#include <string>

#include <engine/scene/entity_id.h>

namespace slrye::engine
{

class Scene;

}
namespace slrye {
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;

    void set_scene(engine::Scene* scene) noexcept;
    void render();

    void set_selected_entity(std::optional<engine::EntityId> entity) noexcept;
    [[nodiscard]] std::optional<engine::EntityId> selected_entity() const noexcept;

    void set_visible(bool visible) noexcept;
    [[nodiscard]] bool is_visible() const noexcept;

private:
    void draw_entity_node(engine::EntityId id);
    void draw_context_menu();

    engine::Scene* scene_ = nullptr;
    std::optional<engine::EntityId> selected_entity_;
    bool visible_ = true;
};

}
