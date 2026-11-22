#include <editor/panels/scene_hierarchy_panel.h>

#include <cstdint>
#include <vector>

#include <imgui.h>

#include <engine/scene/components.h>
#include <engine/scene/entity.h>
#include <engine/scene/scene.h>

namespace slrye
{

void SceneHierarchyPanel::set_scene(Scene* scene) noexcept
{
    scene_ = scene;
    selected_entity_.reset();
}

void SceneHierarchyPanel::set_selected_entity(std::optional<EntityId> entity) noexcept
{
    selected_entity_ = entity;
}

std::optional<EntityId> SceneHierarchyPanel::selected_entity() const noexcept
{
    return selected_entity_;
}

void SceneHierarchyPanel::set_visible(bool visible) noexcept
{
    visible_ = visible;
}

bool SceneHierarchyPanel::is_visible() const noexcept
{
    return visible_;
}

void SceneHierarchyPanel::render()
{
    if (!visible_)
    {
        return;
    }

    if (!ImGui::Begin("Scene Hierarchy", &visible_))
    {
        ImGui::End();
        return;
    }

    if (scene_ == nullptr)
    {
        ImGui::TextDisabled("No scene loaded.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("+ Create Entity"))
    {
        Entity created = scene_->create_entity("New Entity");
        created.add<TransformComponent>();
        selected_entity_ = created.id();
    }

    ImGui::Separator();

    for (EntityId root_id : scene_->root_entities())
    {
        draw_entity_node(root_id);
    }

    // 点击空白区域取消选中
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
    {
        selected_entity_ = std::nullopt;
    }

    draw_context_menu();

    ImGui::End();
}

void SceneHierarchyPanel::draw_entity_node(EntityId id)
{
    if (scene_ == nullptr)
    {
        return;
    }

    Entity entity = scene_->find(id);
    if (!entity.is_valid())
    {
        return;
    }

    const std::string name = entity.name();
    const std::vector<EntityId> children = scene_->children(id);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selected_entity_.has_value() && *selected_entity_ == id)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const bool opened = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(id.value)),
        flags,
        "%s",
        name.c_str());

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        selected_entity_ = id;
    }

    // 拖拽成父子层级关系
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &id, sizeof(EntityId));
        ImGui::Text("Move %s", name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
        {
            const EntityId dropped_id = *static_cast<const EntityId*>(payload->Data);
            if (dropped_id != id)
            {
                scene_->set_parent(dropped_id, id);
            }
        }
        ImGui::EndDragDropTarget();
    }

    bool entity_deleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Add Child Entity"))
        {
            Entity child = scene_->create_entity("Child Entity");
            child.add<TransformComponent>();
            scene_->set_parent(child.id(), id);
        }
        if (ImGui::MenuItem("Clear Parent"))
        {
            scene_->clear_parent(id);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete Entity"))
        {
            entity_deleted = true;
        }
        ImGui::EndPopup();
    }

    if (opened)
    {
        for (EntityId child_id : children)
        {
            draw_entity_node(child_id);
        }
        ImGui::TreePop();
    }

    if (entity_deleted)
    {
        scene_->destroy_entity(id);
        if (selected_entity_.has_value() && *selected_entity_ == id)
        {
            selected_entity_ = std::nullopt;
        }
    }
}

void SceneHierarchyPanel::draw_context_menu()
{
    if (ImGui::BeginPopupContextWindow(
            "HierarchyContextMenu",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            Entity entity = scene_->create_entity("Entity");
            entity.add<TransformComponent>();
            selected_entity_ = entity.id();
        }
        if (ImGui::MenuItem("Create Light"))
        {
            Entity entity = scene_->create_entity("Point Light");
            entity.add<TransformComponent>(glm::vec3(0.0f, 3.0f, 0.0f));
            entity.add<LightComponent>();
            selected_entity_ = entity.id();
        }
        if (ImGui::MenuItem("Create Cube (Collider)"))
        {
            Entity entity = scene_->create_entity("Cube");
            entity.add<TransformComponent>();
            entity.add<ColliderComponent>();
            selected_entity_ = entity.id();
        }
        ImGui::EndPopup();
    }
}

}
