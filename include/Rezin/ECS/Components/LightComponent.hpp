#pragma once

#include <glm/vec3.hpp>

#include <cstdint>

namespace rezin
{

// A single component type keeps the game-facing API similar to Unity's Light
// component. The renderer can branch or group lights by this compact type.
enum class LightType : std::uint8_t
{
    Directional,
    Point,
    Spot
};

// LightComponent contains configuration data only. Position and rotation come
// from TransformComponent, giving every light one authoritative transform:
//
//     TransformComponent + LightComponent
//
// A future LightSystem or renderer will read these values and upload the
// appropriate directional, point, or spot-light data to shaders.
struct LightComponent
{
    LightType type = LightType::Point;

    // Base RGB color shared by the ambient, diffuse, and specular terms.
    glm::vec3 color{1.0f};

    // Independent strengths allow the renderer to derive the three Phong
    // colors without storing three mostly duplicated RGB vectors.
    float ambientIntensity = 0.05f;
    float diffuseIntensity = 0.8f;
    float specularIntensity = 1.0f;

    // Point and spot lights use these values to fade over distance.
    // Directional lights ignore attenuation because they model distant light.
    float constantAttenuation = 1.0f;
    float linearAttenuation = 0.09f;
    float quadraticAttenuation = 0.032f;

    // Spot angles are stored in degrees for an editor-friendly API. A future
    // renderer will convert them to cosine cutoff values for the shader.
    float innerConeAngle = 12.5f;
    float outerConeAngle = 15.0f;

    bool enabled = true;
    bool castsShadows = false;
};

}
