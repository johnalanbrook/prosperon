#version 330
layout (location = 0) in vec2 pos;
out vec2 apos;

uniform vec2 offset;

layout (std140) uniform Projection {
    mat4 projection;
};

void main()
{
    vec4 ipos = inverse(projection) * vec4(pos, 0.f, 1.f);
    apos = ipos.xy + offset;

    gl_Position = vec4(pos, 0.f, 1.f);
}
