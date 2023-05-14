#version 330 core
layout (location = 0) in vec2 vertex; 
layout (location = 1) in vec3 acolor;
layout (location = 2) in vec2 apos;
layout (location = 3) in float aradius;
//layout (location = 4) in float afill;

out vec2 coords;

out float radius;
out vec3 fcolor;

uniform mat4 proj;

void main()
{
    gl_Position = proj * vec4((vertex * aradius) + apos, 0.0, 1.0);
    coords = vertex.xy;
    fcolor = acolor;
    radius = aradius;
}
