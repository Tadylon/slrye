#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/registry.hpp>

#include <engine/scene/components.h>
#include <engine/scene/entity.h>
#include <engine/scene/entity_id.h>

namespace slrye::engine
{

class Scene
{
public:
    explicit Scene(std::string name = "Untitled");
    ~Scene() = default;

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
    [[nodiscard]] std::uint64_t revision() const noexcept;
    void mark_dirty() noexcept;
    void clear_dirty() noexcept;

    // Component Facade Methods
    template<typename T>
    [[nodiscard]] bool has_component(EntityId id) const noexcept
    {
        const entt::entity handle = handle_for(id);
        return handle != entt::null && registry_.all_of<T>(handle);
    }

    template<typename T>
    [[nodiscard]] T* try_get_component(EntityId id) noexcept
    {
        const entt::entity handle = handle_for(id);
        return handle != entt::null ? registry_.try_get<T>(handle) : nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* try_get_component(EntityId id) const noexcept
    {
        const entt::entity handle = handle_for(id);
        return handle != entt::null ? registry_.try_get<T>(handle) : nullptr;
    }

    template<typename T>
    T& get_component(EntityId id)
    {
        T* comp = try_get_component<T>(id);
        if (comp == nullptr)
        {
            throw std::out_of_range("Entity does not have the requested component");
        }
        return *comp;
    }

    template<typename T>
    const T& get_component(EntityId id) const
    {
        const T* comp = try_get_component<T>(id);
        if (comp == nullptr)
        {
            throw std::out_of_range("Entity does not have the requested component");
        }
        return *comp;
    }

    template<typename T, typename... Args>
    T& add_component(EntityId id, Args&&... args)
    {
        const entt::entity handle = handle_for(id);
        if (handle == entt::null)
        {
            throw std::logic_error("Cannot add component to an invalid entity");
        }
        if (registry_.all_of<T>(handle))
        {
            throw std::logic_error("Entity already has the requested component");
        }
        T& component = registry_.emplace<T>(handle, std::forward<Args>(args)...);
        mark_dirty();
        return component;
    }

    template<typename T>
    bool remove_component(EntityId id)
    {
        const entt::entity handle = handle_for(id);
        if (handle == entt::null || !registry_.all_of<T>(handle))
        {
            return false;
        }
        registry_.remove<T>(handle);
        mark_dirty();
        return true;
    }

    // High-performance view iteration Facade
    template<typename... Components, typename Func>
    void for_each(Func&& func)
    {
        auto view = registry_.view<Components...>();
        for (auto handle : view)
        {
            auto it = entity_to_id_.find(handle);
            if (it != entity_to_id_.end())
            {
                func(it->second, view.template get<Components>(handle)...);
            }
        }
    }

    template<typename... Components, typename Func>
    void for_each(Func&& func) const
    {
        auto view = registry_.view<Components...>();
        for (auto handle : view)
        {
            auto it = entity_to_id_.find(handle);
            if (it != entity_to_id_.end())
            {
                func(it->second, view.template get<Components>(handle)...);
            }
        }
    }

private:
    friend class Entity;

    [[nodiscard]] EntityId generate_id();
    [[nodiscard]] entt::entity handle_for(EntityId id) const noexcept;

    std::string name_;
    entt::registry registry_;
    std::unordered_map<EntityId, entt::entity, EntityIdHash> entities_;
    std::unordered_map<entt::entity, EntityId> entity_to_id_;
    std::vector<EntityId> entity_order_;
    std::mt19937_64 id_generator_;
    bool dirty_ = false;
    std::uint64_t revision_ = 0;
};

// Inline implementations of Entity template methods delegating to Scene Facade
template<typename T, typename... Args>
inline T& Entity::add(Args&&... args)
{
    if (!is_valid())
    {
        throw std::logic_error("Cannot add a component to an invalid entity");
    }
    return scene_->add_component<T>(id_, std::forward<Args>(args)...);
}

template<typename T>
inline bool Entity::has() const noexcept
{
    return is_valid() && scene_->has_component<T>(id_);
}

template<typename T>
inline T* Entity::try_get() noexcept
{
    return is_valid() ? scene_->try_get_component<T>(id_) : nullptr;
}

template<typename T>
inline const T* Entity::try_get() const noexcept
{
    return is_valid() ? scene_->try_get_component<T>(id_) : nullptr;
}

template<typename T>
inline T& Entity::get()
{
    if (!is_valid())
    {
        throw std::logic_error("Entity does not have the requested component");
    }
    return scene_->get_component<T>(id_);
}

template<typename T>
inline const T& Entity::get() const
{
    if (!is_valid())
    {
        throw std::logic_error("Entity does not have the requested component");
    }
    return scene_->get_component<T>(id_);
}

template<typename T>
inline bool Entity::remove()
{
    return is_valid() && scene_->remove_component<T>(id_);
}

}

namespace slrye
{
using engine::Scene;
}
