#include <views/camera_controller.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Camera controller rotates and moves the editor camera")
{
    slrye::PerspectiveCamera camera;
    slrye::CameraController controller;
    camera.set_orientation(0.0f, 0.0f);
    camera.set_position(glm::vec3(0.0f));
    controller.rotation_sensitivity = 0.5f;
    controller.move_speed = 10.0f;

    slrye::CameraControllerInput input;
    input.rotate = true;
    input.mouse_delta_x = 20.0f;
    input.mouse_delta_y = -10.0f;
    input.move_forward = true;
    input.delta_seconds = 0.05f;

    CHECK(controller.update(camera, input));
    CHECK(camera.yaw_degrees() == Catch::Approx(10.0f));
    CHECK(camera.pitch_degrees() == Catch::Approx(5.0f));
    CHECK(camera.position().z < -0.4f);
}

TEST_CASE("Camera focus keeps the current view direction")
{
    slrye::PerspectiveCamera camera;
    slrye::CameraController controller;
    camera.set_orientation(90.0f, 0.0f);
    controller.focus_distance = 4.0f;

    controller.focus(camera, glm::vec3(2.0f, 3.0f, 4.0f));

    CHECK(camera.position().x == Catch::Approx(-2.0f).margin(0.001f));
    CHECK(camera.position().y == Catch::Approx(3.0f).margin(0.001f));
    CHECK(camera.position().z == Catch::Approx(4.0f).margin(0.001f));
}
