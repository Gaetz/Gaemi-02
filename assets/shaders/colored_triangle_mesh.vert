#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex
{
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

// Push constants block
layout( push_constant ) uniform constants
{
    mat4 render_matrix;
    VertexBuffer vertex_buffer;
} PushConstants;

void main()
{
    // Load vertex data from device adress
    Vertex v = PushConstants.vertex_buffer.vertices[gl_VertexIndex];

    // Output data
    gl_Position = PushConstants.render_matrix *vec4(v.position, 1.0f);
    outColor = v.color.xyz;
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}