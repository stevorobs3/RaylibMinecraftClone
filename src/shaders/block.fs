#version 330

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

        // Opaque overlay texels replace the base colour completely.
        // Alpha is a cutout mask, not a colour blending weight.
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
