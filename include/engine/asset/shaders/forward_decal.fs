#version 430 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D decalAlbedo;
uniform float opacity;
uniform vec4 tintColor;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    mat4 invProjection;
    mat4 invView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

layout(std140, binding = 21) uniform LightData {
    mat4 lightSpaceMatricesDir[16];
    mat4 lightSpaceMatricesSpot[16];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
} light;

struct DirLight {
    vec3 direction; float shadowIndex;
    vec3 color; float intensity;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct PointLight {
    vec3 position; float shadowIndex;
    vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct SpotLight {
    vec3 position; float pad0;
    vec3 direction; float shadowIndex;
    vec3 color; float intensity;
    float cutOff; float outerCutOff; float constant; float linear;
    float quadratic; float pad2; float pad3; float pad4;
    vec3 ambient; float pad5;
    vec3 diffuse; float pad6;
    vec3 specular; float pad7;
};

layout(std430, binding = 23) buffer DirLightBuffer { DirLight dirLights[]; };
layout(std430, binding = 24) buffer PointLightBuffer { PointLight pointLights[]; };
layout(std430, binding = 25) buffer SpotLightBuffer { SpotLight spotLights[]; };

void main()
{
    vec4 texColor = texture(decalAlbedo, TexCoords);
    if (texColor.a < 0.1) discard;

    vec3 linearColor = pow(texColor.rgb, vec3(2.2)) * tintColor.rgb;
    
    // BLACK FIX - Dark pixels (cracks) boost alpha and darken color
    float isDark = 1.0 - clamp(dot(linearColor, vec3(0.333)), 0.0, 1.0);
    float finalAlpha = mix(texColor.a * opacity * tintColor.a, 1.0, isDark * 0.98);
    linearColor *= mix(1.0, 0.7, isDark * 0.7);
    finalAlpha = clamp(finalAlpha, 0.0, 1.0);

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);

    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < light.numDirLights; ++i) {
        vec3 L = normalize(-dirLights[i].direction);
        float diff = max(dot(N, L), 0.0);
        Lo += dirLights[i].color * dirLights[i].intensity * diff * linearColor;
    }

    for(int i = 0; i < light.nrPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        float dist = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        float diff = max(dot(N, L), 0.0);
        Lo += pointLights[i].color * pointLights[i].intensity * attenuation * diff * linearColor;
    }

    for(int i = 0; i < light.nrSpotLights; ++i) {
        vec3 L = normalize(spotLights[i].position - FragPos);
        float diff = max(dot(N, L), 0.0);
        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float spotIntensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);
        float dist = length(spotLights[i].position - FragPos);
        float attenuation = 1.0 / (spotLights[i].constant + spotLights[i].linear * dist + spotLights[i].quadratic * (dist * dist));
        Lo += spotLights[i].color * spotLights[i].intensity * attenuation * spotIntensity * diff * linearColor;
    }

    // Baseline ambient
    vec3 ambient = linearColor * 0.15; 
    
    FragColor = vec4(ambient + Lo, finalAlpha);
}
