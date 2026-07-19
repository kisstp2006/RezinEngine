#include <Rezin/Assets/Model/Model.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace rezin
{

ModelAsset::ModelAsset(
    std::filesystem::path sourcePath,
    std::vector<ModelMeshData> meshes
)
    : sourcePath_(std::move(sourcePath)),
      meshes_(std::move(meshes))
{
    if (meshes_.empty())
    {
        throw std::invalid_argument(
            "ModelAsset requires at least one mesh."
        );
    }

    for (std::size_t meshIndex = 0; meshIndex < meshes_.size(); ++meshIndex)
    {
        const ModelMeshData& currentMesh = meshes_[meshIndex];
        const std::string meshDescription = currentMesh.name.empty()
            ? "mesh #" + std::to_string(meshIndex)
            : "mesh '" + currentMesh.name + "'";

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

std::size_t ModelAsset::meshCount() const noexcept
{
    return meshes_.size();
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

}
