#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main()
{
    // Return red
    outFragColor = vec4(inColor,1.0f);
}