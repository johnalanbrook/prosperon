#version 330 core
in vec2 TexCoords;
in vec3 fColor;

out vec4 color;

uniform sampler2D text;

void main()
{

//    color = vec4(fColor.xyz, texture(text, TexCoords).r);
    color = vec4(1.f,1.f,1.f, texture(text, TexCoords).r);    
    if (color.a <= 0.1f)
      discard;
      
     
}
