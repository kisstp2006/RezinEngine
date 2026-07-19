#include <Rezin/Assets/Model/Model.hpp>

#include <Rezin/Assets/Mesh/Mesh.hpp>
#include <Rezin/Assets/Texture/Texture.hpp>
#include <Rezin/Graphics/ShaderProgram.hpp>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

[[nodiscard]] std::filesystem::path textureCacheKey(
    const std::filesystem::path& path
)
{
    return path.lexically_normal();
}

struct CachedModelTexture
{
    std::filesystem::path path;
    std::unique_ptr<rezin::Texture2D> texture;
};

struct CachedEmbeddedModelTexture
{
    std::uint32_t index{rezin::noEmbeddedModelTexture};
    std::unique_ptr<rezin::Texture2D> texture;
};

[[nodiscard]] const rezin::Texture2D* loadCachedTexture(
    const std::filesystem::path& path,
    const rezin::TextureSpecification& specification,
    std::vector<CachedModelTexture>& cache
)
{
    if (path.empty())
        throw std::invalid_argument("Model texture path must not be empty.");

    const std::filesystem::path key = textureCacheKey(path);
    for (const CachedModelTexture& cached : cache)
    {
        if (cached.path == key)
            return cached.texture.get();
    }

    auto texture = std::make_unique<rezin::Texture2D>(
        path,
        specification
    );

    const rezin::Texture2D* texturePointer = texture.get();
    cache.push_back({key, std::move(texture)});
    return texturePointer;
}

[[nodiscard]] const rezin::Texture2D* loadCachedEmbeddedTexture(
    std::uint32_t index,
    const rezin::ModelEmbeddedTextureData& embeddedTexture,
    const rezin::TextureSpecification& specification,
    std::vector<CachedEmbeddedModelTexture>& cache
)
{
    for (const CachedEmbeddedModelTexture& cached : cache)
    {
        if (cached.index == index)
            return cached.texture.get();
    }

    auto texture = std::make_unique<rezin::Texture2D>(
        embeddedTexture.encodedImage,
        std::filesystem::path(embeddedTexture.name),
        specification
    );

    const rezin::Texture2D* texturePointer = texture.get();
    cache.push_back({index, std::move(texture)});
    return texturePointer;
}

[[nodiscard]] const std::filesystem::path& selectTexturePath(
    const std::filesystem::path& importedPath,
    const std::filesystem::path& fallbackPath,
    const char* textureType,
    const std::string& materialName
)
{
    if (!importedPath.empty())
        return importedPath;

    if (!fallbackPath.empty())
        return fallbackPath;

    throw std::runtime_error(
        "Material '" + materialName + "' has no " + textureType
        + " texture and no fallback was configured."
    );
}

}

namespace rezin
{

ModelAsset::ModelAsset(
    std::filesystem::path sourcePath,
    std::vector<ModelMeshData> meshes,
    std::vector<ModelMaterialData> materials,
    std::vector<ModelEmbeddedTextureData> embeddedTextures
)
    : sourcePath_(std::move(sourcePath)),
      meshes_(std::move(meshes)),
      materials_(std::move(materials)),
      embeddedTextures_(std::move(embeddedTextures))
{
    if (meshes_.empty())
    {
        throw std::invalid_argument(
            "ModelAsset requires at least one mesh."
        );
    }

    if (materials_.empty())
    {
        ModelMaterialData defaultMaterial;
        defaultMaterial.name = "Default Material";
        materials_.push_back(std::move(defaultMaterial));
    }

    for (const ModelMaterialData& material : materials_)
    {
        if (
            material.diffuseEmbeddedTexture != noEmbeddedModelTexture
            && material.diffuseEmbeddedTexture >= embeddedTextures_.size()
        )
        {
            throw std::out_of_range(
                "Model material references an invalid embedded diffuse texture."
            );
        }

        if (
            material.specularEmbeddedTexture != noEmbeddedModelTexture
            && material.specularEmbeddedTexture >= embeddedTextures_.size()
        )
        {
            throw std::out_of_range(
                "Model material references an invalid embedded specular texture."
            );
        }

        if (
            material.normalEmbeddedTexture != noEmbeddedModelTexture
            && material.normalEmbeddedTexture >= embeddedTextures_.size()
        )
        {
            throw std::out_of_range(
                "Model material references an invalid embedded normal texture."
            );
        }
    }

    for (std::size_t meshIndex = 0; meshIndex < meshes_.size(); ++meshIndex)
    {
        const ModelMeshData& currentMesh = meshes_[meshIndex];
        const std::string meshDescription = currentMesh.name.empty()
            ? "mesh #" + std::to_string(meshIndex)
            : "mesh '" + currentMesh.name + "'";

        if (currentMesh.materialSlot >= materials_.size())
        {
            throw std::out_of_range(
                "ModelAsset " + meshDescription
                + " references a material outside the material array."
            );
        }

        if (currentMesh.vertices.empty())
        {
            throw std::invalid_argument(
                "ModelAsset " + meshDescription + " has no vertices."
            );
        }

        if (currentMesh.indices.empty())
        {
            throw std::invalid_argument(
                "ModelAsset " + meshDescription + " has no indices."
            );
        }

        if (currentMesh.indices.size() % 3 != 0)
        {
            throw std::invalid_argument(
                "ModelAsset " + meshDescription
                + " does not contain complete triangle index groups."
            );
        }

        for (const std::uint32_t vertexIndex : currentMesh.indices)
        {
            if (vertexIndex >= currentMesh.vertices.size())
            {
                throw std::out_of_range(
                    "ModelAsset " + meshDescription
                    + " contains an index outside its vertex array."
                );
            }
        }

        vertexCount_ += currentMesh.vertices.size();
        indexCount_ += currentMesh.indices.size();
    }
}

ModelAsset::ModelAsset(ModelAsset&& other) noexcept
    : sourcePath_(std::move(other.sourcePath_)),
      meshes_(std::move(other.meshes_)),
      materials_(std::move(other.materials_)),
      embeddedTextures_(std::move(other.embeddedTextures_)),
      vertexCount_(std::exchange(other.vertexCount_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0))
{
}

ModelAsset& ModelAsset::operator=(ModelAsset&& other) noexcept
{
    if (this == &other)
        return *this;

    sourcePath_ = std::move(other.sourcePath_);
    meshes_ = std::move(other.meshes_);
    materials_ = std::move(other.materials_);
    embeddedTextures_ = std::move(other.embeddedTextures_);
    vertexCount_ = std::exchange(other.vertexCount_, 0);
    indexCount_ = std::exchange(other.indexCount_, 0);
    return *this;
}

const std::filesystem::path& ModelAsset::path() const noexcept
{
    return sourcePath_;
}

std::span<const ModelMeshData> ModelAsset::meshes() const noexcept
{
    return std::span<const ModelMeshData>(meshes_);
}

std::span<const ModelMaterialData> ModelAsset::materials() const noexcept
{
    return std::span<const ModelMaterialData>(materials_);
}

std::span<const ModelEmbeddedTextureData>
ModelAsset::embeddedTextures() const noexcept
{
    return std::span<const ModelEmbeddedTextureData>(embeddedTextures_);
}

const ModelMeshData& ModelAsset::mesh(std::size_t index) const
{
    if (index >= meshes_.size())
    {
        throw std::out_of_range(
            "ModelAsset mesh index is outside the available mesh range."
        );
    }

    return meshes_[index];
}

const ModelMaterialData& ModelAsset::material(std::size_t index) const
{
    if (index >= materials_.size())
    {
        throw std::out_of_range(
            "ModelAsset material index is outside the available material range."
        );
    }

    return materials_[index];
}

const ModelEmbeddedTextureData& ModelAsset::embeddedTexture(
    std::size_t index
) const
{
    if (index >= embeddedTextures_.size())
    {
        throw std::out_of_range(
            "ModelAsset embedded texture index is outside the available range."
        );
    }

    return embeddedTextures_[index];
}

std::size_t ModelAsset::meshCount() const noexcept
{
    return meshes_.size();
}

std::size_t ModelAsset::materialCount() const noexcept
{
    return materials_.size();
}

std::size_t ModelAsset::embeddedTextureCount() const noexcept
{
    return embeddedTextures_.size();
}

std::size_t ModelAsset::vertexCount() const noexcept
{
    return vertexCount_;
}

std::size_t ModelAsset::indexCount() const noexcept
{
    return indexCount_;
}

std::size_t ModelAsset::triangleCount() const noexcept
{
    return indexCount_ / 3;
}

bool ModelAsset::empty() const noexcept
{
    return meshes_.empty();
}

struct Model::Implementation
{
    struct RuntimeMaterial
    {
        const Texture2D* diffuseTexture{nullptr};
        const Texture2D* specularTexture{nullptr};
        const Texture2D* normalTexture{nullptr};
        float shininess{32.0f};
    };

    Implementation(
        ModelAsset sourceAsset,
        ModelRenderSpecification renderSpecification
    )
        : asset(std::move(sourceAsset)),
          specification(std::move(renderSpecification))
    {
        if (
            specification.diffuseTextureSlot
            == specification.specularTextureSlot
            || specification.diffuseTextureSlot
            == specification.normalTextureSlot
            || specification.specularTextureSlot
            == specification.normalTextureSlot
        )
        {
            throw std::invalid_argument(
                "Diffuse, specular, and normal textures must use different slots."
            );
        }

        GLint maximumTextureUnits = 0;
        glGetIntegerv(
            GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
            &maximumTextureUnits
        );

        if (maximumTextureUnits <= 0
            || specification.diffuseTextureSlot
                >= static_cast<std::uint32_t>(maximumTextureUnits)
            || specification.specularTextureSlot
                >= static_cast<std::uint32_t>(maximumTextureUnits)
            || specification.normalTextureSlot
                >= static_cast<std::uint32_t>(maximumTextureUnits))
        {
            throw std::out_of_range(
                "Model texture slots exceed the texture units supported "
                "by the current OpenGL context."
            );
        }

        if (!std::isfinite(specification.defaultShininess)
            || specification.defaultShininess <= 0.0f)
        {
            throw std::invalid_argument(
                "Default model shininess must be a positive finite number."
            );
        }

        TextureSpecification diffuseSpecification;
        diffuseSpecification.flipVertically =
            specification.flipTexturesVertically;
        diffuseSpecification.srgb =
            specification.diffuseTexturesSrgb;

        TextureSpecification specularSpecification;
        specularSpecification.flipVertically =
            specification.flipTexturesVertically;
        specularSpecification.srgb = false;

        TextureSpecification normalSpecification;
        normalSpecification.flipVertically =
            specification.flipTexturesVertically;
        normalSpecification.srgb = false;

        materials.reserve(asset.materialCount());

        for (const ModelMaterialData& materialData : asset.materials())
        {
            const std::string materialName = materialData.name.empty()
                ? "Unnamed Material"
                : materialData.name;

            RuntimeMaterial runtimeMaterial;

            if (
                materialData.diffuseEmbeddedTexture
                != noEmbeddedModelTexture
            )
            {
                runtimeMaterial.diffuseTexture =
                    loadCachedEmbeddedTexture(
                        materialData.diffuseEmbeddedTexture,
                        asset.embeddedTexture(
                            materialData.diffuseEmbeddedTexture
                        ),
                        diffuseSpecification,
                        diffuseEmbeddedTextureCache
                    );
            }
            else
            {
                const std::filesystem::path& diffusePath =
                    selectTexturePath(
                        materialData.diffuseTexturePath,
                        specification.fallbackDiffuseTexture,
                        "diffuse",
                        materialName
                    );

                runtimeMaterial.diffuseTexture = loadCachedTexture(
                    diffusePath,
                    diffuseSpecification,
                    diffuseTextureCache
                );
            }

            if (
                materialData.specularEmbeddedTexture
                != noEmbeddedModelTexture
            )
            {
                runtimeMaterial.specularTexture =
                    loadCachedEmbeddedTexture(
                        materialData.specularEmbeddedTexture,
                        asset.embeddedTexture(
                            materialData.specularEmbeddedTexture
                        ),
                        specularSpecification,
                        specularEmbeddedTextureCache
                    );
            }
            else
            {
                const std::filesystem::path& specularPath =
                    selectTexturePath(
                        materialData.specularTexturePath,
                        specification.fallbackSpecularTexture,
                        "specular",
                        materialName
                    );

                runtimeMaterial.specularTexture = loadCachedTexture(
                    specularPath,
                    specularSpecification,
                    specularTextureCache
                );
            }

            if (
                materialData.normalEmbeddedTexture
                != noEmbeddedModelTexture
            )
            {
                runtimeMaterial.normalTexture =
                    loadCachedEmbeddedTexture(
                        materialData.normalEmbeddedTexture,
                        asset.embeddedTexture(
                            materialData.normalEmbeddedTexture
                        ),
                        normalSpecification,
                        normalEmbeddedTextureCache
                    );
            }
            else
            {
                const std::filesystem::path& normalPath =
                    selectTexturePath(
                        materialData.normalTexturePath,
                        specification.fallbackNormalTexture,
                        "normal",
                        materialName
                    );

                runtimeMaterial.normalTexture = loadCachedTexture(
                    normalPath,
                    normalSpecification,
                    normalTextureCache
                );
            }

            runtimeMaterial.shininess =
                std::isfinite(materialData.shininess)
                && materialData.shininess > 0.0f
                    ? materialData.shininess
                    : specification.defaultShininess;

            materials.push_back(std::move(runtimeMaterial));
        }

        meshes.reserve(asset.meshCount());
        for (const ModelMeshData& meshData : asset.meshes())
            meshes.emplace_back(meshData);
    }

    ModelAsset asset;
    ModelRenderSpecification specification;
    std::vector<CachedModelTexture> diffuseTextureCache;
    std::vector<CachedModelTexture> specularTextureCache;
    std::vector<CachedModelTexture> normalTextureCache;
    std::vector<CachedEmbeddedModelTexture> diffuseEmbeddedTextureCache;
    std::vector<CachedEmbeddedModelTexture> specularEmbeddedTextureCache;
    std::vector<CachedEmbeddedModelTexture> normalEmbeddedTextureCache;
    std::vector<RuntimeMaterial> materials;
    std::vector<Mesh> meshes;
};

Model::Model(
    ModelAsset asset,
    ModelRenderSpecification specification
)
    : implementation_(std::make_unique<Implementation>(
          std::move(asset),
          std::move(specification)
      ))
{
}

Model::~Model() = default;

Model::Model(Model&& other) noexcept = default;

Model& Model::operator=(Model&& other) noexcept = default;

void Model::draw(const ShaderProgram& shader) const
{
    if (!implementation_)
        throw std::logic_error("Cannot draw a moved-from Model.");

    shader.use();
    shader.setInt(
        "material.diffuse",
        static_cast<int>(
            implementation_->specification.diffuseTextureSlot
        )
    );
    shader.setInt(
        "material.specular",
        static_cast<int>(
            implementation_->specification.specularTextureSlot
        )
    );
    shader.setInt(
        "material.normalMap",
        static_cast<int>(
            implementation_->specification.normalTextureSlot
        )
    );

    for (const Mesh& mesh : implementation_->meshes)
    {
        const Implementation::RuntimeMaterial& material =
            implementation_->materials.at(mesh.materialSlot());

        material.diffuseTexture->bind(
            implementation_->specification.diffuseTextureSlot
        );
        material.specularTexture->bind(
            implementation_->specification.specularTextureSlot
        );
        material.normalTexture->bind(
            implementation_->specification.normalTextureSlot
        );
        shader.setFloat("material.shininess", material.shininess);
        mesh.draw();
    }
}

void Model::drawGeometry() const
{
    if (!implementation_)
        throw std::logic_error("Cannot draw a moved-from Model.");

    for (const Mesh& mesh : implementation_->meshes)
        mesh.draw();
}

const ModelAsset& Model::asset() const
{
    if (!implementation_)
        throw std::logic_error("Cannot access a moved-from Model.");

    return implementation_->asset;
}

std::size_t Model::meshCount() const noexcept
{
    return implementation_ ? implementation_->meshes.size() : 0;
}

}
