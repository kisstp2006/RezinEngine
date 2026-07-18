#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace rezin
{

// This is a complete ECS component: a plain struct containing only per-entity
// data. It does not inherit from anything and has no update function. A future
// TransformSystem will process many TransformComponent values in one query.
struct TransformComponent
{
    glm::vec3 position{0.0f};

    // Quaternion identity means "no rotation". Quaternions avoid the gimbal
    // lock problems that appear when three Euler angles are combined.
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    glm::vec3 scale{1.0f};
};

}
