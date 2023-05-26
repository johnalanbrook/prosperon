#version 330
layout (location = 0) in vec2 apos;
layout (location = 1) in float adist;
layout (location = 2) in vec4 acolor;
layout (location = 3) in float aseglen;
layout (location = 4) in float asegspeed;

out float dist;
out vec4 fcolor;
out float seg_len;
out float seg_speed;

uniform mat4 proj;

void main()
{
    gl_Position = proj * vec4(apos, 0.f, 1.f);
    fcolor = acolor;
    dist = adist;
    seg_len = aseglen;
    seg_speed = asegspeed;
}
