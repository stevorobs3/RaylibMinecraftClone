#include "chunk_section_mesh.h"

bool ChunkSectionMesh::replace(const std::vector<MeshData>& data,Texture2D texture) {
  std::vector<OwnedModel> replacement;
  replacement.reserve(data.size());
  for (const MeshData& part : data) {
    auto mesh=MeshGenerator::upload(part);
    if(!mesh) return false;
    // LoadModelFromMesh transfers ownership of the mesh buffers.
    Model model=LoadModelFromMesh(*mesh);
    if(!IsModelValid(model)) {
      UnloadModel(model);
      return false;
    }
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture;
    replacement.emplace_back(model);
  }
  models_.swap(replacement); // Old GPU objects released after successful replacement.
  return true;
}
