#version 330
in vec2 coords;

out vec4 color;

in float radius;
in vec3 fcolor;
in vec2 pos;

void main()
{
  int thickness = 1;
  bool fill = false;

    // int tt = thickness + 1;
    float R1 = 1.f;
//    float R2 = 1.f - (thickness*zoom / radius);
    float R2 = 1.f - (thickness/radius);
    float dist = sqrt(dot(coords, coords));

    if (dist >= R2 && dist <= R1)
        color = vec4(fcolor, 1.f);
    else if (dist < R2)
        color = vec4(fcolor, 0.1f);
    else
        discard;
}
