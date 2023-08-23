#version 330 core
in vec4 fcolor;

out vec4 color;

void main()
{
  float d = length(gl_PointCoord - vec2(0.5,0.5));
  if (d >= 0.47)
    discard;
    
  color = fcolor;
}
