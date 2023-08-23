#version 330 core
in vec2 texcoords;
in vec4 fcolor;

out vec4 color;

uniform sampler2D image;

void main()
{
    color = fcolor * texture(image, texcoords);

    if (color.a <= 0.1f)
        discard;
}
