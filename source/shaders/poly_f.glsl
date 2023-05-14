#version 330 core

out vec4 fcolor;
in vec3 color;
in vec2 uv;

void main()
{
  fcolor = vec4(color,1.0);
}
