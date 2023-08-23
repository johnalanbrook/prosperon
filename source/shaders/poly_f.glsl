#version 330 core

out vec4 fcolor;
in vec4 color;
in vec2 uv;

void main()
{
  fcolor = color;
}
