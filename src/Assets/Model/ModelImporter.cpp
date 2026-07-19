#include <Rezin/Assets/Model/ModelImporter.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>




namespace
{

void processNode(
    const aiNode& node,
    const aiScene& scene,
    std::vector<rezin::ModelMeshData>& meshes
);

rezin::ModelMeshData processMesh(
    const aiMesh& mesh
);

}


namespace rezin
{

ModelAsset ModelImporter::load(
    const std::filesystem::path& path
)
{
    const std::filesystem::path modelDirectory =
    path.parent_path();

    // Assimp loading will be implemented in the next section.
    throw std::runtime_error(
        "Model importing is not implemented yet: "
        + path.string()
    );
}

}
