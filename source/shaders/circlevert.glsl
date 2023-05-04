#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 coords;

uniform mat4 proj;
uniform vec2 res;

void main()
{
    gl_Position = proj * vec4(vertex.xy, 0.0, 1.0);
    coords = vertex.zw;
}
