#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout (std140) uniform Matrices {
    mat4 projection;
    mat4 view;
};

out vec3 vcolor;

void main()
{
    gl_Position = projection * view * vec4(position, 1.0f);

    vcolor = color;
}