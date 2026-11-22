#pragma once

#include <optional>
#include <string>
#include <vector>

#include <engine/scene/entity_id.h>

namespace slrye::engine
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
    T& add(Args&&... args);

    template<typename T>
    [[nodiscard]] bool has() const noexcept;

    template<typename T>
    [[nodiscard]] T* try_get() noexcept;

    template<typename T>
    [[nodiscard]] const T* try_get() const noexcept;

    template<typename T>
    T& get();

    template<typename T>
    const T& get() const;

    template<typename T>
    bool remove();

    bool operator==(const Entity& other) const noexcept
    {
        return scene_ == other.scene_ && id_ == other.id_;
    }

    bool operator!=(const Entity& other) const noexcept
    {
        return !(*this == other);
    }

private:
    friend class Scene;

    Entity(Scene* scene, EntityId id) noexcept
        : scene_(scene),
          id_(id)
    {
    }

    Scene* scene_ = nullptr;
    EntityId id_;
};

}

namespace slrye
{
using engine::Entity;
}
