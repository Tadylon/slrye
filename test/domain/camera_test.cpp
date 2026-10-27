#include <domain/camera.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

TEST_CASE("Perspective camera builds a center ray from its forward direction")
{
    slrye::PerspectiveCamera camera;
    camera.set_position(glm::vec3(0.0f, 2.0f, 6.0f));
    camera.set_orientation(0.0f, 0.0f);
    camera.set_aspect_ratio(16.0f / 9.0f);

    const slrye::Ray ray = camera.ray_from_viewport(400.0f, 300.0f, 800.0f, 600.0f);

    CHECK(ray.origin == camera.position());
    CHECK(ray.direction.x == Catch::Approx(0.0f).margin(0.0001f));
    CHECK(ray.direction.y == Catch::Approx(0.0f).margin(0.0001f));
    CHECK(ray.direction.z == Catch::Approx(-1.0f).margin(0.0001f));
}

TEST_CASE("Perspective camera clamps unsafe projection parameters")
{
    slrye::PerspectiveCamera camera;

    camera.set_orientation(725.0f, 120.0f);
    camera.set_vertical_fov_degrees(1.0f);
    camera.set_clip_planes(0.0f, -5.0f);
    camera.set_aspect_ratio(0.0f);

    CHECK(camera.yaw_degrees() == Catch::Approx(5.0f));
    CHECK(camera.pitch_degrees() == Catch::Approx(89.0f));
    CHECK(camera.vertical_fov_degrees() == Catch::Approx(10.0f));
    CHECK(camera.aspect_ratio() > 0.0f);
    CHECK(camera.far_clip() > camera.near_clip());
    CHECK(std::isfinite(camera.view_projection_matrix()[0][0]));
}
