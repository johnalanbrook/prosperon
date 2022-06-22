#version 330 core

layout (location = 0) out vec4 color;

in vec3 vcolor;
in vec3 FragPos;

void main(void)
{
    color = vec4(vcolor, 1.f);
}