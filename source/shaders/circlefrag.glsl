#version 330
in vec2 coords;

out vec4 color;


in float radius;
in vec4 fcolor;
in vec2 pos;

in float segsize;
in float fill;

#define PI 3.14

void main()
{
  float px = 1/radius;
  float R1 = 1.f;
  float R2 = 1.0 - fill;
  float dist = sqrt(pow(coords.x,2) + pow(coords.y,2));
  color = fcolor;
    
//  if (dist < R2 || dist > R1)
//    discard;

    float sm = 1 - smoothstep(R1-px,R1,dist);
    float sm2 = smoothstep(R2-px,R2,dist);
    float a = sm*sm2;
    color = mix(vec4(0,0,0,0), fcolor, a);
  if (segsize<0)
    return;
    
  float f = atan(coords.y, coords.x) + PI;

  if (mod(f, segsize) < segsize/2)
    discard;
}
