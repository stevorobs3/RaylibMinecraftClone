#include "block_shader.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

bool BlockShader::load() {
  const fs::path directory =
    fs::path(GetApplicationDirectory()) / "shaders";

  const std::string vertex = (directory / "block.vs").string();
  const std::string fragment = (directory / "block.fs").string();

  if (!FileExists(vertex.c_str()) ||
      !FileExists(fragment.c_str())) {
    TraceLog(LOG_ERROR, "Block shader files are missing");
    return false;
  }

  Shader loaded = LoadShader(
    vertex.c_str(), fragment.c_str()
  );

  // A compilation failure may return raylib's default shader.
  // Its missing UV2 attribute is a useful validity check.
  if (!IsShaderValid(loaded) ||
      GetShaderLocationAttrib(loaded, "vertexTexCoord2") < 0 ||
      GetShaderLocation(loaded, "texture0") < 0 ||
      GetShaderLocation(loaded, "mvp") < 0 ||
      GetShaderLocation(loaded, "overlayAlphaCutoff") < 0) {
    if (loaded.locs)
      UnloadShader(loaded);

    TraceLog(LOG_ERROR, "Layered block shader failed to load");
    return false;
  }

  const int cutoff_location = GetShaderLocation(
    loaded, "overlayAlphaCutoff"
  );
  const float cutoff = BlockOverlay::alpha_cutoff;

  SetShaderValue(
    loaded, cutoff_location, &cutoff, SHADER_UNIFORM_FLOAT
  );

  unload();
  shader_ = loaded;
  return true;
}

void BlockShader::unload() noexcept {
  if (shader_.locs)
    UnloadShader(shader_);

  shader_ = {};
}
