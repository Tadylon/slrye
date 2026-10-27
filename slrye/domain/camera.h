#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace slrye
{

struct Ray
{
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
};

class PerspectiveCamera
{
public:
    PerspectiveCamera() = default;

    [[nodiscard]] const glm::vec3& position() const noexcept;
    void set_position(const glm::vec3& position) noexcept;
    void translate(const glm::vec3& offset) noexcept;

    [[nodiscard]] float yaw_degrees() const noexcept;
    [[nodiscard]] float pitch_degrees() const noexcept;
    void set_orientation(float yaw_degrees, float pitch_degrees) noexcept;

    [[nodiscard]] float vertical_fov_degrees() const noexcept;
    void set_vertical_fov_degrees(float fov_degrees) noexcept;
    [[nodiscard]] float aspect_ratio() const noexcept;
    void set_aspect_ratio(float aspect_ratio) noexcept;
    [[nodiscard]] float near_clip() const noexcept;
    [[nodiscard]] float far_clip() const noexcept;
    void set_clip_planes(float near_clip, float far_clip) noexcept;

    [[nodiscard]] glm::vec3 forward() const noexcept;
    [[nodiscard]] glm::vec3 right() const noexcept;
    [[nodiscard]] glm::vec3 up() const noexcept;
    [[nodiscard]] glm::mat4 view_matrix() const noexcept;
    [[nodiscard]] glm::mat4 projection_matrix() const noexcept;
    [[nodiscard]] glm::mat4 view_projection_matrix() const noexcept;

    [[nodiscard]] Ray ray_from_ndc(float x, float y) const noexcept;
    [[nodiscard]] Ray ray_from_viewport(
        float x,
        float y,
        float width,
        float height) const noexcept;

private:
    glm::vec3 position_{0.0f, 2.0f, 6.0f};
    float yaw_degrees_ = 0.0f;
    float pitch_degrees_ = -10.0f;
    float vertical_fov_degrees_ = 60.0f;
    float aspect_ratio_ = 16.0f / 9.0f;
    float near_clip_ = 0.1f;
    float far_clip_ = 1000.0f;
};

}
