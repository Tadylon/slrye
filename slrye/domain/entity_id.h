#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace slrye
{

class EntityId
{
public:
    constexpr EntityId() noexcept = default;
    explicit constexpr EntityId(std::uint64_t value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]] constexpr std::uint64_t value() const noexcept
    {
        return value_;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept
    {
        return value_ != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return is_valid();
    }

    friend constexpr bool operator==(EntityId lhs, EntityId rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    friend constexpr bool operator!=(EntityId lhs, EntityId rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    std::uint64_t value_ = 0;
};

struct EntityIdHash
{
    [[nodiscard]] std::size_t operator()(EntityId id) const noexcept
    {
        return std::hash<std::uint64_t>{}(id.value());
    }
};

}
