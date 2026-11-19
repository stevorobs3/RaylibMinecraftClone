#include "block_shader.h"

namespace {

// UV2 = (-1, -1) means the face has no overlay.
// Vertex colour is the base tint without UV2; otherwise the
// overlay tint. Both layers sample the same atlas texture.
constexpr const char* vertex_source = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec2 vertexTexCoord2;
in vec4 vertexColor;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec2 fragOverlayCoord;
out vec4 fragTint;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragOverlayCoord = vertexTexCoord2;
    fragTint = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

constexpr const char* fragment_source = R"(#version 330
in vec2 fragTexCoord;
in vec2 fragOverlayCoord;
in vec4 fragTint;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float overlayAlphaCutoff;

out vec4 finalColor;

void main()
{
    vec4 base = texture(texture0, fragTexCoord);

    if (fragOverlayCoord.x >= 0.0)
    {
        vec4 overlay = texture(texture0, fragOverlayCoord);

        // Grass uses a binary alpha cutout, not blending.
        // A present overlay pixel entirely replaces the base.
        vec3 rgb = overlay.a >= overlayAlphaCutoff
            ? overlay.rgb * fragTint.rgb
            : base.rgb;

        finalColor = vec4(rgb, base.a) * colDiffuse;
    }
    else
    {
        finalColor = base * fragTint * colDiffuse;
    }
}
)";

} // namespace

bool BlockShader::load() {
  Shader loaded = LoadShaderFromMemory(
    vertex_source, fragment_source
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
