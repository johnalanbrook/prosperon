#version 330
in vec2 coords;

out vec4 color;

uniform float radius;
uniform int thickness;

void main()
{

    // int tt = thickness + 1;
    float R1 = 1.f;
    float R2 = 1.f - (thickness / radius);
    float dist = sqrt(dot(coords, coords));
    if (dist <= R2 || dist >= R1)
        discard;

/*
    float smoother = 0.01f -  (radius  * 0.00003f);
    float sm = smoothstep(R1, R1-smoother, dist);
    float sm2 = smoothstep(R2, R2+smoother, dist);
    float alpha = sm*sm2;
*/
    color = vec4(0.4f, 0.5f, 0.6f, 1.f);
}