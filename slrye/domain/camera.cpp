#include <domain/camera.h>

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace slrye
{
namespace
{

constexpr float kMinimumAspectRatio = 0.0001f;
constexpr float kMaximumPitchDegrees = 89.0f;

}

const glm::vec3& PerspectiveCamera::position() const noexcept
{
    return position_;
}

void PerspectiveCamera::set_position(const glm::vec3& position) noexcept
{
    position_ = position;
}

void PerspectiveCamera::translate(const glm::vec3& offset) noexcept
{
    position_ += offset;
}

float PerspectiveCamera::yaw_degrees() const noexcept
{
    return yaw_degrees_;
}

float PerspectiveCamera::pitch_degrees() const noexcept
{
    return pitch_degrees_;
}

void PerspectiveCamera::set_orientation(float yaw_degrees, float pitch_degrees) noexcept
{
    yaw_degrees_ = std::remainder(yaw_degrees, 360.0f);
    pitch_degrees_ = std::clamp(
        pitch_degrees,
        -kMaximumPitchDegrees,
        kMaximumPitchDegrees);
}

float PerspectiveCamera::vertical_fov_degrees() const noexcept
{
    return vertical_fov_degrees_;
}

void PerspectiveCamera::set_vertical_fov_degrees(float fov_degrees) noexcept
{
    vertical_fov_degrees_ = std::clamp(fov_degrees, 10.0f, 120.0f);
}

float PerspectiveCamera::aspect_ratio() const noexcept
{
    return aspect_ratio_;
}

void PerspectiveCamera::set_aspect_ratio(float aspect_ratio) noexcept
{
    aspect_ratio_ = std::max(aspect_ratio, kMinimumAspectRatio);
}

float PerspectiveCamera::near_clip() const noexcept
{
    return near_clip_;
}

float PerspectiveCamera::far_clip() const noexcept
{
    return far_clip_;
}

void PerspectiveCamera::set_clip_planes(float near_clip, float far_clip) noexcept
{
    near_clip_ = std::max(near_clip, 0.0001f);
    far_clip_ = std::max(far_clip, near_clip_ + 0.0001f);
}

glm::vec3 PerspectiveCamera::forward() const noexcept
{
    const float yaw = glm::radians(yaw_degrees_);
    const float pitch = glm::radians(pitch_degrees_);
    const float horizontal = std::cos(pitch);
    return glm::normalize(glm::vec3(
        horizontal * std::sin(yaw),
        std::sin(pitch),
        -horizontal * std::cos(yaw)));
}

glm::vec3 PerspectiveCamera::right() const noexcept
{
    return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 PerspectiveCamera::up() const noexcept
{
    return glm::normalize(glm::cross(right(), forward()));
}

glm::mat4 PerspectiveCamera::view_matrix() const noexcept
{
    return glm::lookAt(position_, position_ + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 PerspectiveCamera::projection_matrix() const noexcept
{
    glm::mat4 projection = glm::perspectiveRH_ZO(
        glm::radians(vertical_fov_degrees_),
        aspect_ratio_,
        near_clip_,
        far_clip_);
    projection[1][1] *= -1.0f;
    return projection;
}

glm::mat4 PerspectiveCamera::view_projection_matrix() const noexcept
{
    return projection_matrix() * view_matrix();
}

Ray PerspectiveCamera::ray_from_ndc(float x, float y) const noexcept
{
    const float tangent = std::tan(glm::radians(vertical_fov_degrees_) * 0.5f);
    const glm::vec3 direction = glm::normalize(
        forward() +
        right() * (x * aspect_ratio_ * tangent) +
        up() * (y * tangent));
    return Ray{position_, direction};
}

Ray PerspectiveCamera::ray_from_viewport(
    float x,
    float y,
    float width,
    float height) const noexcept
{
    if (width <= 0.0f || height <= 0.0f)
    {
        return Ray{position_, forward()};
    }

    const float ndc_x = (x / width) * 2.0f - 1.0f;
    const float ndc_y = 1.0f - (y / height) * 2.0f;
    return ray_from_ndc(ndc_x, ndc_y);
}

}
