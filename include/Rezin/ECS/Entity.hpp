#pragma once

#include <cstdint>
#include <limits>

namespace rezin
{

class EntityManager;

// Entity is a small, copyable handle rather than an object containing gameplay
// data. The index locates an entity, while generation detects stale handles
// after that index has been destroyed and reused.
class Entity final
{
public:
    using Index = std::uint32_t;
    using Generation = std::uint32_t;
    using WorldId = std::uint32_t;

    static constexpr Index invalidIndex =
        std::numeric_limits<Index>::max();

    constexpr Entity() noexcept = default;

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        return index_ != invalidIndex && worldId_ != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return isValid();
    }

    [[nodiscard]] constexpr Index index() const noexcept
    {
        return index_;
    }

    [[nodiscard]] constexpr Generation generation() const noexcept
    {
        return generation_;
    }

    [[nodiscard]] constexpr WorldId worldId() const noexcept
    {
        return worldId_;
    }

    [[nodiscard]] friend constexpr bool operator==(
        Entity left,
        Entity right
    ) noexcept = default;

private:
    friend class EntityManager;

    constexpr Entity(
        Index index,
        Generation generation,
        WorldId worldId
    ) noexcept
        : index_(index),
          generation_(generation),
          worldId_(worldId)
    {
    }

    Index index_ = invalidIndex;
    Generation generation_ = 0;
    WorldId worldId_ = 0;
};

}
