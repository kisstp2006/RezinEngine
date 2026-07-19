#pragma once

#include <Rezin/Assets/Model/Model.hpp>

#include <filesystem>

namespace rezin
{

// ModelImporter is the public entry point for translating model files into the
// engine-owned ModelAsset format. Assimp types stay private to the .cpp file.
class ModelImporter final
{
public:
    [[nodiscard]] static ModelAsset load(
        const std::filesystem::path& path
    );

private:
    // The importer only exposes static operations and should not be created.
    ModelImporter() = delete;
};

}
