#version 330 core
layout (location = 0) in vec2 vert;
layout (location = 1) in vec2 pos;
layout (location = 2) in vec2 wh;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec2 st;
layout (location = 5) in vec4 vColor;

out vec2 TexCoords;
out vec4 fColor;
out vec2 fst;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(pos + (vert * wh), 0.0, 1.0);
    
    TexCoords = uv + vec2(vert.x*st.x, st.y - vert.y*st.y);
    fst = st / wh;
    
    fColor = vColor;
}
