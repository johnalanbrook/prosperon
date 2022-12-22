#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 coords;

layout (std140) uniform Projection
{
    mat4 projection;
};

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    coords = vertex.zw;
}