#pragma once

#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <entt/entity/registry.hpp>

#include <domain/components.h>
#include <domain/entity.h>

namespace slrye
{

class SceneSerializer;

class Scene
{
public:
    explicit Scene(std::string name = "Untitled");

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    [[nodiscard]] const std::string& name() const noexcept;
    void set_name(std::string name);

    Entity create_entity(std::string name = "Entity");
    Entity create_entity(EntityId id, std::string name = "Entity");
    bool destroy_entity(EntityId id);

    [[nodiscard]] Entity find(EntityId id);
    [[nodiscard]] bool contains(EntityId id) const noexcept;
    [[nodiscard]] std::size_t entity_count() const noexcept;
    [[nodiscard]] const std::vector<EntityId>& entities() const noexcept;
    [[nodiscard]] std::vector<EntityId> root_entities() const;

    bool set_parent(EntityId child, EntityId parent);
    bool clear_parent(EntityId child);
    [[nodiscard]] std::optional<EntityId> parent(EntityId id) const;
    [[nodiscard]] std::vector<EntityId> children(EntityId id) const;

    [[nodiscard]] bool is_dirty() const noexcept;
    void mark_dirty() noexcept;
    void clear_dirty() noexcept;

    [[nodiscard]] entt::registry& registry() noexcept;
    [[nodiscard]] const entt::registry& registry() const noexcept;

private:
    friend class SceneSerializer;

    [[nodiscard]] EntityId generate_id();
    [[nodiscard]] entt::entity handle_for(EntityId id) const noexcept;

    std::string name_;
    entt::registry registry_;
    std::unordered_map<EntityId, entt::entity, EntityIdHash> entities_;
    std::vector<EntityId> entity_order_;
    std::mt19937_64 id_generator_;
    bool dirty_ = false;
};

}
