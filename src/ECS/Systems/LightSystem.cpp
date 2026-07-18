#include <Rezin/ECS/Systems/LightSystem.hpp>

#include <Rezin/ECS/Components/LightComponent.hpp>
#include <Rezin/ECS/Components/TransformComponent.hpp>
#include <Rezin/ECS/EntityManager.hpp>
#include <Rezin/ECS/World.hpp>
#include <Rezin/Graphics/ShaderProgram.hpp>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <string>

namespace rezin
{

namespace
{

constexpr std::size_t maximumPointLights = 4;

glm::vec3 lightDirection(const TransformComponent& transform)
{
    return glm::normalize(
        transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f)
    );
}

}

LightSystem::LightSystem(ShaderProgram& lightingShader) noexcept
    : lightingShader_(lightingShader)
{
}

void LightSystem::onCreate(World& world)
{
    // The current implementation does not own additional resources.
    static_cast<void>(world);
}

void LightSystem::onUpdate(World& world, float deltaSeconds)
{
    std::size_t pointLightCount = 0;
    bool hasDirectionalLight = false;
    bool hasSpotLight = false;

    static_cast<void>(deltaSeconds);

    // ShaderProgram uses glUniform calls, so its OpenGL program must be active
    // while the system uploads the current frame's light data.
    lightingShader_.use();

    EntityManager& entities = world.entityManager();

    entities.query<TransformComponent, LightComponent>(
        [this,
         &pointLightCount,
         &hasDirectionalLight,
         &hasSpotLight](Entity entity,
           const TransformComponent& transform,
           const LightComponent& light)
        {
            static_cast<void>(entity);

            if (!light.enabled)
                return;

            switch (light.type)
            {
                case LightType::Directional:
                {
                    // The current forward shader supports one sun-like light.
                    if (!hasDirectionalLight)
                    {
                        uploadDirectionalLight(transform, light);
                        hasDirectionalLight = true;
                    }

                    break;
                }

                case LightType::Point:
                {
                    if (pointLightCount < maximumPointLights)
                    {
                        uploadPointLight(
                            pointLightCount,
                            transform,
                            light
                        );
                        ++pointLightCount;
                    }

                    break;
                }

                case LightType::Spot:
                {
                    // The current forward shader supports one spotlight.
                    if (!hasSpotLight)
                    {
                        uploadSpotLight(transform, light);
                        hasSpotLight = true;
                    }

                    break;
                }
            }
        }
    );

    lightingShader_.setBool(
        "hasDirectionalLight",
        hasDirectionalLight
    );

    lightingShader_.setInt(
        "pointLightCount",
        static_cast<int>(pointLightCount)
    );

    lightingShader_.setBool(
        "hasSpotLight",
        hasSpotLight
    );
}

void LightSystem::onDestroy(World& world) noexcept
{
    // Cleanup will be added only if the final implementation owns resources.
    static_cast<void>(world);
}

void LightSystem::uploadDirectionalLight(
    const TransformComponent& transform,
    const LightComponent& light
)
{
    lightingShader_.setVec3(
        "dirLight.direction",
        lightDirection(transform)
    );
    lightingShader_.setVec3(
        "dirLight.ambient",
        light.color * light.ambientIntensity
    );
    lightingShader_.setVec3(
        "dirLight.diffuse",
        light.color * light.diffuseIntensity
    );
    lightingShader_.setVec3(
        "dirLight.specular",
        light.color * light.specularIntensity
    );
}

void LightSystem::uploadPointLight(
    std::size_t index,
    const TransformComponent& transform,
    const LightComponent& light
)
{
    const std::string prefix =
        "pointLights[" + std::to_string(index) + "].";

    lightingShader_.setVec3(prefix + "position", transform.position);
    lightingShader_.setFloat(
        prefix + "constant",
        light.constantAttenuation
    );
    lightingShader_.setFloat(
        prefix + "linear",
        light.linearAttenuation
    );
    lightingShader_.setFloat(
        prefix + "quadratic",
        light.quadraticAttenuation
    );
    lightingShader_.setVec3(
        prefix + "ambient",
        light.color * light.ambientIntensity
    );
    lightingShader_.setVec3(
        prefix + "diffuse",
        light.color * light.diffuseIntensity
    );
    lightingShader_.setVec3(
        prefix + "specular",
        light.color * light.specularIntensity
    );
}

void LightSystem::uploadSpotLight(
    const TransformComponent& transform,
    const LightComponent& light
)
{
    lightingShader_.setVec3("spotLight.position", transform.position);
    lightingShader_.setVec3(
        "spotLight.direction",
        lightDirection(transform)
    );
    lightingShader_.setFloat(
        "spotLight.cutOff",
        glm::cos(glm::radians(light.innerConeAngle))
    );
    lightingShader_.setFloat(
        "spotLight.outerCutOff",
        glm::cos(glm::radians(light.outerConeAngle))
    );
    lightingShader_.setFloat(
        "spotLight.constant",
        light.constantAttenuation
    );
    lightingShader_.setFloat(
        "spotLight.linear",
        light.linearAttenuation
    );
    lightingShader_.setFloat(
        "spotLight.quadratic",
        light.quadraticAttenuation
    );
    lightingShader_.setVec3(
        "spotLight.ambient",
        light.color * light.ambientIntensity
    );
    lightingShader_.setVec3(
        "spotLight.diffuse",
        light.color * light.diffuseIntensity
    );
    lightingShader_.setVec3(
        "spotLight.specular",
        light.color * light.specularIntensity
    );
}

}
