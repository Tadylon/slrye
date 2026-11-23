#pragma once

#include <optional>
#include <string>
#include <vector>

#include <engine/scene/entity_id.h>

namespace slrye::engine
{

class Scene;

/**
 * @brief 轻量级实体对象句柄（Handle / Value Object）
 * 
 * 职责与层级关系：
 * 1. 位于 Scene ECS 体系，采用典型的 Handle 模式（仅占 16 字节：一个指针 + 一个 64 位 ID）；
 * 2. 自身【不存储任何组件数据】，只是指向 Scene 内部 EnTT 注册表条目的借用句柄；
 * 3. 具备值拷贝语义（可随意按值传递或存入容器），通过 EntityId 与所属 Scene 强绑定；
 * 4. 提供流畅易用的强类型组件增删查改（add / get / has / remove）与父子节点层级操作。
 */
class Entity
{
public:
    Entity() = default;

    /// @brief 检查句柄是否关联了有效的场景且实体在场景中未被销毁
    [[nodiscard]] bool is_valid() const noexcept;

    /// @brief 获取实体的全局唯一 64 位 ID
    [[nodiscard]] EntityId id() const noexcept;

    /// @brief 获取实体的显示名称（底层访问 NameComponent）
    [[nodiscard]] std::string name() const;
    void set_name(std::string name);

    /// @brief 获取父节点实体 ID（底层访问 HierarchyComponent）
    [[nodiscard]] std::optional<EntityId> parent() const;

    /// @brief 获取直接子节点实体 ID 列表
    [[nodiscard]] std::vector<EntityId> children() const;

    /// @brief 设置新的父实体，自动维护原父子双向引用关系
    bool set_parent(Entity parent);

    /// @brief 脱离父节点成为根实体
    bool clear_parent();

    /// @brief 销毁当前实体及其所有递归子实体
    bool destroy();

    /// @brief 标记场景结构或组件发生变动（触发视口重绘）
    void mark_dirty();

    template<typename T, typename... Args>
    T& add(Args&&... args);

    template<typename T>
    [[nodiscard]] bool has() const noexcept;

    template<typename T>
    [[nodiscard]] T* try_get() noexcept;

    template<typename T>
    [[nodiscard]] const T* try_get() const noexcept;

    template<typename T>
    T& get();

    template<typename T>
    const T& get() const;

    template<typename T>
    bool remove();

    bool operator==(const Entity& other) const noexcept
    {
        return scene_ == other.scene_ && id_ == other.id_;
    }

    bool operator!=(const Entity& other) const noexcept
    {
        return !(*this == other);
    }

private:
    friend class Scene;

    Entity(Scene* scene, EntityId id) noexcept
        : scene_(scene),
          id_(id)
    {
    }

    Scene* scene_ = nullptr; ///< 借用的所属场景指针（非拥有）
    EntityId id_;            ///< 实体的 64 位稳定全局标识符
};

}

namespace slrye
{
using engine::Entity;
}
