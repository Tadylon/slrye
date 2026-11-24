#pragma once

#include <glm/vec3.hpp>

#include <engine/renderer/camera.h>

namespace slrye
{

/**
 * @brief 视口相机单帧输入状态采集数据结构
 */
struct ViewportCameraInput
{
    bool rotate = false;
    bool pan = false;
    bool move_forward = false;
    bool move_backward = false;
    bool move_left = false;
    bool move_right = false;
    bool move_up = false;
    bool move_down = false;
    bool boost = false;
    float mouse_delta_x = 0.0f;
    float mouse_delta_y = 0.0f;
    float pan_delta_x = 0.0f;
    float pan_delta_y = 0.0f;
    float wheel_delta = 0.0f;
    float delta_seconds = 0.0f;
};

// 向下兼容别名
using CameraControllerInput = ViewportCameraInput;

/**
 * @brief 3D 视口相机交互操纵控制器（专用于 Viewport 面板的漫游控制算法，非独立 UI 面板）
 * 
 * 职责与层级关系：
 * 1. 位于 Editor Panels 模块，由 Viewport 面板独占持有；
 * 2. 负责响应视口内部的鼠标和键盘操作（鼠标右键旋转环视、中键平移、滚轮缩放、WASD 空间飞行漫游）；
 * 3. 计算并更新 engine::PerspectiveCamera 的空间三维位置与偏航俯仰欧拉角；
 * 4. 提供 focus() 方法，将视口相机精准对焦至目标实体坐标。
 */
class ViewportCameraController
{
public:
    [[nodiscard]] bool update(
        engine::PerspectiveCamera& camera,
        const ViewportCameraInput& input) const noexcept;

    void focus(engine::PerspectiveCamera& camera, const glm::vec3& target) const noexcept;

    float move_speed = 6.0f;
    float rotation_sensitivity = 0.15f;
    float pan_speed = 0.01f;
    float zoom_speed = 0.75f;
    float boost_multiplier = 3.0f;
    float focus_distance = 5.0f;

private:
    [[nodiscard]] glm::vec3 movement_direction(
        const engine::PerspectiveCamera& camera,
        const ViewportCameraInput& input) const noexcept;
};

// 向下兼容别名
using CameraController = ViewportCameraController;

}
