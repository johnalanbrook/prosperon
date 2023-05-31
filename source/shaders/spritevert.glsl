#version 330 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 vColor;
out vec2 texcoords;

uniform mat4 proj;
uniform mat4 mpv;

void main()
{
    texcoords = uv;
    gl_Position = mpv * proj * vec4(vertex, 0.0, 1.0);
}
