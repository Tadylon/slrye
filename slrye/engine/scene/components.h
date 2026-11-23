#pragma once

#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <engine/scene/entity_id.h>

namespace slrye::engine
{

/**
 * @brief 实体名称组件（可读标识）
 */
struct NameComponent
{
    std::string value = "Entity"; ///< 实体在层级树和属性面板中展示的名称
};

/**
 * @brief 空间变换组件（3D 坐标空间三要素，新 Entity 默认自带）
 */
struct TransformComponent
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};      ///< 本地世界坐标系空间三维位置 (X, Y, Z)
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};///< 四元数表示的三维旋转 (W, X, Y, Z)
    glm::vec3 scale{1.0f, 1.0f, 1.0f};         ///< 三轴缩放比例 (SX, SY, SZ)
};

/**
 * @brief 实体场景树层级关系组件
 */
struct HierarchyComponent
{
    EntityId parent;                    ///< 父实体稳定 ID（无效 ID 表示当前为根节点）
    std::vector<EntityId> children;     ///< 直接子实体稳定 ID 列表
};

/**
 * @brief 场景内虚拟摄像机组件
 */
struct CameraComponent
{
    float vertical_fov_degrees = 60.0f; ///< 垂直视场角（度）
    float near_clip = 0.1f;             ///< 近裁剪平面距离
    float far_clip = 1000.0f;           ///< 远裁剪平面距离
    bool primary = true;                ///< 是否作为当前场景的主摄像机
};

/**
 * @brief 光源类型枚举
 */
enum class LightType
{
    Directional, ///< 平行光（模拟太阳光，无距离衰减，具有全局一致方向）
    Point,       ///< 点光源（从一点向周围球形辐射，具有最大照射范围与距离平方反比衰减）
    Spot         ///< 聚光灯（圆锥形定向辐射，具有内圆锥角与外圆锥角羽化）
};

/**
 * @brief 场景光照组件
 */
struct LightComponent
{
    LightType type = LightType::Point;  ///< 光源类型
    glm::vec3 color{1.0f, 1.0f, 1.0f};  ///< 光源基色 RGB [0, 1]
    float intensity = 1.0f;             ///< 光照强度系数
    float range = 10.0f;                ///< 最大有效辐射距离（点光源与聚光灯适用）
    float inner_cone_degrees = 20.0f;   ///< 聚光灯内锥半角（完全高亮区）
    float outer_cone_degrees = 30.0f;   ///< 聚光灯外锥半角（平滑渐变到全黑边缘）
};

/**
 * @brief 物理碰撞体形状枚举
 */
enum class ColliderType
{
    Box,     ///< 轴对称定向盒（AABB / OBB）
    Sphere,  ///< 几何球体
    Capsule  ///< 直立圆柱胶囊体
};

/**
 * @brief 几何碰撞盒组件（用于射线拾取与物理检测）
 */
struct ColliderComponent
{
    ColliderType type = ColliderType::Box;   ///< 碰撞体类型
    glm::vec3 half_extents{0.5f, 0.5f, 0.5f};///< 盒体从中心到各面的半长半宽半高 (X, Y, Z)
    float radius = 0.5f;                     ///< 球体或胶囊体截面半径
    float height = 2.0f;                     ///< 胶囊体总高度
    bool enabled = true;                     ///< 碰撞体是否生效
};

}

namespace slrye
{
using engine::NameComponent;
using engine::TransformComponent;
using engine::HierarchyComponent;
using engine::CameraComponent;
using engine::LightType;
using engine::LightComponent;
using engine::ColliderType;
using engine::ColliderComponent;
}
