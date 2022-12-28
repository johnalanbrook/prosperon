#version 330 core
layout (location = 0) in vec2 vertex;
out vec2 texcoords;

layout (std140) uniform Projection
{
    mat4 projection;
};

uniform mat4 model;

void main()
{
    texcoords = vertex.xy;
    texcoords.y *= -1;
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
}