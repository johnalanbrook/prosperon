#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

layout (std140) uniform Matrices {
    mat4 projection;
    mat4 view;
};

//out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

//out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

out vec4 FragPosLightSpace;
out mat3 TBN;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform vec3 viewPos;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (boneIds[i] == -1)
            continue;

        if (boneIds[i] >= 4) {
            totalPosition = vec4(aPos, 1.f);
            break;
        }

        vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos, 1.f);
        totalPosition += localPosition * weights[i];
        vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
    }


    FragPos = vec3(model * vec4(aPos, 1.f));
    TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    TBN = transpose(mat3(T, B, N));
    // TangentLightPos = TBN * lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * FragPos;
    
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.f);
    gl_Position = projection * view * model * totalPosition;
}