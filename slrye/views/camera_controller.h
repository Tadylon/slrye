#pragma once

#include <glm/vec3.hpp>

#include <domain/camera.h>

namespace slrye
{

struct CameraControllerInput
{
    bool rotate = false;
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    bool boost = false;
    float mouse_delta_x = 0.0f;
    float mouse_delta_y = 0.0f;
    float wheel_delta = 0.0f;
    float delta_seconds = 0.0f;
};

class CameraController
{
public:
    [[nodiscard]] bool update(
        PerspectiveCamera& camera,
        const CameraControllerInput& input) const noexcept;

    void focus(PerspectiveCamera& camera, const glm::vec3& target) const noexcept;

    float move_speed = 6.0f;
    float rotation_sensitivity = 0.15f;
    float zoom_speed = 0.75f;
    float boost_multiplier = 3.0f;
    float focus_distance = 5.0f;

private:
    [[nodiscard]] glm::vec3 movement_direction(
        const PerspectiveCamera& camera,
        const CameraControllerInput& input) const noexcept;
};

}
