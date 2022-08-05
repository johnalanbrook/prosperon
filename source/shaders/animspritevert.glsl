#version 330 core
layout (location = 0) in vec2 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

layout (std140) uniform Projection
{
    mat4 projection;
};

uniform mat4 model;

void main()
{
    TexCoords = vertex.xy;
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
}