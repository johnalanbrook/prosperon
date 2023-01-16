#version 330
out vec4 color;

uniform vec3 linecolor;
uniform float alpha;

void main()
{
    color = vec4(linecolor, alpha);
}