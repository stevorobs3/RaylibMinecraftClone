#include "chunk_section_mesh.h"

bool ChunkSectionMesh::replace(const std::vector<MeshData>& data,Texture2D texture) {
  std::vector<OwnedModel> replacement;
  replacement.reserve(data.size());
  for (const MeshData& part : data) {
    auto mesh=MeshGenerator::upload(part);
    if(!mesh) return false;
    // LoadModelFromMesh transfers ownership of the mesh buffers.
    Model model=LoadModelFromMesh(*mesh);
    if(!model.meshes || !model.materials || !model.meshMaterial ||
       !model.materials[0].maps) {
      if(model.meshes) UnloadModel(model);
      else {
        // raylib could not allocate the Model mesh array; it never took ownership.
        UnloadMesh(*mesh);
        if(model.materials) {
          if(model.materials[0].maps) MemFree(model.materials[0].maps);
          MemFree(model.materials);
        }
        MemFree(model.meshMaterial);
      }
      return false;
    }
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture;
    replacement.emplace_back(model);
  }
  models_.swap(replacement); // Old GPU objects released after successful replacement.
  return true;
}
