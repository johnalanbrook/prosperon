#version 330 core
layout (location=0) in vec3 a_pos;
layout (location=1) in vec2 a_tex_coords;
layout (location=2) in vec3 a_norm;

out vec2 tex_coords;
out vec3 normal;
out vec3 frag_pos;
out vec4 frag_pos_light;


uniform mat4 vp;
uniform mat4 model;
uniform mat4 proj;
uniform mat4 lsm;

void main() {
  frag_pos = vec3(model * vec4(a_pos, 1.0));
  gl_Position = proj * vp * vec4(frag_pos, 1.0);  
  tex_coords = a_tex_coords;
  normal = mat3(transpose(inverse(model))) * a_norm;
  frag_pos_light = lsm * vec4(frag_pos, 1.0);
}
