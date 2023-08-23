#version 330 core
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 vColor;

out vec2 texcoords;
out vec4 fcolor;

uniform mat4 proj;

void main()
{
  fcolor = vColor;
  texcoords = uv;
  gl_Position = proj * vec4(vertex, 0.0, 1.0);
}
