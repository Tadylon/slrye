#include <domain/scene.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace slrye
{

bool Entity::is_valid() const noexcept
{
    return scene_ != nullptr && registry_ != nullptr && scene_->contains(id_);
}

EntityId Entity::id() const noexcept
{
    return id_;
}

std::string Entity::name() const
{
    return is_valid() ? registry_->get<NameComponent>(handle_).value : std::string{};
}

void Entity::set_name(std::string name)
{
    if (!is_valid())
    {
        throw std::logic_error("Cannot rename an invalid entity");
    }

    registry_->get<NameComponent>(handle_).value = std::move(name);
    mark_dirty();
}

std::optional<EntityId> Entity::parent() const
{
    return is_valid() ? scene_->parent(id_) : std::nullopt;
}

std::vector<EntityId> Entity::children() const
{
    return is_valid() ? scene_->children(id_) : std::vector<EntityId>{};
}

bool Entity::set_parent(Entity parent_entity)
{
    return is_valid() && parent_entity.is_valid() && scene_->set_parent(id_, parent_entity.id_);
}

bool Entity::clear_parent()
{
    return is_valid() && scene_->clear_parent(id_);
}

bool Entity::destroy()
{
    return is_valid() && scene_->destroy_entity(id_);
}

void Entity::mark_dirty()
{
    if (is_valid())
    {
        scene_->mark_dirty();
    }
}

Scene::Scene(std::string name)
    : name_(std::move(name)),
      id_generator_(
          static_cast<std::uint64_t>(std::random_device{}()) ^
          static_cast<std::uint64_t>(
              std::chrono::steady_clock::now().time_since_epoch().count()))
{
}

const std::string& Scene::name() const noexcept
{
    return name_;
}

void Scene::set_name(std::string name)
{
    if (name_ == name)
    {
        return;
    }

    name_ = std::move(name);
    mark_dirty();
}

Entity Scene::create_entity(std::string name)
{
    return create_entity(generate_id(), std::move(name));
}

Entity Scene::create_entity(EntityId id, std::string name)
{
    if (!id.is_valid() || contains(id))
    {
        return {};
    }

    const entt::entity handle = registry_.create();
    registry_.emplace<NameComponent>(handle, NameComponent{std::move(name)});
    registry_.emplace<TransformComponent>(handle);
    registry_.emplace<HierarchyComponent>(handle);

    entities_.emplace(id, handle);
    entity_order_.push_back(id);
    mark_dirty();
    return Entity(this, &registry_, handle, id);
}

bool Scene::destroy_entity(EntityId id)
{
    Entity entity = find(id);
    if (!entity.is_valid())
    {
        return false;
    }

    const std::vector<EntityId> child_ids = children(id);
    for (EntityId child : child_ids)
    {
        destroy_entity(child);
    }

    const std::optional<EntityId> parent_id = parent(id);
    if (parent_id.has_value())
    {
        const entt::entity parent_handle = handle_for(*parent_id);
        if (parent_handle != entt::null)
        {
            HierarchyComponent& parent_hierarchy = registry_.get<HierarchyComponent>(parent_handle);
            std::erase(parent_hierarchy.children, id);
        }
    }

    registry_.destroy(handle_for(id));
    entities_.erase(id);
    std::erase(entity_order_, id);
    mark_dirty();
    return true;
}

Entity Scene::find(EntityId id)
{
    const entt::entity handle = handle_for(id);
    if (handle == entt::null)
    {
        return {};
    }

    return Entity(this, &registry_, handle, id);
}

bool Scene::contains(EntityId id) const noexcept
{
    if (!id.is_valid())
    {
        return false;
    }

    const auto iterator = entities_.find(id);
    return iterator != entities_.end() && registry_.valid(iterator->second);
}

std::size_t Scene::entity_count() const noexcept
{
    return entities_.size();
}

const std::vector<EntityId>& Scene::entities() const noexcept
{
    return entity_order_;
}

std::vector<EntityId> Scene::root_entities() const
{
    std::vector<EntityId> roots;
    roots.reserve(entity_order_.size());
    for (EntityId id : entity_order_)
    {
        if (!parent(id).has_value())
        {
            roots.push_back(id);
        }
    }
    return roots;
}

bool Scene::set_parent(EntityId child, EntityId parent_id)
{
    const entt::entity child_handle = handle_for(child);
    const entt::entity parent_handle = handle_for(parent_id);
    if (child_handle == entt::null || parent_handle == entt::null || child == parent_id)
    {
        return false;
    }

    std::optional<EntityId> ancestor = parent_id;
    while (ancestor.has_value())
    {
        if (*ancestor == child)
        {
            return false;
        }
        ancestor = parent(*ancestor);
    }

    HierarchyComponent& child_hierarchy = registry_.get<HierarchyComponent>(child_handle);
    if (child_hierarchy.parent == parent_id)
    {
        return true;
    }

    if (child_hierarchy.parent.is_valid())
    {
        const entt::entity old_parent_handle = handle_for(child_hierarchy.parent);
        if (old_parent_handle != entt::null)
        {
            HierarchyComponent& old_parent_hierarchy = registry_.get<HierarchyComponent>(old_parent_handle);
            std::erase(old_parent_hierarchy.children, child);
        }
    }

    child_hierarchy.parent = parent_id;
    HierarchyComponent& parent_hierarchy = registry_.get<HierarchyComponent>(parent_handle);
    parent_hierarchy.children.push_back(child);
    mark_dirty();
    return true;
}

bool Scene::clear_parent(EntityId child)
{
    const entt::entity child_handle = handle_for(child);
    if (child_handle == entt::null)
    {
        return false;
    }

    HierarchyComponent& child_hierarchy = registry_.get<HierarchyComponent>(child_handle);
    if (!child_hierarchy.parent.is_valid())
    {
        return false;
    }

    const entt::entity parent_handle = handle_for(child_hierarchy.parent);
    if (parent_handle != entt::null)
    {
        HierarchyComponent& parent_hierarchy = registry_.get<HierarchyComponent>(parent_handle);
        std::erase(parent_hierarchy.children, child);
    }

    child_hierarchy.parent = EntityId{};
    mark_dirty();
    return true;
}

std::optional<EntityId> Scene::parent(EntityId id) const
{
    const entt::entity handle = handle_for(id);
    if (handle == entt::null)
    {
        return std::nullopt;
    }

    const EntityId parent_id = registry_.get<HierarchyComponent>(handle).parent;
    return parent_id.is_valid() ? std::optional<EntityId>(parent_id) : std::nullopt;
}

std::vector<EntityId> Scene::children(EntityId id) const
{
    const entt::entity handle = handle_for(id);
    if (handle == entt::null)
    {
        return {};
    }

    return registry_.get<HierarchyComponent>(handle).children;
}

bool Scene::is_dirty() const noexcept
{
    return dirty_;
}

void Scene::mark_dirty() noexcept
{
    dirty_ = true;
}

void Scene::clear_dirty() noexcept
{
    dirty_ = false;
}

entt::registry& Scene::registry() noexcept
{
    return registry_;
}

const entt::registry& Scene::registry() const noexcept
{
    return registry_;
}

EntityId Scene::generate_id()
{
    std::uniform_int_distribution<std::uint64_t> distribution(1);
    EntityId id;
    do
    {
        id = EntityId(distribution(id_generator_));
    }
    while (contains(id));
    return id;
}

entt::entity Scene::handle_for(EntityId id) const noexcept
{
    const auto iterator = entities_.find(id);
    if (iterator == entities_.end() || !registry_.valid(iterator->second))
    {
        return entt::null;
    }
    return iterator->second;
}

}
