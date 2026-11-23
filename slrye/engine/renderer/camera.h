#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace slrye::engine
{

/**
 * @brief 空间射线数据结构（用于视口拾取与碰撞相交计算）
 */
struct Ray
{
    glm::vec3 origin{0.0f};                 ///< 射线原点（通常为摄像机空间位置）
    glm::vec3 direction{0.0f, 0.0f, -1.0f}; ///< 归一化的射线传播方向向量
};

/**
 * @brief 视口透视摄像机（数学模型与矩阵发生器）
 * 
 * 职责与层级关系：
 * 1. 位于 Renderer 层，是视口 3D 渲染与几何拾取的空间基准；
 * 2. 维护欧拉角（Yaw/Pitch）与空间位置，实时构建 LookAt 观察矩阵；
 * 3. 针对 Vulkan 图形规范生成透视投影矩阵（包括 Vulkan Y 轴朝下的矫正与深度范围 [0, 1] 映射）；
 * 4. 结合视口面板分辨率（Viewport UI Extent），动态更新宽高比（Aspect Ratio），并提供逆投影屏幕空间光线投射功能。
 */
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

    /// @brief 从归一化设备坐标 (NDC [-1, 1]) 生成发射射线
    [[nodiscard]] Ray ray_from_ndc(float x, float y) const noexcept;

    /// @brief 从 ImGui 视口像素坐标与尺寸生成拾取射线
    [[nodiscard]] Ray ray_from_viewport(
        float x,
        float y,
        float width,
        float height) const noexcept;

private:
    glm::vec3 position_{0.0f, 2.0f, 6.0f};  ///< 摄像机在 3D 世界空间的位置
    float yaw_degrees_ = 0.0f;               ///< 水平旋转偏航角（度）
    float pitch_degrees_ = -10.0f;           ///< 垂直旋转俯仰角（度，约束在 -89° ~ +89°）
    float vertical_fov_degrees_ = 60.0f;     ///< 垂直视场角 FOV（度）
    float aspect_ratio_ = 16.0f / 9.0f;      ///< 视口宽高比（width / height）
    float near_clip_ = 0.1f;                 ///< 近裁剪平面距离
    float far_clip_ = 1000.0f;               ///< 远裁剪平面距离
};

}

namespace slrye
{
using engine::Ray;
using engine::PerspectiveCamera;
}
