#version 330
out vec4 frag_color;

in vec2 apos;
vec2 bpos;
uniform float thickness; /* thickness in pixels */
uniform float span;
uniform vec3 color;

void main(void)
{
    float t = thickness / span;
    t /= 2.0;
    bpos.x = mod(apos.x, span) / span;
    bpos.y = mod(apos.y, span) / span;
    bpos.x -= t;
    bpos.y -= t;

    float comp = min(bpos.x, bpos.y);

    if (comp > t)
      discard;

    comp += t;

    frag_color = vec4(color, 1.0);
}
