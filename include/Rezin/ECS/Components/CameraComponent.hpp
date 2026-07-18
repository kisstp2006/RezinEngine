#pragma once

#include <cstdint>

namespace rezin
{

// A camera may use perspective projection for a 3D scene or orthographic
// projection for 2D rendering, editor views, and some user interfaces.
enum class CameraProjection : std::uint8_t
{
    Perspective,
    Orthographic
};

// CameraComponent contains only lens and projection settings. Position and
// rotation belong to TransformComponent, so every entity has one authoritative
// transform instead of separate transform values in multiple components.
//
// An entity becomes a camera by owning both components:
//     TransformComponent + CameraComponent
//
// A future CameraSystem will query that pair and calculate view/projection
// matrices for the renderer. No inheritance or manual registration is needed.
struct CameraComponent
{
    CameraProjection projection = CameraProjection::Perspective;

    // Vertical field of view in degrees. Used only by perspective cameras.
    float verticalFieldOfView = 45.0f;

    // Visible objects must be farther than nearClip and closer than farClip.
    // nearClip must stay positive, and farClip must be greater than nearClip.
    float nearClip = 0.1f;
    float farClip = 100.0f;

    // Vertical size of an orthographic camera's visible area. The horizontal
    // size is derived from this value and the framebuffer aspect ratio.
    float orthographicSize = 10.0f;

    // The renderer will eventually select one primary camera per World.
    bool primary = true;
};

}
