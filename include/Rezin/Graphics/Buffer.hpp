#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace rezin
{

// Describes how one value is stored for each vertex. For example, Float3 is
// commonly used for an XYZ position, while Float2 is commonly used for UVs.
enum class ShaderDataType : std::uint8_t
{
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4
};

[[nodiscard]] std::uint32_t shaderDataTypeSize(ShaderDataType type);
[[nodiscard]] std::uint32_t shaderDataTypeComponentCount(ShaderDataType type);

struct BufferElement
{
    // The name helps debugging and leaves room for future shader reflection.
    // Currently, VertexArray assigns shader attribute locations by list order.
    std::string name;
    ShaderDataType type{ShaderDataType::None};
    std::uint32_t size{0};
    std::size_t offset{0};
    bool normalized{false};

    BufferElement() = default;

    BufferElement(
        ShaderDataType elementType,
        std::string elementName,
        bool shouldNormalize = false
    );
};

class BufferLayout final
{
public:
    BufferLayout() = default;
    BufferLayout(std::initializer_list<BufferElement> elements);

    [[nodiscard]] const std::vector<BufferElement>& elements() const noexcept;
    [[nodiscard]] std::uint32_t stride() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    // Calculates each element's byte offset and the size of one whole vertex.
    // The size of one whole vertex is called the stride.
    void calculateOffsetsAndStride();

    std::vector<BufferElement> elements_;
    std::uint32_t stride_{0};
};

enum class BufferUsage : std::uint8_t
{
    // Static data is normally uploaded once. Dynamic data may be updated while
    // the application is running; this helps OpenGL choose suitable storage.
    Static,
    Dynamic
};

class VertexBuffer final
{
public:
    // A valid OpenGL context and loaded GLAD functions are required here.
    // This object owns its OpenGL buffer and deletes it in the destructor.
    VertexBuffer(
        const void* data,
        std::size_t sizeBytes,
        BufferUsage usage = BufferUsage::Static
    );

    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;

    void setData(
        const void* data,
        std::size_t sizeBytes,
        std::size_t offsetBytes = 0
    );

    void setLayout(BufferLayout layout);

    [[nodiscard]] const BufferLayout& layout() const noexcept;
    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] std::size_t sizeBytes() const noexcept;

private:
    GLuint bufferId_{0};
    std::size_t sizeBytes_{0};
    BufferLayout layout_;
};

class IndexBuffer final
{
public:
    // Indices select vertices by number so several triangles can reuse the
    // same vertex. This implementation stores 32-bit unsigned indices.
    IndexBuffer(
        const std::uint32_t* indices,
        std::uint32_t count,
        BufferUsage usage = BufferUsage::Static
    );

    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;

    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] std::uint32_t count() const noexcept;

private:
    GLuint bufferId_{0};
    std::uint32_t count_{0};
};

}
