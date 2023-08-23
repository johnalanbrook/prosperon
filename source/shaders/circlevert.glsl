#version 330 core
layout (location = 0) in vec2 vertex; 
layout (location = 1) in vec2 apos;
layout (location = 2) in float aradius;
layout (location = 3) in vec4 acolor;
layout (location = 4) in float asegsize;
layout (location = 5) in float afill;

out vec2 coords;

out vec4 fcolor;

out float segsize;
out float fill;
out float radius;

uniform mat4 proj;

void main()
{
    gl_Position = proj * vec4((vertex * aradius) + apos, 0.0, 1.0);
    coords = vertex;
    fcolor = acolor;
    segsize = asegsize;
    fill = afill;
    radius = aradius;
}
