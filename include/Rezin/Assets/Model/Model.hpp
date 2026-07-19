#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace rezin
{

inline constexpr std::uint32_t noEmbeddedModelTexture =
    std::numeric_limits<std::uint32_t>::max();

// ModelVertex is the engine-owned vertex format produced by a future importer.
// Keeping it independent from Assimp prevents third-party types from leaking
// into rendering, ECS components, or saved engine data.
struct ModelVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec2 textureCoordinates{0.0f};
    glm::vec4 tangent{0.0f};
};

// This is the renderer-independent part of a classic material. Empty texture
// paths mean that the renderable Model should use its configured fallback.
// Keeping paths here avoids creating OpenGL textures during file importing.
struct ModelMaterialData
{
    std::string name;
    std::filesystem::path diffuseTexturePath;
    std::filesystem::path specularTexturePath;
    std::uint32_t diffuseEmbeddedTexture{noEmbeddedModelTexture};
    std::uint32_t specularEmbeddedTexture{noEmbeddedModelTexture};
    std::uint32_t normalEmbeddedTexture{noEmbeddedModelTexture};
    float shininess{32.0f};
    std::filesystem::path normalTexturePath;
};

// Compressed PNG/JPG-style image bytes copied from the imported model. The
// bytes remain renderer-independent until Model creates a Texture2D from them.
struct ModelEmbeddedTextureData
{
    std::string name;
    std::vector<std::uint8_t> encodedImage;
};

// One imported model may contain several independently drawable meshes. This
// structure stores the CPU-side data for one of those meshes. GPU buffers will
// be created by the mesh/rendering layer instead of by ModelAsset itself.
struct ModelMeshData
{
    std::string name;
    std::vector<ModelVertex> vertices;
    std::vector<std::uint32_t> indices;

    // Index of the material used by this mesh inside ModelAsset::materials().
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
        std::vector<ModelMeshData> meshes,
        std::vector<ModelMaterialData> materials = {},
        std::vector<ModelEmbeddedTextureData> embeddedTextures = {}
    );

    ~ModelAsset() = default;

    ModelAsset(const ModelAsset&) = delete;
    ModelAsset& operator=(const ModelAsset&) = delete;

    ModelAsset(ModelAsset&& other) noexcept;
    ModelAsset& operator=(ModelAsset&& other) noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] std::span<const ModelMeshData> meshes() const noexcept;
    [[nodiscard]] std::span<const ModelMaterialData> materials() const noexcept;
    [[nodiscard]] std::span<const ModelEmbeddedTextureData>
        embeddedTextures() const noexcept;
    [[nodiscard]] const ModelMeshData& mesh(std::size_t index) const;
    [[nodiscard]] const ModelMaterialData& material(std::size_t index) const;
    [[nodiscard]] const ModelEmbeddedTextureData& embeddedTexture(
        std::size_t index
    ) const;

    [[nodiscard]] std::size_t meshCount() const noexcept;
    [[nodiscard]] std::size_t materialCount() const noexcept;
    [[nodiscard]] std::size_t embeddedTextureCount() const noexcept;
    [[nodiscard]] std::size_t vertexCount() const noexcept;
    [[nodiscard]] std::size_t indexCount() const noexcept;
    [[nodiscard]] std::size_t triangleCount() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    std::filesystem::path sourcePath_;
    std::vector<ModelMeshData> meshes_;
    std::vector<ModelMaterialData> materials_;
    std::vector<ModelEmbeddedTextureData> embeddedTextures_;
    std::size_t vertexCount_{0};
    std::size_t indexCount_{0};
};

class ShaderProgram;

// Controls how a CPU-side ModelAsset becomes a renderable OpenGL Model.
// The fallback textures are used when an imported material has no map of that
// type, which keeps all samplers valid for the current lighting shader.
struct ModelRenderSpecification
{
    std::filesystem::path fallbackDiffuseTexture{
        "assets/texture/missingTexture.png"
    };
    std::filesystem::path fallbackSpecularTexture{
        "assets/texture/missingTexture_specular.png"
    };
    std::filesystem::path fallbackNormalTexture{
        "assets/texture/missingTexture_normal.png"
    };

    std::uint32_t diffuseTextureSlot{0};
    std::uint32_t specularTextureSlot{1};
    std::uint32_t normalTextureSlot{2};
    float defaultShininess{32.0f};
    bool flipTexturesVertically{true};
    bool diffuseTexturesSrgb{false};
};

// Model is the GPU-side representation of a complete imported model. It owns
// one Mesh per imported mesh and shares duplicate material textures internally.
// Construct and destroy it only while a valid OpenGL context exists.
class Model final
{
public:
    explicit Model(
        ModelAsset asset,
        ModelRenderSpecification specification = {}
    );

    ~Model();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    // The shader must follow the current material uniform contract:
    // material.diffuse, material.specular, material.normalMap,
    // and material.shininess.
    void draw(const ShaderProgram& shader) const;

    // Draws only the imported mesh geometry. This is useful for simple shaders
    // such as light markers that do not expose the material uniform contract.
    void drawGeometry() const;

    [[nodiscard]] const ModelAsset& asset() const;
    [[nodiscard]] std::size_t meshCount() const noexcept;

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}
