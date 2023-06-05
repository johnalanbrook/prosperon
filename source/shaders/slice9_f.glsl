#version 330

in vec2 uv; /* image uv */
in vec4 border; /* uv length of border, normalized to image dimensions; left, bottom, right, top */
in vec2 scale; /* polygon dimensions / texture dimensions */
in vec4 fcolor;

out vec4 color;

uniform sampler2d image;

float map(float value, float min1, float max1, float min2, float max2)
{
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float processAxis(float coord, float texBorder, float winBorder)
{
  if (coord < winBorder)
    return map(coord, 0, winBorder, 0, texBorder);
  if (coord < 1 - winBorder)
    return map(coord, winBorder, 1 - winBorder, texBorder, 1 - texBorder);

  return map(coord, 1 - winBorder, 1, 1 - texBorder, 1);
}

uv9slice(vec2 uv, vec2 s, vec4 b)
{
  vec2 t = clamp((s * uv - b.xy) / (s - b.xy - b.zw), 0.0, 1.0);
  return mix(uv * s, 1.0 - s * (1.0 - uv), t);
}

void main()
{
  uv = uv9slice(uv, scale, border);
  color = fcolor * texture(image, uv);
}
