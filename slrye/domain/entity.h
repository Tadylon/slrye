#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <entt/entity/registry.hpp>

#include <domain/entity_id.h>

namespace slrye
{

class Scene;

class Entity
{
public:
    Entity() = default;

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] EntityId id() const noexcept;
    [[nodiscard]] std::string name() const;
    void set_name(std::string name);

    [[nodiscard]] std::optional<EntityId> parent() const;
    [[nodiscard]] std::vector<EntityId> children() const;
    bool set_parent(Entity parent);
    bool clear_parent();
    bool destroy();
    void mark_dirty();

    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
        if (!is_valid())
        {
            throw std::logic_error("Cannot add a component to an invalid entity");
        }
        if (registry_->all_of<T>(handle_))
        {
            throw std::logic_error("Entity already has the requested component");
        }

        T& component = registry_->emplace<T>(handle_, std::forward<Args>(args)...);
        mark_dirty();
        return component;
    }

    template<typename T>
    [[nodiscard]] bool has() const noexcept
    {
        return is_valid() && registry_->all_of<T>(handle_);
    }

    template<typename T>
    [[nodiscard]] T* try_get() noexcept
    {
        return has<T>() ? registry_->try_get<T>(handle_) : nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* try_get() const noexcept
    {
        return has<T>() ? registry_->try_get<T>(handle_) : nullptr;
    }

    template<typename T>
    T& get()
    {
        T* component = try_get<T>();
        if (component == nullptr)
        {
            throw std::out_of_range("Entity does not have the requested component");
        }
        return *component;
    }

    template<typename T>
    const T& get() const
    {
        const T* component = try_get<T>();
        if (component == nullptr)
        {
            throw std::out_of_range("Entity does not have the requested component");
        }
        return *component;
    }

    template<typename T>
    bool remove()
    {
        if (!has<T>())
        {
            return false;
        }

        registry_->remove<T>(handle_);
        mark_dirty();
        return true;
    }

private:
    friend class Scene;

    Entity(
        Scene* scene,
        entt::registry* registry,
        entt::entity handle,
        EntityId id) noexcept
        : scene_(scene),
          registry_(registry),
          handle_(handle),
          id_(id)
    {
    }

    Scene* scene_ = nullptr;
    entt::registry* registry_ = nullptr;
    entt::entity handle_ = entt::null;
    EntityId id_;
};

}
