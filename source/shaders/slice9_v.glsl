#version 330 core

layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 vuv;
layout (location = 2) in vec4 vborder;
layout (location = 3) in vec2 vscale;
layout (location = 4) in vec4 vcolor;

out vec2 uv;
out vec4 border;
out vec2 scale;
out vec4 fcolor;

uniform mat4 projection;

void main()
{
  gl_Position = projection * vec4(vert, 0.0, 1.0);
  
  uv = vuv;
  border = vborder;
  scale = vscale;
  fcolor = vcolor;
}
