#version 330
layout (location = 0) in vec2 pos;
out vec2 apos;

layout (std140) uniform Projection {
    mat4 projection;
};

void main()
{
    mat4 iproj = inverse(projection);
    vec4 ipos = iproj * vec4(pos, 0.f, 1.f);
    apos = ipos.xy;

    gl_Position = vec4(pos, 0.f, 1.f);
}