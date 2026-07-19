#pragma once

#include <Rezin/Assets/Model/Model.hpp>
#include <Rezin/Graphics/Buffer.hpp>
#include <Rezin/Graphics/VertexArray.hpp>

#include <cstdint>
#include <string>


namespace rezin
{

// Mesh is the GPU representation of one ModelMeshData object.
//
// ModelMeshData owns the imported CPU geometry. Mesh uploads that geometry to
// OpenGL and owns the resulting buffers. It intentionally does not know about
// entities, transforms, shaders, textures, or materials.

class Mesh final
{
    public:
        explicit Mesh(const ModelMeshData& meshData);
        ~Mesh() = default;

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&&) noexcept = default;
        Mesh& operator=(Mesh&&) noexcept = default;

        // Binds this mesh and submits its indexed triangles. Shader, transform, and
        // material setup must happen before this call in the render system.
        void draw() const noexcept;

        [[nodiscard]] const std::string& name() const noexcept;
        [[nodiscard]] std::uint32_t indexCount() const noexcept;
        [[nodiscard]] std::uint32_t materialSlot() const noexcept;

    private:
        // Declaration order is intentional. C++ destroys members in reverse order,
        // so VertexArray is destroyed before the buffers referenced by that VAO.
        VertexBuffer vertexBuffer_;
        IndexBuffer indexBuffer_;
        VertexArray vertexArray_;

        std::string name_;
        std::uint32_t materialSlot_{0};

};



}
