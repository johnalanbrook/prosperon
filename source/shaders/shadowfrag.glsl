#version 330 core
out float frag_color;

void main()
{
//  frag_color = encode_depth(gl_FragCoord.z);
  frag_color = gl_FragCoord.z;
}  
