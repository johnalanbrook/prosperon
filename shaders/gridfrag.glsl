#version 330
out vec4 color;

in vec2 apos;
vec2 bpos;
uniform int thickness;
uniform int span;

void main(void)
{
    float t = thickness / 2.f;
    bpos.x = mod(apos.x, span);
    bpos.y = mod(apos.y, span);

    if (!(bpos.x <= t || bpos.x >= (span - t) || bpos.y <= t || bpos.y >= (span - t)))
        discard;

    color = vec4(0.4f, 0.7f, 0.2f, 0.3f);
}