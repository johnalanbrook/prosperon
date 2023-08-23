#version 330
layout (location = 0) in vec2 pos;

out vec2 apos;

layout (std140) uniform Projection {
    mat4 projection;
};

uniform vec2 offset;

void main()
{
//    vec4 ipos = inverse(projection) * vec4(pos, 0.f, 1.f);
    apos = pos * vec2(600, 360);
//    apos = pos + offset;


    gl_Position = vec4(pos, 0.f, 1.f);
}
