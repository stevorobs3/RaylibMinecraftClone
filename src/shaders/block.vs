#version 330

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
