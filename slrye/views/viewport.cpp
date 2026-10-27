#include <views/viewport.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/vec4.hpp>

#include <domain/scene.h>
#include <domain/scene_transform.h>
#include <services/picking/picking.h>

namespace slrye
{

bool Viewport::initialize(
    const VulkanContext& context,
    const std::filesystem::path& shader_directory,
    Scene& scene,
    std::string* error)
{
    scene_ = &scene;
    if (!renderer_.initialize(context, shader_directory, error))
    {
        initialized_ = false;
        return false;
    }

    initialized_ = true;
    camera_dirty_ = true;
    return true;
}

void Viewport::render(ImGuiIO& io)
{
    if (!visible_)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (!ImGui::Begin("Viewport", &visible_))
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::PopStyleVar();

    draw_toolbar();

    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (available.x <= 1.0f || available.y <= 1.0f || scene_ == nullptr)
    {
        ImGui::TextDisabled("No scene is available.");
        ImGui::End();
        return;
    }

    const ImVec2 scale = io.DisplayFramebufferScale;
    const std::uint32_t width = static_cast<std::uint32_t>(
        std::clamp(available.x * std::max(scale.x, 1.0f), 1.0f, 8192.0f));
    const std::uint32_t height = static_cast<std::uint32_t>(
        std::clamp(available.y * std::max(scale.y, 1.0f), 1.0f, 8192.0f));
    const float aspect_ratio = available.x / available.y;
    if (std::abs(camera_.aspect_ratio() - aspect_ratio) > 0.0001f)
    {
        camera_.set_aspect_ratio(aspect_ratio);
        camera_dirty_ = true;
    }

    const bool scene_changed = scene_->revision() != rendered_scene_revision_;
    const bool size_changed = width != rendered_width_ || height != rendered_height_;
    if (initialized_ && (camera_dirty_ || scene_changed || size_changed))
    {
        std::string error;
        if (renderer_.render(
                *scene_,
                camera_,
                width,
                height,
                selected_entity_,
                show_grid_,
                &error))
        {
            rendered_scene_revision_ = scene_->revision();
            rendered_width_ = width;
            rendered_height_ = height;
            camera_dirty_ = false;
            last_error_.clear();
        }
        else
        {
            last_error_ = std::move(error);
        }
    }

    const ImVec2 image_position = ImGui::GetCursorScreenPos();
    if (initialized_ && renderer_.texture_id() != ImTextureID_Invalid)
    {
        ImGui::Image(
            ImTextureRef(renderer_.texture_id()),
            available,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f));
        process_input(io, image_position, available);
    }
    else
    {
        ImGui::Dummy(available);
        ImGui::SetCursorScreenPos(ImVec2(image_position.x + 12.0f, image_position.y + 12.0f));
        ImGui::TextUnformatted(
            last_error_.empty() ? "Vulkan viewport is unavailable." : last_error_.c_str());
    }

    ImGui::End();
}

void Viewport::shutdown()
{
    renderer_.shutdown();
    initialized_ = false;
    rendered_width_ = 0;
    rendered_height_ = 0;
}

void Viewport::set_scene(Scene& scene) noexcept
{
    scene_ = &scene;
    selected_entity_.reset();
    request_redraw();
}

void Viewport::set_camera_state(
    const glm::vec3& position,
    float yaw_degrees,
    float pitch_degrees) noexcept
{
    camera_.set_position(position);
    camera_.set_orientation(yaw_degrees, pitch_degrees);
    request_redraw();
}

void Viewport::set_selected_entity(std::optional<EntityId> entity) noexcept
{
    if (selected_entity_ == entity)
    {
        return;
    }
    selected_entity_ = entity;
    request_redraw();
}

void Viewport::set_visible(bool visible) noexcept
{
    visible_ = visible;
}

void Viewport::set_camera_speed(float speed) noexcept
{
    controller_.move_speed = std::max(speed, 0.1f);
}

void Viewport::set_grid_visible(bool visible) noexcept
{
    if (show_grid_ == visible)
    {
        return;
    }
    show_grid_ = visible;
    request_redraw();
}

PerspectiveCamera& Viewport::camera() noexcept
{
    return camera_;
}

const PerspectiveCamera& Viewport::camera() const noexcept
{
    return camera_;
}

std::optional<EntityId> Viewport::selected_entity() const noexcept
{
    return selected_entity_;
}

bool Viewport::is_initialized() const noexcept
{
    return initialized_;
}

bool Viewport::is_visible() const noexcept
{
    return visible_;
}

float Viewport::camera_speed() const noexcept
{
    return controller_.move_speed;
}

bool Viewport::is_grid_visible() const noexcept
{
    return show_grid_;
}

void Viewport::draw_toolbar()
{
    ImGui::SetCursorPos(ImVec2(8.0f, 8.0f));
    ImGui::TextDisabled("Perspective");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::DragFloat(
            "Speed",
            &controller_.move_speed,
            0.1f,
            0.1f,
            100.0f,
            "%.1f"))
    {
        controller_.move_speed = std::max(controller_.move_speed, 0.1f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Focus") && selected_entity_.has_value())
    {
        focus_selected();
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Grid", &show_grid_))
    {
        request_redraw();
    }
    ImGui::SameLine();
    const glm::vec3 position = camera_.position();
    ImGui::TextDisabled("Camera %.1f, %.1f, %.1f", position.x, position.y, position.z);
}

void Viewport::process_input(
    ImGuiIO& io,
    const ImVec2& image_position,
    const ImVec2& image_size)
{
    const bool hovered = ImGui::IsItemHovered();
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rotating_ = true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        rotating_ = false;
    }

    CameraControllerInput input;
    input.rotate = rotating_;
    input.boost = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    input.mouse_delta_x = rotating_ ? io.MouseDelta.x : 0.0f;
    input.mouse_delta_y = rotating_ ? io.MouseDelta.y : 0.0f;
    input.wheel_delta = hovered ? io.MouseWheel : 0.0f;
    input.delta_seconds = io.DeltaTime;
    if (rotating_)
    {
        input.move_forward = ImGui::IsKeyDown(ImGuiKey_W);
        input.move_backward = ImGui::IsKeyDown(ImGuiKey_S);
        input.move_left = ImGui::IsKeyDown(ImGuiKey_A);
        input.move_right = ImGui::IsKeyDown(ImGuiKey_D);
        input.move_up = ImGui::IsKeyDown(ImGuiKey_E);
        input.move_down = ImGui::IsKeyDown(ImGuiKey_Q);
    }
    if (controller_.update(camera_, input))
    {
        camera_dirty_ = true;
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const Ray ray = camera_.ray_from_viewport(
            io.MousePos.x - image_position.x,
            io.MousePos.y - image_position.y,
            image_size.x,
            image_size.y);
        const std::optional<PickResult> result = pick_entity(*scene_, ray);
        set_selected_entity(result.has_value()
            ? std::optional<EntityId>(result->entity)
            : std::nullopt);
    }
}

void Viewport::focus_selected()
{
    if (scene_ == nullptr || !selected_entity_.has_value())
    {
        return;
    }

    glm::mat4 world;
    if (!world_transform_matrix(*scene_, *selected_entity_, world))
    {
        return;
    }

    const glm::vec3 target(world[3]);
    controller_.focus(camera_, target);
    camera_dirty_ = true;
}

void Viewport::request_redraw() noexcept
{
    camera_dirty_ = true;
}

}
