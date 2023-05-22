#version 330 core
in vec2 TexCoords;
in vec4 fColor;

out vec4 color;

uniform sampler2D text;

float osize = 1.0;

void main()
{
  float lettera = texture(text,TexCoords).r;
  

//    color = vec4(1.f, 1.f, 1.f, texture(text, TexCoords).r);
  if (lettera <= 0.1f)
    discard;
      
  color = vec4(fColor.xyz, lettera * fColor.a);
}
