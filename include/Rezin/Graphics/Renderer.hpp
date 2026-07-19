#pragma once

#include <glm/vec4.hpp>

#include <cstdint>

namespace rezin
{

enum class DepthCompareFunction
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

struct RendererSpecification
{
    glm::vec4 clearColor{0.1f, 0.1f, 0.1f, 1.0f};
    bool depthTesting{true};
    bool depthWrite{true};
    bool stencilTest{true};
    DepthCompareFunction depthFunction{DepthCompareFunction::Less};
    std::uint32_t depthBufferBits{24};
};

struct RendererCapabilities
{
    std::uint32_t maximumVertexAttributes{0};
    std::uint32_t maximumCombinedTextureUnits{0};
    std::uint32_t maximumTextureSize{0};
};

// Renderer owns global graphics state for the current OpenGL context.
// Application initializes it automatically, so sandbox code only uses this
// engine API and never needs to include GLAD or call raw OpenGL functions.
class Renderer final
{
public:
    Renderer() = delete;

    static void initialize(
        const RendererSpecification& specification
    );
    static void shutdown() noexcept;

    // Called by Application before user rendering. It clears both the color
    // and depth buffers so each frame starts with predictable framebuffer data.
    static void beginFrame();

    static void setViewport(
        std::uint32_t width,
        std::uint32_t height
    );

    static void setClearColor(const glm::vec4& color);
    static void setDepthTesting(bool enabled);
    static void setDepthWrite(bool enabled);
    static void setDepthFunction(DepthCompareFunction function);

    [[nodiscard]] static const RendererSpecification& specification();
    [[nodiscard]] static const RendererCapabilities& capabilities();
    [[nodiscard]] static bool isInitialized() noexcept;
};

}
