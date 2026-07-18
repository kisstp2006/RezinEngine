#include <Rezin/Graphics/VertexArray.hpp>

#include <stdexcept>
#include <utility>

namespace
{

GLenum toOpenGlBaseType(rezin::ShaderDataType type)
{
    switch (type)
    {
        case rezin::ShaderDataType::Float:
        case rezin::ShaderDataType::Float2:
        case rezin::ShaderDataType::Float3:
        case rezin::ShaderDataType::Float4:
            return GL_FLOAT;

        case rezin::ShaderDataType::Int:
        case rezin::ShaderDataType::Int2:
        case rezin::ShaderDataType::Int3:
        case rezin::ShaderDataType::Int4:
            return GL_INT;

        case rezin::ShaderDataType::UInt:
        case rezin::ShaderDataType::UInt2:
        case rezin::ShaderDataType::UInt3:
        case rezin::ShaderDataType::UInt4:
            return GL_UNSIGNED_INT;

        case rezin::ShaderDataType::None:
            break;
    }

    throw std::invalid_argument(
        "Unknown ShaderDataType: it cannot be converted to an OpenGL base type."
    );
}

bool isIntegerType(rezin::ShaderDataType type)
{
    switch (type)
    {
        case rezin::ShaderDataType::Int:
        case rezin::ShaderDataType::Int2:
        case rezin::ShaderDataType::Int3:
        case rezin::ShaderDataType::Int4:
        case rezin::ShaderDataType::UInt:
        case rezin::ShaderDataType::UInt2:
        case rezin::ShaderDataType::UInt3:
        case rezin::ShaderDataType::UInt4:
            return true;

        case rezin::ShaderDataType::Float:
        case rezin::ShaderDataType::Float2:
        case rezin::ShaderDataType::Float3:
        case rezin::ShaderDataType::Float4:
        case rezin::ShaderDataType::None:
            return false;
    }

    return false;
}

}

namespace rezin
{

VertexArray::VertexArray()
{
    // A Vertex Array Object stores the connection between shader input
    // locations and fields inside one or more vertex buffers.
    glCreateVertexArrays(1, &vertexArrayId_);
    if (vertexArrayId_ == 0)
        throw std::runtime_error(
            "OpenGL failed to create the VertexArray object."
        );
}

VertexArray::~VertexArray()
{
    if (vertexArrayId_ != 0)
        glDeleteVertexArrays(1, &vertexArrayId_);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : vertexArrayId_(std::exchange(other.vertexArrayId_, 0)),
      nextAttributeIndex_(std::exchange(other.nextAttributeIndex_, 0)),
      nextBindingIndex_(std::exchange(other.nextBindingIndex_, 0))
{
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this == &other)
        return *this;

    if (vertexArrayId_ != 0)
        glDeleteVertexArrays(1, &vertexArrayId_);

    vertexArrayId_ = std::exchange(other.vertexArrayId_, 0);
    nextAttributeIndex_ = std::exchange(other.nextAttributeIndex_, 0);
    nextBindingIndex_ = std::exchange(other.nextBindingIndex_, 0);
    return *this;
}

void VertexArray::bind() const noexcept
{
    glBindVertexArray(vertexArrayId_);
}

void VertexArray::unbind() noexcept
{
    glBindVertexArray(0);
}

void VertexArray::addVertexBuffer(const VertexBuffer& vertexBuffer)
{
    const BufferLayout& layout = vertexBuffer.layout();
    if (layout.empty())
        throw std::invalid_argument(
            "Assign a BufferLayout to the VertexBuffer before adding it to a VertexArray. "
            "The layout tells OpenGL where each vertex attribute is stored."
        );

    // The binding describes the buffer and vertex stride. The attributes below
    // describe its individual fields, such as position, color, or texture UV.
    const GLuint bindingIndex = nextBindingIndex_++;

    glVertexArrayVertexBuffer(
        vertexArrayId_,
        bindingIndex,
        vertexBuffer.id(),
        0,
        static_cast<GLsizei>(layout.stride())
    );

    for (const BufferElement& element : layout.elements())
    {
        const GLuint attributeIndex = nextAttributeIndex_++;
        const GLint componentCount = static_cast<GLint>(
            shaderDataTypeComponentCount(element.type)
        );
        const GLenum baseType = toOpenGlBaseType(element.type);
        const GLuint relativeOffset = static_cast<GLuint>(element.offset);

        glEnableVertexArrayAttrib(vertexArrayId_, attributeIndex);

        if (isIntegerType(element.type))
        {
            glVertexArrayAttribIFormat(
                vertexArrayId_,
                attributeIndex,
                componentCount,
                baseType,
                relativeOffset
            );
        }
        else
        {
            glVertexArrayAttribFormat(
                vertexArrayId_,
                attributeIndex,
                componentCount,
                baseType,
                element.normalized ? GL_TRUE : GL_FALSE,
                relativeOffset
            );
        }

        glVertexArrayAttribBinding(
            vertexArrayId_,
            attributeIndex,
            bindingIndex
        );
    }
}

void VertexArray::setIndexBuffer(const IndexBuffer& indexBuffer)
{
    glVertexArrayElementBuffer(vertexArrayId_, indexBuffer.id());
}

GLuint VertexArray::id() const noexcept
{
    return vertexArrayId_;
}

}
