#version 330 core

out vec4 FragColor;

struct Light {
    vec3 direction;
    vec3 color;
};

struct PLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

struct SLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
};

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D diffuseTex;
uniform sampler2D specTex;
uniform float shininess;
uniform Light light;
uniform PLight plight;
uniform SLight slight;

in vec3 Normal;
in vec3 FragPos;

in vec2 TexCoords;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

// void main()
// {             
//     float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
//     FragColor = vec4(vec3(depth), 1.0);
// }
void main()
{
    //vec3 dirDir = normalize(-Directional.direction);

    float ambientStrength = 1.f;
    vec3 ambient = ambientStrength * vec3(texture(diffuseTex, TexCoords));
    
    // vec3 norm = normalize(Normal);

    // vec3 diffuse;

    // Directional
    //vec3 lightDir = normalize(-light.direction);

   

    // Point
    // float distance = length(plight.position - FragPos);
    // float attenuation = 1.f / (plight.constant + plight.linear * distance + plight.quadratic * (distance * distance));
    // vec3 lightDir = normalize(lightPos - FragPos);

    // float diff = max(dot(norm, lightDir), 0.f);
    //diffuse = diff * lightColor * vec3(texture(diffuseTex, TexCoords)) * attenuation;

    // Spot
    // lightDir = normalize(slight.position - FragPos);
    // float theta = dot(lightDir, normalize(-slight.direction));
    // float epsilon = slight.cutOff - slight.outerCutOff;
    // float intensity = clamp((theta - slight.outerCutOff) / epsilon, 0.f, 1.f);
    // diffuse = lightColor * diff * vec3(texture(diffuseTex, TexCoords)) * intensity;

    // float specularStrength = 0.5f;
    // vec3 viewDir = normalize(viewPos - FragPos);
    // vec3 reflectDir = reflect(-lightDir, norm);
    // float spec = pow(max(dot(viewDir, reflectDir), 0.f), 32);
    // vec3 specular = specularStrength * spec * lightColor * texture(specTex, TexCoords).rgb;

    // if(texture(specTex, TexCoords).r < 0.1f)
    //     discard;

    //vec3 result = (ambient + diffuse + specular) * objectColor;
    
    FragColor = vec4(ambient, 1.f);
} 
