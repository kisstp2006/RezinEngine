#pragma once

#include <Rezin/ECS/System.hpp>

#include <cstddef>

namespace rezin
{

struct LightComponent;
struct TransformComponent;

class ShaderProgram;

// LightSystem contains the behavior shared by every light entity.
//
// Game code stores lighting settings in LightComponent and placement in
// TransformComponent. Each update, this system queries those components and
// uploads the resulting scene lighting data to the active lighting shader.
class LightSystem final : public System
{
public:
    explicit LightSystem(ShaderProgram& lightingShader) noexcept;
    ~LightSystem() override = default;

protected:
    // Called once when the system is added to a World.
    void onCreate(World& world) override;

    // Called by World::update() to collect and upload the current lights.
    void onUpdate(World& world, float deltaSeconds) override;

    // Called once before the World removes its systems.
    void onDestroy(World& world) noexcept override;

private:
    void uploadDirectionalLight(
        const TransformComponent& transform,
        const LightComponent& light
    );

    void uploadPointLight(
        std::size_t index,
        const TransformComponent& transform,
        const LightComponent& light
    );

    void uploadSpotLight(
        const TransformComponent& transform,
        const LightComponent& light
    );

    ShaderProgram& lightingShader_;
};

}
