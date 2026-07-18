#pragma once

#include <Rezin/Graphics/Buffer.hpp>

#include <glad/glad.h>

#include <cstdint>

namespace rezin
{

// A VAO remembers how shader inputs read data from GPU buffers, but this C++
// class does not own the VertexBuffer or IndexBuffer objects. Keep those buffer
// objects alive for as long as this VertexArray is used for drawing.
class VertexArray final
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;

    void addVertexBuffer(const VertexBuffer& vertexBuffer);
    void setIndexBuffer(const IndexBuffer& indexBuffer);

    [[nodiscard]] GLuint id() const noexcept;

private:
    GLuint vertexArrayId_{0};
    std::uint32_t nextAttributeIndex_{0};
    std::uint32_t nextBindingIndex_{0};
};

}
