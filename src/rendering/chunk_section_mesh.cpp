#include "chunk_section_mesh.h"

bool ChunkSectionMesh::replace(const std::vector<MeshData>& data,Texture2D texture,
                               std::uint64_t generation) {
  std::vector<OwnedModel> replacement;
  replacement.reserve(data.size());
  for (const MeshData& part : data) {
    auto mesh = MeshGenerator::upload(part);
    if(!mesh) return false;
    // LoadModelFromMesh transfers ownership of the mesh buffers.
    Model model=LoadModelFromMesh(*mesh);
    if (!model.meshes || !model.materials || !model.meshMaterial ||
       !model.materials[0].maps) {
      // Avoid UnloadModel on a partially allocated Model (it assumes
      // its materials array is usable whenever materialCount > 0).
      if(model.meshes) {
        UnloadMesh(model.meshes[0]);
        MemFree(model.meshes);
      } else {
        UnloadMesh(*mesh);
      }
      if(model.materials) {
        if(model.materials[0].maps) MemFree(model.materials[0].maps);
        MemFree(model.materials);
      }
      MemFree(model.meshMaterial);
      return false;
    }
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture;
    replacement.emplace_back(model);
  }
  models_.swap(replacement); // Old GPU objects released after successful replacement.
  generation_=generation;
  return true;
}
