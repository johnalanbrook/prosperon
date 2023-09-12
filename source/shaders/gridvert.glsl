#version 330
layout (location = 0) in vec2 pos;

out vec2 apos;

layout (std140) uniform Projection {
    mat4 projection;
};

uniform vec2 offset;
uniform vec2 dimen;

void main()
{
    apos = ((pos*0.5)*dimen) + offset;

    gl_Position = vec4(pos, 0.f, 1.f);
}
