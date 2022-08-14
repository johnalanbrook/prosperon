#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform Matrices {
    mat4 projection;
    mat4 view;
};

out vec3 vcolor;
uniform mat4 model;

void main()
{
    vec3 m = aPos;

    vcolor.r = m.r / 1.f;
    vcolor.g = m.g / 1.f;
    vcolor.b = m.b / 1.f;

    gl_Position = projection * view * model * (vec4(m, 1.f));
}  