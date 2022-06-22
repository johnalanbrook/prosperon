#version 330 core
out vec4 FragColor;

struct Light {
    vec3 direction;
    vec3 color;
    float strength;
};

struct PLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 color;
    float strength;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float strength;
    float cutoff;
    float outerCutoff;
    float distance;
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in mat3 TBN;

//in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D shadowMap;

uniform Light dirLight;
uniform vec3 viewPos;

vec3 norm;
vec3 viewDir;

#define NR_POINT_LIGHTS 1
uniform PLight pointLights[NR_POINT_LIGHTS];

uniform SpotLight spotLight;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closest Depth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

vec3 CalcDirLight(Light light) {
    vec3 lightDir = TBN * normalize(-light.direction);
    float lightStrength = 3.f;
    vec3 lightColor = vec3(1.f);

    // diffuse shading
    float diff = max(dot(norm, lightDir), 0.f);

    // specular shading
    float specularStrength = 0.5f;
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Use this instead of reflectDir?
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32.f) * specularStrength; // 32.f is "shininess"

    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

    // combine results
    return light.strength * (diff + spec) * (1.f-shadow) * light.color;
}

vec3 CalcPointLight(PLight light) {
    vec3 tbnPos = TBN * light.position;
    vec3 lightDir = normalize(tbnPos - TangentFragPos);

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.f);

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.f), 32.f);

    // Attenuation
    float distance = length(light.position - TangentFragPos);
    float attenuation = 1.f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return light.color * attenuation * light.strength * (diff + spec);
}

vec3 CalcSpotLight(SpotLight light) {
    vec3 tbnPos = TBN * light.position;
    vec3 tbnDir = TBN * light.direction;
    vec3 lightDir = normalize(tbnPos - TangentFragPos);
    
    float diff = max(dot(norm, lightDir), 0.f);

    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.f), 32.f);

    float theta = dot(lightDir, normalize(-tbnDir));
    float epsilon = light.cutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.f, 1.f);

    float distance = length(light.position - FragPos);
    float attenuation = 1.f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return light.color * light.strength * (diff + spec) * intensity * attenuation;

}

void main()
{   
    vec4 model = texture(texture_diffuse1, TexCoords);

    if (model.a <= 0.1f)
        discard;

    vec3 albedo = model.rgb;
    
    norm = texture(texture_normal1, TexCoords).rgb;
    norm = normalize(norm * 2.f - 1.f);
    //vec3 norm = normalize(TangentViewPos);

    viewDir = normalize(TangentViewPos - TangentFragPos);

    // Ambient doesn't change
    float ambientStrength = 0.3f;
    vec3 ambientColor = vec3(1.f);
    vec3 ambient = ambientStrength * ambientColor;

    // Per light calculation
    
    // Directional
    vec3 result =  CalcDirLight(dirLight);
    result += ambient;

    // Point
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i]);

    result += CalcSpotLight(spotLight);


    FragColor = vec4(result * model.rgb, 1.f);
}

