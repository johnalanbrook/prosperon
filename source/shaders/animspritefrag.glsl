#version 330
in vec2 TexCoords;
out vec4 color;

uniform sampler2DArray image;
uniform float frame;
uniform vec3 spriteColor;

void main()
{
    color = vec4(spriteColor, 1.f) * texture(image, vec3(TexCoords,frame));
    if (color.a < 0.1)
        discard;
}