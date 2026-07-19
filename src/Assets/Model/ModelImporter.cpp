#include <Rezin/Assets/Model/ModelImporter.hpp>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

[[nodiscard]] unsigned int toAssimpFlags(
    rezin::ModelImportFlags flags
)
{
    using rezin::ModelImportFlags;
    using rezin::hasModelImportFlag;

    if (hasModelImportFlag(flags, ModelImportFlags::GenerateNormals)
        && hasModelImportFlag(flags, ModelImportFlags::GenerateSmoothNormals))
    {
        throw std::invalid_argument(
            "GenerateNormals and GenerateSmoothNormals cannot be enabled "
            "at the same time."
        );
    }

    unsigned int assimpFlags = 0;

    if (hasModelImportFlag(flags, ModelImportFlags::Triangulate))
        assimpFlags |= aiProcess_Triangulate;

    if (hasModelImportFlag(flags, ModelImportFlags::GenerateNormals))
        assimpFlags |= aiProcess_GenNormals;

    if (hasModelImportFlag(flags, ModelImportFlags::GenerateSmoothNormals))
        assimpFlags |= aiProcess_GenSmoothNormals;

    if (hasModelImportFlag(flags, ModelImportFlags::FlipUVs))
        assimpFlags |= aiProcess_FlipUVs;

    if (hasModelImportFlag(flags, ModelImportFlags::JoinIdenticalVertices))
        assimpFlags |= aiProcess_JoinIdenticalVertices;

    if (hasModelImportFlag(flags, ModelImportFlags::ImproveCacheLocality))
        assimpFlags |= aiProcess_ImproveCacheLocality;

    if (hasModelImportFlag(flags, ModelImportFlags::PreTransformVertices))
        assimpFlags |= aiProcess_PreTransformVertices;

    if (hasModelImportFlag(flags, ModelImportFlags::OptimizeMeshes))
        assimpFlags |= aiProcess_OptimizeMeshes;

    if (hasModelImportFlag(flags, ModelImportFlags::ValidateDataStructure))
        assimpFlags |= aiProcess_ValidateDataStructure;

    if (hasModelImportFlag(flags, ModelImportFlags::FindInvalidData))
        assimpFlags |= aiProcess_FindInvalidData;

    return assimpFlags;
}

[[nodiscard]] rezin::ModelMeshData processMesh(const aiMesh& mesh)
{
    static_assert(
        std::numeric_limits<unsigned int>::max()
            <= std::numeric_limits<std::uint32_t>::max(),
        "Assimp mesh indices do not fit into ModelMeshData indices."
    );

    if (mesh.mNumVertices == 0 || mesh.mVertices == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned a mesh without vertex positions."
        );
    }

    if (!mesh.HasNormals() || mesh.mNormals == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned a mesh without normals. Enable "
            "GenerateNormals or GenerateSmoothNormals when importing."
        );
    }

    rezin::ModelMeshData result;

    if (mesh.mName.length > 0)
        result.name = mesh.mName.C_Str();
    else
        result.name = "Unnamed Mesh";

    result.materialSlot = static_cast<std::uint32_t>(mesh.mMaterialIndex);
    result.vertices.reserve(mesh.mNumVertices);

    const bool hasTextureCoordinates =
        mesh.HasTextureCoords(0) && mesh.mTextureCoords[0] != nullptr;

    for (unsigned int vertexIndex = 0;
         vertexIndex < mesh.mNumVertices;
         ++vertexIndex)
    {
        rezin::ModelVertex vertex;

        vertex.position = {
            mesh.mVertices[vertexIndex].x,
            mesh.mVertices[vertexIndex].y,
            mesh.mVertices[vertexIndex].z
        };

        vertex.normal = {
            mesh.mNormals[vertexIndex].x,
            mesh.mNormals[vertexIndex].y,
            mesh.mNormals[vertexIndex].z
        };

        if (hasTextureCoordinates)
        {
            vertex.textureCoordinates = {
                mesh.mTextureCoords[0][vertexIndex].x,
                mesh.mTextureCoords[0][vertexIndex].y
            };
        }

        result.vertices.push_back(vertex);
    }

    if (mesh.mNumFaces == 0 || mesh.mFaces == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned a mesh without index faces: " + result.name
        );
    }

    result.indices.reserve(
        static_cast<std::size_t>(mesh.mNumFaces) * 3
    );

    for (unsigned int faceIndex = 0;
         faceIndex < mesh.mNumFaces;
         ++faceIndex)
    {
        const aiFace& face = mesh.mFaces[faceIndex];

        if (face.mNumIndices != 3 || face.mIndices == nullptr)
        {
            throw std::runtime_error(
                "Model mesh contains a non-triangle face. Enable the "
                "Triangulate import flag: " + result.name
            );
        }

        for (unsigned int index = 0; index < face.mNumIndices; ++index)
            result.indices.push_back(
                static_cast<std::uint32_t>(face.mIndices[index])
            );
    }

    return result;
}

struct ImportedTextureReference
{
    std::filesystem::path externalPath;
    std::uint32_t embeddedIndex{rezin::noEmbeddedModelTexture};
};

[[nodiscard]] std::uint32_t findEmbeddedTextureIndex(
    const aiScene& scene,
    const aiTexture& texture
)
{
    if (scene.mNumTextures == 0 || scene.mTextures == nullptr)
    {
        throw std::runtime_error(
            "Assimp resolved an embedded texture, but the scene texture "
            "array is empty."
        );
    }

    for (unsigned int index = 0; index < scene.mNumTextures; ++index)
    {
        if (scene.mTextures[index] == &texture)
            return static_cast<std::uint32_t>(index);
    }

    throw std::runtime_error(
        "Assimp resolved an embedded texture outside the scene texture array."
    );
}

[[nodiscard]] ImportedTextureReference readTextureReference(
    const aiMaterial& material,
    aiTextureType textureType,
    const aiScene& scene,
    const std::filesystem::path& modelDirectory,
    const std::string& materialName
)
{
    if (material.GetTextureCount(textureType) == 0)
        return {};

    aiString importedPath;
    if (
        material.GetTexture(textureType, 0, &importedPath)
        != AI_SUCCESS
    )
    {
        throw std::runtime_error(
            "Assimp could not read a texture path from material: "
            + materialName
        );
    }

    const std::string rawPath = importedPath.C_Str();
    if (rawPath.empty())
        return {};

    if (const aiTexture* embedded =
            scene.GetEmbeddedTexture(rawPath.c_str()))
    {
        ImportedTextureReference result;
        result.embeddedIndex = findEmbeddedTextureIndex(scene, *embedded);
        return result;
    }

    if (rawPath.front() == '*')
    {
        throw std::runtime_error(
            "Material '" + materialName
            + "' references an unknown embedded texture: " + rawPath
        );
    }

    ImportedTextureReference result;
    const std::filesystem::path texturePath(rawPath);
    if (texturePath.is_absolute())
        result.externalPath = texturePath.lexically_normal();
    else
        result.externalPath =
            (modelDirectory / texturePath).lexically_normal();

    return result;
}

[[nodiscard]] std::vector<rezin::ModelEmbeddedTextureData>
processEmbeddedTextures(const aiScene& scene)
{
    std::vector<rezin::ModelEmbeddedTextureData> textures;

    if (scene.mNumTextures == 0)
        return textures;

    if (scene.mTextures == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned an invalid embedded texture array."
        );
    }

    textures.reserve(scene.mNumTextures);

    for (unsigned int index = 0; index < scene.mNumTextures; ++index)
    {
        const aiTexture* texture = scene.mTextures[index];
        if (texture == nullptr || texture->pcData == nullptr)
        {
            throw std::runtime_error(
                "Assimp returned an invalid embedded texture."
            );
        }

        // mHeight == 0 means mWidth is the compressed PNG/JPG byte count.
        // Uncompressed aiTexel arrays need a separate raw-pixel upload path.
        if (texture->mHeight != 0)
        {
            throw std::runtime_error(
                "Uncompressed embedded model textures are not supported yet."
            );
        }

        if (texture->mWidth == 0)
        {
            throw std::runtime_error(
                "Assimp returned an empty embedded texture."
            );
        }

        rezin::ModelEmbeddedTextureData result;
        result.name = texture->mFilename.length > 0
            ? texture->mFilename.C_Str()
            : "Embedded Texture " + std::to_string(index);

        const auto* firstByte = reinterpret_cast<const std::uint8_t*>(
            texture->pcData
        );

        result.encodedImage.assign(
            firstByte,
            firstByte + texture->mWidth
        );

        textures.push_back(std::move(result));
    }

    return textures;
}

[[nodiscard]] std::vector<rezin::ModelMaterialData> processMaterials(
    const aiScene& scene,
    const std::filesystem::path& modelDirectory
)
{
    std::vector<rezin::ModelMaterialData> materials;

    if (scene.mNumMaterials == 0 || scene.mMaterials == nullptr)
    {
        rezin::ModelMaterialData defaultMaterial;
        defaultMaterial.name = "Default Material";
        materials.push_back(std::move(defaultMaterial));
        return materials;
    }

    materials.reserve(scene.mNumMaterials);

    for (unsigned int materialIndex = 0;
         materialIndex < scene.mNumMaterials;
         ++materialIndex)
    {
        const aiMaterial* material = scene.mMaterials[materialIndex];
        if (material == nullptr)
        {
            throw std::runtime_error(
                "Assimp returned a null material."
            );
        }

        rezin::ModelMaterialData result;

        aiString importedName;
        if (material->Get(AI_MATKEY_NAME, importedName) == AI_SUCCESS
            && importedName.length > 0)
        {
            result.name = importedName.C_Str();
        }
        else
        {
            result.name =
                "Material " + std::to_string(materialIndex);
        }

        float importedShininess = result.shininess;
        if (
            material->Get(
                AI_MATKEY_SHININESS,
                importedShininess
            ) == AI_SUCCESS
        )
        {
            result.shininess = importedShininess;
        }

        // The current lighting shader supports one diffuse and one specular
        // sampler per material, so only the first map of each type is imported.
        const ImportedTextureReference diffuseTexture =
            readTextureReference(
            *material,
            aiTextureType_DIFFUSE,
            scene,
            modelDirectory,
            result.name
        );

        result.diffuseTexturePath = diffuseTexture.externalPath;
        result.diffuseEmbeddedTexture = diffuseTexture.embeddedIndex;

        const ImportedTextureReference specularTexture =
            readTextureReference(
            *material,
            aiTextureType_SPECULAR,
            scene,
            modelDirectory,
            result.name
        );

        result.specularTexturePath = specularTexture.externalPath;
        result.specularEmbeddedTexture = specularTexture.embeddedIndex;

        materials.push_back(std::move(result));
    }

    return materials;
}

void processNode(
    const aiNode& node,
    const aiScene& scene,
    std::vector<rezin::ModelMeshData>& meshes
)
{
    if (node.mNumMeshes > 0 && node.mMeshes == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned a node with an invalid mesh index array."
        );
    }

    for (unsigned int nodeMeshIndex = 0;
         nodeMeshIndex < node.mNumMeshes;
         ++nodeMeshIndex)
    {
        const unsigned int sceneMeshIndex = node.mMeshes[nodeMeshIndex];

        if (sceneMeshIndex >= scene.mNumMeshes
            || scene.mMeshes[sceneMeshIndex] == nullptr)
        {
            throw std::runtime_error(
                "Assimp returned a node with an invalid mesh reference."
            );
        }

        meshes.push_back(processMesh(*scene.mMeshes[sceneMeshIndex]));
    }

    if (node.mNumChildren > 0 && node.mChildren == nullptr)
    {
        throw std::runtime_error(
            "Assimp returned a node with an invalid child array."
        );
    }

    for (unsigned int childIndex = 0;
         childIndex < node.mNumChildren;
         ++childIndex)
    {
        if (node.mChildren[childIndex] == nullptr)
        {
            throw std::runtime_error(
                "Assimp returned a null child node."
            );
        }

        processNode(*node.mChildren[childIndex], scene, meshes);
    }
}

}

namespace rezin
{

ModelAsset ModelImporter::load(
    const std::filesystem::path& path,
    ModelImportFlags flags
)
{
    if (path.empty())
        throw std::invalid_argument("Model path must not be empty.");

    const std::filesystem::path normalizedPath = path.lexically_normal();

    if (!std::filesystem::exists(normalizedPath))
    {
        throw std::runtime_error(
            "Model file does not exist: " + normalizedPath.string()
        );
    }

    if (!std::filesystem::is_regular_file(normalizedPath))
    {
        throw std::runtime_error(
            "Model path is not a regular file: " + normalizedPath.string()
        );
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        normalizedPath.string(),
        toAssimpFlags(flags)
    );

    if (scene == nullptr
        || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0
        || scene->mRootNode == nullptr)
    {
        const std::string importerError = importer.GetErrorString();

        throw std::runtime_error(
            "Failed to import model: " + normalizedPath.string()
            + "\nAssimp error: "
            + (importerError.empty()
                ? std::string{"Unknown Assimp error."}
                : importerError)
        );
    }

    if (scene->mNumMeshes == 0 || scene->mMeshes == nullptr)
    {
        throw std::runtime_error(
            "Imported model does not contain any meshes: "
            + normalizedPath.string()
        );
    }

    std::vector<ModelMeshData> meshes;
    meshes.reserve(scene->mNumMeshes);
    processNode(*scene->mRootNode, *scene, meshes);

    if (meshes.empty())
    {
        throw std::runtime_error(
            "Imported model did not produce any engine meshes: "
            + normalizedPath.string()
        );
    }

    std::vector<ModelMaterialData> materials = processMaterials(
        *scene,
        normalizedPath.parent_path()
    );

    std::vector<ModelEmbeddedTextureData> embeddedTextures =
        processEmbeddedTextures(*scene);

    return ModelAsset(
        normalizedPath,
        std::move(meshes),
        std::move(materials),
        std::move(embeddedTextures)
    );
}

}
