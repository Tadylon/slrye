#include <views/inspector_panel.h>

#include <cstring>

#include <imgui.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <domain/components.h>
#include <domain/entity.h>
#include <domain/scene.h>

namespace slrye
{

void InspectorPanel::set_scene(Scene* scene) noexcept
{
    scene_ = scene;
    selected_entity_.reset();
}

void InspectorPanel::set_selected_entity(std::optional<EntityId> entity) noexcept
{
    selected_entity_ = entity;
}

std::optional<EntityId> InspectorPanel::selected_entity() const noexcept
{
    return selected_entity_;
}

void InspectorPanel::set_visible(bool visible) noexcept
{
    visible_ = visible;
}

bool InspectorPanel::is_visible() const noexcept
{
    return visible_;
}

void InspectorPanel::render()
{
    if (!visible_)
    {
        return;
    }

    if (!ImGui::Begin("Inspector", &visible_))
    {
        ImGui::End();
        return;
    }

    if (scene_ == nullptr || !selected_entity_.has_value())
    {
        ImGui::TextDisabled("No entity selected.");
        ImGui::End();
        return;
    }

    Entity entity = scene_->find(*selected_entity_);
    if (!entity.is_valid())
    {
        ImGui::TextDisabled("Selected entity is no longer valid.");
        ImGui::End();
        return;
    }

    draw_components(entity);

    ImGui::End();
}

void InspectorPanel::draw_components(Entity entity)
{
    // 1. 实体名称
    char name_buffer[256];
    std::string current_name = entity.name();
    std::strncpy(name_buffer, current_name.c_str(), sizeof(name_buffer) - 1);
    name_buffer[sizeof(name_buffer) - 1] = '\0';

    if (ImGui::InputText("##EntityName", name_buffer, sizeof(name_buffer)))
    {
        entity.set_name(name_buffer);
        scene_->mark_dirty();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("ID: %llu", static_cast<unsigned long long>(entity.id().value));
    ImGui::Separator();

    // 2. Transform 组件
    if (entity.has<TransformComponent>())
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& tc = entity.get<TransformComponent>();
            bool modified = false;

            modified |= draw_vec3_control("Position", tc.position, 0.0f);

            glm::vec3 euler = glm::degrees(glm::eulerAngles(tc.rotation));
            if (draw_vec3_control("Rotation", euler, 0.0f))
            {
                tc.rotation = glm::quat(glm::radians(euler));
                modified = true;
            }

            modified |= draw_vec3_control("Scale", tc.scale, 1.0f);

            if (modified)
            {
                scene_->mark_dirty();
            }
        }
    }

    // 3. Light 组件
    if (entity.has<LightComponent>())
    {
        bool remove_light = false;
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& light = entity.get<LightComponent>();
            bool modified = false;

            const char* light_types[] = {"Directional", "Point", "Spot"};
            int current_type = static_cast<int>(light.type);
            if (ImGui::Combo("Type", &current_type, light_types, 3))
            {
                light.type = static_cast<LightType>(current_type);
                modified = true;
            }

            modified |= ImGui::ColorEdit3("Color", &light.color.x);
            modified |= ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
            modified |= ImGui::DragFloat("Range", &light.range, 0.5f, 0.1f, 1000.0f);

            if (light.type == LightType::Spot)
            {
                modified |= ImGui::DragFloat("Inner Cone", &light.inner_cone_degrees, 0.5f, 0.0f, 90.0f);
                modified |= ImGui::DragFloat("Outer Cone", &light.outer_cone_degrees, 0.5f, 0.0f, 90.0f);
            }

            if (ImGui::Button("Remove Light"))
            {
                remove_light = true;
            }

            if (modified)
            {
                scene_->mark_dirty();
            }
        }
        if (remove_light)
        {
            entity.remove<LightComponent>();
            scene_->mark_dirty();
        }
    }

    // 4. Camera 组件
    if (entity.has<CameraComponent>())
    {
        bool remove_camera = false;
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& camera = entity.get<CameraComponent>();
            bool modified = false;

            modified |= ImGui::DragFloat("Vertical FOV", &camera.vertical_fov_degrees, 0.5f, 1.0f, 179.0f);
            modified |= ImGui::DragFloat("Near Clip", &camera.near_clip, 0.01f, 0.001f, 100.0f);
            modified |= ImGui::DragFloat("Far Clip", &camera.far_clip, 1.0f, 1.0f, 10000.0f);
            modified |= ImGui::Checkbox("Primary", &camera.primary);

            if (ImGui::Button("Remove Camera"))
            {
                remove_camera = true;
            }

            if (modified)
            {
                scene_->mark_dirty();
            }
        }
        if (remove_camera)
        {
            entity.remove<CameraComponent>();
            scene_->mark_dirty();
        }
    }

    // 5. Collider 组件
    if (entity.has<ColliderComponent>())
    {
        bool remove_collider = false;
        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& collider = entity.get<ColliderComponent>();
            bool modified = false;

            const char* collider_types[] = {"Box", "Sphere", "Capsule"};
            int current_type = static_cast<int>(collider.type);
            if (ImGui::Combo("Collider Type", &current_type, collider_types, 3))
            {
                collider.type = static_cast<ColliderType>(current_type);
                modified = true;
            }

            if (collider.type == ColliderType::Box)
            {
                modified |= draw_vec3_control("Half Extents", collider.half_extents, 0.5f);
            }
            else if (collider.type == ColliderType::Sphere)
            {
                modified |= ImGui::DragFloat("Radius", &collider.radius, 0.05f, 0.01f, 100.0f);
            }
            else if (collider.type == ColliderType::Capsule)
            {
                modified |= ImGui::DragFloat("Radius", &collider.radius, 0.05f, 0.01f, 100.0f);
                modified |= ImGui::DragFloat("Height", &collider.height, 0.05f, 0.01f, 100.0f);
            }

            modified |= ImGui::Checkbox("Enabled", &collider.enabled);

            if (ImGui::Button("Remove Collider"))
            {
                remove_collider = true;
            }

            if (modified)
            {
                scene_->mark_dirty();
            }
        }
        if (remove_collider)
        {
            entity.remove<ColliderComponent>();
            scene_->mark_dirty();
        }
    }

    // 6. 添加组件弹窗
    ImGui::Separator();
    if (ImGui::Button("+ Add Component"))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (!entity.has<TransformComponent>() && ImGui::MenuItem("Transform"))
        {
            entity.add<TransformComponent>();
            scene_->mark_dirty();
            ImGui::CloseCurrentPopup();
        }
        if (!entity.has<LightComponent>() && ImGui::MenuItem("Light"))
        {
            entity.add<LightComponent>();
            scene_->mark_dirty();
            ImGui::CloseCurrentPopup();
        }
        if (!entity.has<CameraComponent>() && ImGui::MenuItem("Camera"))
        {
            entity.add<CameraComponent>();
            scene_->mark_dirty();
            ImGui::CloseCurrentPopup();
        }
        if (!entity.has<ColliderComponent>() && ImGui::MenuItem("Collider"))
        {
            entity.add<ColliderComponent>();
            scene_->mark_dirty();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

bool InspectorPanel::draw_vec3_control(const char* label, glm::vec3& values, float reset_value)
{
    bool changed = false;
    ImGui::PushID(label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 90.0f);
    ImGui::TextUnformatted(label);
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.15f, 0.15f, 1.0f});
    if (ImGui::Button("X"))
    {
        values.x = reset_value;
        changed = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    changed |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y"))
    {
        values.y = reset_value;
        changed = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    changed |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.15f, 0.35f, 0.85f, 1.0f});
    if (ImGui::Button("Z"))
    {
        values.z = reset_value;
        changed = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    changed |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::Columns(1);
    ImGui::PopID();
    return changed;
}

}
