#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

uniform mat4 world;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    TexCoords = inTexCoord;
    gl_Position = projection * world * vec4(inPosition, 1.0);
}