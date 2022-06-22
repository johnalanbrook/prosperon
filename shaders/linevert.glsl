#version 330
layout (location = 0) in vec2 pos;

layout (std140) uniform Projection {
    mat4 projection;
};

void main()
{
    gl_Position = projection * vec4(pos, 0.f, 1.f);
}