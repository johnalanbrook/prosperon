#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 texcoords;

uniform mat4 mpv;

void main()
{
    texcoords = vertex.zw;
    gl_Position = mpv * vec4(vertex.xy, 0.0, 1.0);
}
