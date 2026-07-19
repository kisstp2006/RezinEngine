#include <Rezin/Assets/Mesh/Mesh.hpp>

#include <glad/glad.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace
{
    // Our current BufferLayout calculates offsets by adding the attribute sizes.
    // These checks guarantee that ModelVertex really uses the expected tightly
    // packed 12-float representation.
    static_assert(std::is_standard_layout_v<rezin::ModelVertex>);

    static_assert(
        offsetof(rezin::ModelVertex, position) == 0
    );

    static_assert(
        offsetof(rezin::ModelVertex, normal)
        == sizeof(float) * 3
    );

    static_assert(
        offsetof(rezin::ModelVertex, textureCoordinates)
        == sizeof(float) * 6
    );

    static_assert(offsetof(rezin::ModelVertex, tangent) == sizeof(float) * 8);
    static_assert(sizeof(rezin::ModelVertex) == sizeof(float) * 12);

    std::size_t checkedVertexBufferSize(
        const rezin::ModelMeshData& meshData
    )
    {
        if (meshData.vertices.empty())
        {
            throw std::invalid_argument(
                "Cannot create a Mesh without vertices."
            );
        }

        if (
            meshData.vertices.size()
            > std::numeric_limits<std::size_t>::max()
                / sizeof(rezin::ModelVertex)
        )
        {
            throw std::length_error(
                "Mesh vertex data is too large."
            );
        }

        return meshData.vertices.size()
            * sizeof(rezin::ModelVertex);
    }

    std::uint32_t checkedIndexCount(
        const rezin::ModelMeshData& meshData
    )
    {
        if (meshData.indices.empty())
        {
            throw std::invalid_argument(
                "Cannot create a Mesh without indices."
            );
        }

        if (meshData.indices.size() % 3 != 0)
        {
            throw std::invalid_argument(
                "Mesh indices must contain complete triangle groups."
            );
        }

        if (
            meshData.indices.size()
            > static_cast<std::size_t>(
                std::numeric_limits<GLsizei>::max()
            )
        )
        {
            throw std::length_error(
                "Mesh has more indices than one OpenGL draw call supports."
            );
        }

        for (const std::uint32_t vertexIndex : meshData.indices)
        {
            if (vertexIndex >= meshData.vertices.size())
            {
                throw std::out_of_range(
                    "Mesh index points outside the vertex array."
                );
            }
        }

        return static_cast<std::uint32_t>(
            meshData.indices.size()
        );
    }



}

namespace rezin
{
        Mesh::Mesh(const ModelMeshData& meshData)
        : vertexBuffer_(
              meshData.vertices.data(),
              checkedVertexBufferSize(meshData),
              BufferUsage::Static
          ),
          indexBuffer_(
              meshData.indices.data(),
              checkedIndexCount(meshData),
              BufferUsage::Static
          ),
          vertexArray_(),
          name_(meshData.name),
          materialSlot_(meshData.materialSlot)
    {
        // The list order becomes shader locations 0, 1, 2, and 3.
        vertexBuffer_.setLayout({
            {ShaderDataType::Float3, "aPosition"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTextureCoordinates"},
            {ShaderDataType::Float4, "aTangent"}
        });

        vertexArray_.addVertexBuffer(vertexBuffer_);
        vertexArray_.setIndexBuffer(indexBuffer_);
    }
    void Mesh::draw() const noexcept
    {
        vertexArray_.bind();

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indexBuffer_.count()),
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    const std::string& Mesh::name() const noexcept
    {
        return name_;
    }

    std::uint32_t Mesh::indexCount() const noexcept
    {
        return indexBuffer_.count();
    }

    std::uint32_t Mesh::materialSlot() const noexcept
    {
        return materialSlot_;
    }

}
