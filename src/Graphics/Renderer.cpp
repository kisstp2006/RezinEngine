#include <Rezin/Graphics/Renderer.hpp>

#include <glad/glad.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace
{

struct RendererState
{
    rezin::RendererSpecification specification;
    rezin::RendererCapabilities capabilities;
    bool initialized{false};
};

RendererState& state() noexcept
{
    static RendererState rendererState;
    return rendererState;
}

void requireInitialized()
{
    if (!state().initialized)
    {
        throw std::logic_error(
            "Renderer is not initialized. Application must create a graphics "
            "context before renderer API calls are made."
        );
    }
}

GLenum toOpenGlDepthFunction(rezin::DepthCompareFunction function)
{
    switch (function)
    {
        case rezin::DepthCompareFunction::Never:
            return GL_NEVER;
        case rezin::DepthCompareFunction::Less:
            return GL_LESS;
        case rezin::DepthCompareFunction::Equal:
            return GL_EQUAL;
        case rezin::DepthCompareFunction::LessEqual:
            return GL_LEQUAL;
        case rezin::DepthCompareFunction::Greater:
            return GL_GREATER;
        case rezin::DepthCompareFunction::NotEqual:
            return GL_NOTEQUAL;
        case rezin::DepthCompareFunction::GreaterEqual:
            return GL_GEQUAL;
        case rezin::DepthCompareFunction::Always:
            return GL_ALWAYS;
    }

    throw std::invalid_argument("Unknown depth comparison function.");
}

std::uint32_t nonNegativeCapability(GLint value) noexcept
{
    return static_cast<std::uint32_t>(std::max(value, 0));
}

}

namespace rezin
{

void Renderer::initialize(
    const RendererSpecification& specification
)
{
    RendererState& rendererState = state();
    if (rendererState.initialized)
        throw std::logic_error("Renderer is already initialized.");

    if (specification.depthBufferBits == 0)
    {
        throw std::invalid_argument(
            "Renderer depth buffer precision must be greater than zero."
        );
    }

    rendererState.specification = specification;

    GLint maximumVertexAttributes = 0;
    GLint maximumCombinedTextureUnits = 0;
    GLint maximumTextureSize = 0;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maximumVertexAttributes);
    glGetIntegerv(
        GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
        &maximumCombinedTextureUnits
    );
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);

    rendererState.capabilities.maximumVertexAttributes =
        nonNegativeCapability(maximumVertexAttributes);
    rendererState.capabilities.maximumCombinedTextureUnits =
        nonNegativeCapability(maximumCombinedTextureUnits);
    rendererState.capabilities.maximumTextureSize =
        nonNegativeCapability(maximumTextureSize);

    glClearColor(
        specification.clearColor.r,
        specification.clearColor.g,
        specification.clearColor.b,
        specification.clearColor.a
    );
    glClearDepth(1.0);
    glDepthMask(specification.depthWrite ? GL_TRUE : GL_FALSE);
    glDepthFunc(toOpenGlDepthFunction(specification.depthFunction));

    if (specification.depthTesting)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    rendererState.initialized = true;
}

void Renderer::shutdown() noexcept
{
    state() = RendererState{};
}

void Renderer::beginFrame()
{
    requireInitialized();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

void Renderer::setViewport(
    std::uint32_t width,
    std::uint32_t height
)
{
    requireInitialized();

    if (width > static_cast<std::uint32_t>(
            std::numeric_limits<GLsizei>::max())
        || height > static_cast<std::uint32_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        throw std::out_of_range(
            "Renderer viewport dimensions exceed OpenGL limits."
        );
    }

    glViewport(
        0,
        0,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height)
    );
}

void Renderer::setClearColor(const glm::vec4& color)
{
    requireInitialized();
    state().specification.clearColor = color;
    glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::setDepthTesting(bool enabled)
{
    requireInitialized();
    state().specification.depthTesting = enabled;

    if (enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    state().specification.stencilTesting = enabled;
    if (enabled)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

void Renderer::setDepthWrite(bool enabled)
{
    requireInitialized();
    state().specification.depthWrite = enabled;
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void Renderer::setDepthFunction(DepthCompareFunction function)
{
    requireInitialized();
    state().specification.depthFunction = function;
    glDepthFunc(toOpenGlDepthFunction(function));
}

const RendererSpecification& Renderer::specification()
{
    requireInitialized();
    return state().specification;
}

const RendererCapabilities& Renderer::capabilities()
{
    requireInitialized();
    return state().capabilities;
}

bool Renderer::isInitialized() noexcept
{
    return state().initialized;
}

}
