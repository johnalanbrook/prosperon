#version 330 core
in vec2 TexCoords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

void main()
{
  frag_color = texture(diffuse_texture, TexCoords);
  return;
  vec2 screensize = textureSize(diffuse_texture,0);
  
  vec4 color = texture(diffuse_texture, TexCoords);
  float avg = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
  frag_color = vec4(avg,avg,avg,1.0);
  float lc = screensize.y/2.0;
  float line = TexCoords.y * lc;
  float line_intensity = mod(float(line),2);

  float off = line_intensity * 0.0005;
  vec4 shift = vec4(off,0,0,0);
  vec4 color_shift = vec4(0.001,0,0,0);
  float r = color.r + color_shift.r + shift.r;
  float g = color.g - color_shift.g + shift.g;
  float b = color.b;

  frag_color = vec4(r, g*0.99, b, 1.0) * clamp(line_intensity, 0.85, 1.0);
}
