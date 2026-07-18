#pragma once

#include <concepts>
#include <type_traits>

namespace rezin
{

// A component is plain data attached to an entity. It does not inherit from a
// base class and does not need registration. Put behavior in a System so one
// loop can process many components stored next to each other in memory.
//
// Example:
// struct HealthComponent
// {
//     float current = 100.0f;
//     float maximum = 100.0f;
// };
template<typename Type>
concept ComponentData =
    std::is_object_v<Type>
    && !std::is_pointer_v<Type>
    && !std::is_reference_v<Type>
    && std::move_constructible<Type>
    && std::is_move_assignable_v<Type>;

}
