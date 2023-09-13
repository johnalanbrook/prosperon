#version 330 core
in vec2 TexCoords;

out vec4 frag_color;

uniform sampler2D diffuse_texture;

float[] kernel = float[9](1.0/9.0,1.0/9.0,1.0/9.0,
                          1.0/9.0,1.0/9.0,1.0/9.0,
                          1.0/9.0,1.0/9.0,1.0/9.0);

void main()
{
  frag_color = texture(diffuse_texture, TexCoords);
  return;
  
  vec2 res = vec2(640,360);
  vec2 uv = gl_FragCoord.xy;
  vec2 screen = textureSize(diffuse_texture,0);
  vec3 acc = vec3(0);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      vec2 realRes = uv + vec2(i-1,j-1);
      acc += texture(diffuse_texture, realRes / res).rgb * kernel[i*3+j];
    }
  }
  
  frag_color = vec4(acc,1);
}
