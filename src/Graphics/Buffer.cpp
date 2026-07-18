#include <Rezin/Graphics/Buffer.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace
{

GLenum toOpenGlUsage(rezin::BufferUsage usage)
{
    switch (usage)
    {
        case rezin::BufferUsage::Static:
            return GL_STATIC_DRAW;
        case rezin::BufferUsage::Dynamic:
            return GL_DYNAMIC_DRAW;
    }

    return GL_STATIC_DRAW;
}

GLsizeiptr toOpenGlSize(std::size_t sizeBytes)
{
    if (sizeBytes > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()))
        throw std::overflow_error(
            "Buffer size is too large for OpenGL's GLsizeiptr type."
        );

    return static_cast<GLsizeiptr>(sizeBytes);
}

GLintptr toOpenGlOffset(std::size_t offsetBytes)
{
    if (offsetBytes > static_cast<std::size_t>(std::numeric_limits<GLintptr>::max()))
        throw std::overflow_error(
            "Buffer offset is too large for OpenGL's GLintptr type."
        );

    return static_cast<GLintptr>(offsetBytes);
}

}

namespace rezin
{

std::uint32_t shaderDataTypeSize(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:  return sizeof(float);
        case ShaderDataType::Float2: return sizeof(float) * 2;
        case ShaderDataType::Float3: return sizeof(float) * 3;
        case ShaderDataType::Float4: return sizeof(float) * 4;
        case ShaderDataType::Int:    return sizeof(std::int32_t);
        case ShaderDataType::Int2:   return sizeof(std::int32_t) * 2;
        case ShaderDataType::Int3:   return sizeof(std::int32_t) * 3;
        case ShaderDataType::Int4:   return sizeof(std::int32_t) * 4;
        case ShaderDataType::UInt:   return sizeof(std::uint32_t);
        case ShaderDataType::UInt2:  return sizeof(std::uint32_t) * 2;
        case ShaderDataType::UInt3:  return sizeof(std::uint32_t) * 3;
        case ShaderDataType::UInt4:  return sizeof(std::uint32_t) * 4;
        case ShaderDataType::None:
            break;
    }

    throw std::invalid_argument(
        "Unknown ShaderDataType: its size cannot be calculated."
    );
}

std::uint32_t shaderDataTypeComponentCount(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:
        case ShaderDataType::Int:
        case ShaderDataType::UInt:
            return 1;

        case ShaderDataType::Float2:
        case ShaderDataType::Int2:
        case ShaderDataType::UInt2:
            return 2;

        case ShaderDataType::Float3:
        case ShaderDataType::Int3:
        case ShaderDataType::UInt3:
            return 3;

        case ShaderDataType::Float4:
        case ShaderDataType::Int4:
        case ShaderDataType::UInt4:
            return 4;

        case ShaderDataType::None:
            break;
    }

    throw std::invalid_argument(
        "Unknown ShaderDataType: its component count cannot be calculated."
    );
}

BufferElement::BufferElement(
    ShaderDataType elementType,
    std::string elementName,
    bool shouldNormalize
)
    : name(std::move(elementName)),
      type(elementType),
      size(shaderDataTypeSize(elementType)),
      normalized(shouldNormalize)
{
}

BufferLayout::BufferLayout(std::initializer_list<BufferElement> elements)
    : elements_(elements)
{
    calculateOffsetsAndStride();
}

const std::vector<BufferElement>& BufferLayout::elements() const noexcept
{
    return elements_;
}

std::uint32_t BufferLayout::stride() const noexcept
{
    return stride_;
}

bool BufferLayout::empty() const noexcept
{
    return elements_.empty();
}

void BufferLayout::calculateOffsetsAndStride()
{
    // Offsets are measured from the start of one vertex. The stride is the
    // number of bytes OpenGL skips to reach the same field in the next vertex.
    std::size_t currentOffset = 0;
    std::uint64_t currentStride = 0;

    for (BufferElement& element : elements_)
    {
        element.offset = currentOffset;
        currentOffset += element.size;
        currentStride += element.size;
    }

    if (currentStride > std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error(
            "Vertex layout stride exceeds the supported 32-bit size."
        );

    stride_ = static_cast<std::uint32_t>(currentStride);
}

VertexBuffer::VertexBuffer(
    const void* data,
    std::size_t sizeBytes,
    BufferUsage usage
)
    : sizeBytes_(sizeBytes)
{
    if (sizeBytes_ == 0)
        throw std::invalid_argument(
            "Cannot create a VertexBuffer with zero bytes of storage."
        );

    // Direct State Access creates and fills this buffer without changing the
    // currently bound GL_ARRAY_BUFFER, keeping unrelated OpenGL state intact.
    glCreateBuffers(1, &bufferId_);
    if (bufferId_ == 0)
        throw std::runtime_error(
            "OpenGL failed to create the VertexBuffer object."
        );

    glNamedBufferData(
        bufferId_,
        toOpenGlSize(sizeBytes_),
        data,
        toOpenGlUsage(usage)
    );
}

VertexBuffer::~VertexBuffer()
{
    // RAII: releasing the C++ object also releases its GPU resource.
    if (bufferId_ != 0)
        glDeleteBuffers(1, &bufferId_);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : bufferId_(std::exchange(other.bufferId_, 0)),
      sizeBytes_(std::exchange(other.sizeBytes_, 0)),
      layout_(std::move(other.layout_))
{
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
    if (this == &other)
        return *this;

    if (bufferId_ != 0)
        glDeleteBuffers(1, &bufferId_);

    bufferId_ = std::exchange(other.bufferId_, 0);
    sizeBytes_ = std::exchange(other.sizeBytes_, 0);
    layout_ = std::move(other.layout_);
    return *this;
}

void VertexBuffer::bind() const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, bufferId_);
}

void VertexBuffer::unbind() noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::setData(
    const void* data,
    std::size_t sizeBytes,
    std::size_t offsetBytes
)
{
    if (data == nullptr && sizeBytes != 0)
        throw std::invalid_argument(
            "VertexBuffer update data cannot be null when the update size is greater than zero."
        );

    if (offsetBytes > sizeBytes_ || sizeBytes > sizeBytes_ - offsetBytes)
        throw std::out_of_range(
            "VertexBuffer update would write past the end of the allocated GPU buffer."
        );

    glNamedBufferSubData(
        bufferId_,
        toOpenGlOffset(offsetBytes),
        toOpenGlSize(sizeBytes),
        data
    );
}

void VertexBuffer::setLayout(BufferLayout layout)
{
    if (layout.empty())
        throw std::invalid_argument(
            "VertexBuffer layout cannot be empty because OpenGL needs to know how each vertex is structured."
        );

    layout_ = std::move(layout);
}

const BufferLayout& VertexBuffer::layout() const noexcept
{
    return layout_;
}

GLuint VertexBuffer::id() const noexcept
{
    return bufferId_;
}

std::size_t VertexBuffer::sizeBytes() const noexcept
{
    return sizeBytes_;
}

IndexBuffer::IndexBuffer(
    const std::uint32_t* indices,
    std::uint32_t count,
    BufferUsage usage
)
    : count_(count)
{
    if (indices == nullptr || count_ == 0)
        throw std::invalid_argument(
            "Cannot create an IndexBuffer without index data and at least one index."
        );

    const std::size_t sizeBytes = static_cast<std::size_t>(count_) * sizeof(std::uint32_t);

    glCreateBuffers(1, &bufferId_);
    if (bufferId_ == 0)
        throw std::runtime_error(
            "OpenGL failed to create the IndexBuffer object."
        );

    glNamedBufferData(
        bufferId_,
        toOpenGlSize(sizeBytes),
        indices,
        toOpenGlUsage(usage)
    );
}

IndexBuffer::~IndexBuffer()
{
    if (bufferId_ != 0)
        glDeleteBuffers(1, &bufferId_);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : bufferId_(std::exchange(other.bufferId_, 0)),
      count_(std::exchange(other.count_, 0))
{
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
{
    if (this == &other)
        return *this;

    if (bufferId_ != 0)
        glDeleteBuffers(1, &bufferId_);

    bufferId_ = std::exchange(other.bufferId_, 0);
    count_ = std::exchange(other.count_, 0);
    return *this;
}

void IndexBuffer::bind() const noexcept
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferId_);
}

void IndexBuffer::unbind() noexcept
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLuint IndexBuffer::id() const noexcept
{
    return bufferId_;
}

std::uint32_t IndexBuffer::count() const noexcept
{
    return count_;
}

}
