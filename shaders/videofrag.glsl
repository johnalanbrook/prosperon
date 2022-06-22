#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D texture_y;
uniform sampler2D texture_cb;
uniform sampler2D texture_cr;
uniform vec3 spriteColor;

mat4 rec601 = mat4(
    1.16438,  0.00000,  1.59603, -0.87079,
    1.16438, -0.39176, -0.81297,  0.52959,
    1.16438,  2.01723,  0.00000, -1.08139,
    0, 0, 0, 1
);
    
void main() {
    float y = texture2D(texture_y, TexCoords).r;
    float cb = texture2D(texture_cb, TexCoords).r;
    float cr = texture2D(texture_cr, TexCoords).r;
    color = vec4(y, cb, cr, 1.f) * rec601 * vec4(spriteColor, 1.f);
}