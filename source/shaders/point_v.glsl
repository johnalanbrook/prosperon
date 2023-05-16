#version 330 core

layout (location = 0) in vec2 apos;
layout (location = 1) in vec4 acolor;
layout (location = 2) in float radius;

uniform mat4 proj;

out vec4 fcolor;

void main()
{
  gl_Position = proj * vec4(apos, 0.0, 1.0);
  fcolor = acolor;
  gl_PointSize = radius;
}
