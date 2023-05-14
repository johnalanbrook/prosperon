#version 330 core
in vec2 apos;
in vec2 auv;
in vec3 acolor;

out vec3 color;
out vec2 uv;

uniform mat4 proj;

void main()
{
  gl_Position = proj * vec4(apos, 0.0, 1.0);
  color = acolor;
  uv = auv;
}
