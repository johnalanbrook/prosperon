#version 330
in vec2 coords;

out vec4 color;

uniform float radius;
uniform int thickness;
uniform vec3 dbgColor;
uniform bool fill;
uniform float zoom;

void main()
{

    // int tt = thickness + 1;
    float R1 = 1.f;
    float R2 = 1.f - (thickness*zoom / radius);
    float dist = sqrt(dot(coords, coords));

    if (dist >= R2 && dist <= R1)
        color = vec4(dbgColor, 1.f);
    else if (dist < R2)
        color = vec4(dbgColor, 0.1f);
    else
        discard;
}
