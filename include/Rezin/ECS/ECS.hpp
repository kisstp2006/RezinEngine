#pragma once

// Convenience header for game code. Include individual headers instead when a
// file only needs one ECS type and you want to minimize compile times.
#include <Rezin/ECS/Component.hpp>
#include <Rezin/ECS/Components/CameraComponent.hpp>
#include <Rezin/ECS/Components/LightComponent.hpp>
#include <Rezin/ECS/Components/TransformComponent.hpp>
#include <Rezin/ECS/Entity.hpp>
#include <Rezin/ECS/EntityManager.hpp>
#include <Rezin/ECS/System.hpp>
#include <Rezin/ECS/Systems/LightSystem.hpp>
#include <Rezin/ECS/World.hpp>
