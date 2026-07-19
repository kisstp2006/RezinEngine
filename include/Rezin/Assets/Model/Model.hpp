#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace rezin
{

// ModelVertex is the engine-owned vertex format produced by a future importer.
// Keeping it independent from Assimp prevents third-party types from leaking
// into rendering, ECS components, or saved engine data.
struct ModelVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec2 textureCoordinates{0.0f};
};

// One imported model may contain several independently drawable meshes. This
// structure stores the CPU-side data for one of those meshes. GPU buffers will
// be created by the mesh/rendering layer instead of by ModelAsset itself.
struct ModelMeshData
{
    std::string name;
    std::vector<ModelVertex> vertices;
    std::vector<std::uint32_t> indices;

    // This is only an opaque slot number for now. A later material system may
    // map it to a classic Phong material, a PBR material, or another pipeline
    // without changing the model's geometry representation.
    std::uint32_t materialSlot{0};
};

// ModelAsset is an immutable, shareable model resource. It owns imported CPU
// geometry but has no transform, entity, drawing function, OpenGL object, or
// Assimp dependency. A future ModelImporter will build this object, while a
// MeshRendererComponent will only keep a shared reference/asset handle to it.
class ModelAsset final
{
public:
    ModelAsset(
        std::filesystem::path sourcePath,
        std::vector<ModelMeshData> meshes
    );

    ~ModelAsset() = default;

    ModelAsset(const ModelAsset&) = delete;
    ModelAsset& operator=(const ModelAsset&) = delete;

    ModelAsset(ModelAsset&& other) noexcept;
    ModelAsset& operator=(ModelAsset&& other) noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] std::span<const ModelMeshData> meshes() const noexcept;
    [[nodiscard]] const ModelMeshData& mesh(std::size_t index) const;

    [[nodiscard]] std::size_t meshCount() const noexcept;
    [[nodiscard]] std::size_t vertexCount() const noexcept;
    [[nodiscard]] std::size_t indexCount() const noexcept;
    [[nodiscard]] std::size_t triangleCount() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::filesystem::path sourcePath_;
    std::vector<ModelMeshData> meshes_;
    std::size_t vertexCount_{0};
    std::size_t indexCount_{0};
};

}
