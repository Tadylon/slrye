#include <views/camera_controller.h>

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace slrye
{

bool CameraController::update(
    PerspectiveCamera& camera,
    const CameraControllerInput& input) const noexcept
{
    bool changed = false;

    if (input.rotate &&
        (input.mouse_delta_x != 0.0f || input.mouse_delta_y != 0.0f))
    {
        camera.set_orientation(
            camera.yaw_degrees() + input.mouse_delta_x * rotation_sensitivity,
            camera.pitch_degrees() - input.mouse_delta_y * rotation_sensitivity);
        changed = true;
    }

    glm::vec3 movement = movement_direction(camera, input);
    const float movement_length = glm::length(movement);
    if (movement_length > 0.0f)
    {
        movement /= movement_length;
        const float delta_seconds = std::clamp(input.delta_seconds, 0.0f, 0.1f);
        const float speed = move_speed * (input.boost ? boost_multiplier : 1.0f);
        camera.translate(movement * speed * delta_seconds);
        changed = true;
    }

    if (input.rotate && input.wheel_delta != 0.0f)
    {
        const float wheel = std::clamp(input.wheel_delta, -10.0f, 10.0f);
        camera.translate(camera.forward() * wheel * zoom_speed);
        changed = true;
    }

    return changed;
}

void CameraController::focus(
    PerspectiveCamera& camera,
    const glm::vec3& target) const noexcept
{
    camera.set_position(target - camera.forward() * focus_distance);
}

glm::vec3 CameraController::movement_direction(
    const PerspectiveCamera& camera,
    const CameraControllerInput& input) const noexcept
{
    glm::vec3 movement{0.0f};
    if (input.move_forward)
    {
        movement += camera.forward();
    }
    if (input.move_backward)
    {
        movement -= camera.forward();
    }
    if (input.move_right)
    {
        movement += camera.right();
    }
    if (input.move_left)
    {
        movement -= camera.right();
    }
    if (input.move_up)
    {
        movement += glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (input.move_down)
    {
        movement -= glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return movement;
}

}
