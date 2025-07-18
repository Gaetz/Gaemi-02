#version 450

layout (location = 0) out vec3 outColor;

void main()
{
    // Const array of positions for the triangle
    const vec3 positions[3] = vec3[3](
    vec3(1.f,1.f, 0.0f),
    vec3(-1.f,1.f, 0.0f),
    vec3(0.f,-1.f, 0.0f)
    );

    // Const array of colors for the triangle
    const vec3 colors[3] = vec3[3](
    vec3(1.0f, 0.0f, 0.0f), //red
    vec3(0.0f, 1.0f, 0.0f), //green
    vec3(00.f, 0.0f, 1.0f)  //blue
    );

    // Output the position of each vertex
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    outColor = colors[gl_VertexIndex];
}