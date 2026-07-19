#pragma once

#include <Rezin/Assets/Model/Model.hpp>

#include <cstdint>
#include <filesystem>

namespace rezin
{

// Engine-owned import options. Keeping Assimp constants out of this public
// header lets the asset API remain stable if the importer changes later.
enum class ModelImportFlags : std::uint32_t
{
    None                    = 0,
    Triangulate             = 1u << 0,
    GenerateNormals         = 1u << 1,
    GenerateSmoothNormals   = 1u << 2,
    FlipUVs                 = 1u << 3,
    JoinIdenticalVertices   = 1u << 4,
    ImproveCacheLocality    = 1u << 5,
    PreTransformVertices    = 1u << 6,
    OptimizeMeshes          = 1u << 7,
    ValidateDataStructure   = 1u << 8,
    FindInvalidData         = 1u << 9
};

[[nodiscard]] constexpr ModelImportFlags operator|(
    ModelImportFlags left,
    ModelImportFlags right
) noexcept
{
    return static_cast<ModelImportFlags>(
        static_cast<std::uint32_t>(left)
        | static_cast<std::uint32_t>(right)
    );
}

[[nodiscard]] constexpr ModelImportFlags operator&(
    ModelImportFlags left,
    ModelImportFlags right
) noexcept
{
    return static_cast<ModelImportFlags>(
        static_cast<std::uint32_t>(left)
        & static_cast<std::uint32_t>(right)
    );
}

[[nodiscard]] constexpr ModelImportFlags operator~(
    ModelImportFlags flags
) noexcept
{
    return static_cast<ModelImportFlags>(
        ~static_cast<std::uint32_t>(flags)
    );
}

constexpr ModelImportFlags& operator|=(
    ModelImportFlags& left,
    ModelImportFlags right
) noexcept
{
    left = left | right;
    return left;
}

constexpr ModelImportFlags& operator&=(
    ModelImportFlags& left,
    ModelImportFlags right
) noexcept
{
    left = left & right;
    return left;
}

[[nodiscard]] constexpr bool hasModelImportFlag(
    ModelImportFlags flags,
    ModelImportFlags flag
) noexcept
{
    return (flags & flag) != ModelImportFlags::None;
}

// These defaults produce render-ready triangle meshes while preserving the
// engine's current texture convention. FlipUVs remains an explicit choice.
inline constexpr ModelImportFlags defaultModelImportFlags =
    ModelImportFlags::Triangulate
    | ModelImportFlags::GenerateSmoothNormals
    | ModelImportFlags::JoinIdenticalVertices
    | ModelImportFlags::ImproveCacheLocality
    | ModelImportFlags::PreTransformVertices;

// ModelImporter is the public entry point for translating model files into the
// engine-owned ModelAsset format. Assimp types stay private to the .cpp file.
class ModelImporter final
{
public:
    [[nodiscard]] static ModelAsset load(
        const std::filesystem::path& path,
        ModelImportFlags flags = defaultModelImportFlags
    );

private:
    // The importer only exposes static operations and should not be created.
    ModelImporter() = delete;
};

}
