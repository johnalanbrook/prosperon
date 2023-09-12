#version 330 core
in vec2 TexCoords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main()
{
  frag_color = texture(diffuse_texture, TexCoords);
}
