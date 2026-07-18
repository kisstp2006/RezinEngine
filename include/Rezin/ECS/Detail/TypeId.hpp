#pragma once

#include <atomic>
#include <cstddef>

namespace rezin::detail
{

using TypeId = std::size_t;

// Separate families prevent a component and a system from sharing the same ID
// counter. IDs are generated without RTTI, which keeps ECS lookup lightweight
// and avoids depending on platform-specific std::type_info behavior.
struct ComponentTypeFamily final {};
struct SystemTypeFamily final {};

template<typename Family>
TypeId nextTypeId() noexcept
{
    static std::atomic<TypeId> nextId{0};
    return nextId.fetch_add(1, std::memory_order_relaxed);
}

template<typename Type, typename Family>
TypeId typeId() noexcept
{
    // Each Type/Family specialization initializes this value exactly once and
    // returns the same ID for the rest of the program's lifetime.
    static const TypeId id = nextTypeId<Family>();
    return id;
}

}
