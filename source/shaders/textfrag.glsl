#version 330 core
in vec2 TexCoords;
in vec4 fColor;
in vec2 fst;

out vec4 color;

uniform sampler2D text;

float osize = 1.0;

void main()
{
  float lettera = texture(text,TexCoords).r;
  
  if (lettera <= 0.1f)
{
    vec2 uvpos = TexCoords - fst;
    for (int x = 0; x < 3; x++) {
      for (int y = 0; y < 3; y++) {
        float pa = texture(text, uvpos + (fst*vec2(x,y))).r;
	if (pa > 0.1) {
	  color = vec4(0.0,0.0,0.0, fColor.a);
	  return;
	}
      }
    }

    discard;
  }

//  vec2 lsize = fst / textureSize(text,0).xy;
/*  vec2 uvpos = TexCoords - fst;
  for (int x = 0; x < 3; x++) {
    for (int y = 0; 0 < 3; y++) {
      float pa = texture(text, uvpos + (fst * vec2(x,y))).r;

      if (pa <= 0.1) {
        color = vec4(0.0,0.0,0.0,fColor.a);
	return;
      }
    }
  }
*/
      
  color = vec4(fColor.xyz, fColor.a);
}
