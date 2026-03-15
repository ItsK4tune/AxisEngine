#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// 1. Root Level Samplers (Standardized Binding 0-15)
layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2D texture_normal1;
layout (binding = 2) uniform sampler2D texture_metallic1;
layout (binding = 3) uniform sampler2D texture_roughness1;
layout (binding = 4) uniform sampler2D texture_ao1;
layout (binding = 5) uniform sampler2D texture_emissive1;

// 2. UBO Bindings (Standardized Binding 20-25)
layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

layout(std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
} globalData;

// 3. Common Uniforms
uniform float roughness;
uniform float metallic;
uniform float ao;
uniform vec3 emission;
uniform vec4 tintColor;
uniform bool debug_noTexture;
uniform uint entityID;
uniform float u_CustomPorts[8];

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal) * 0.5 + 0.5; // Encode to [0, 1]
    
    vec4 texColor;
    float _roughness;
    if (debug_noTexture) {
        texColor = vec4(1.0);
        _roughness = roughness;
    } else {
        texColor = texture(texture_diffuse1, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
        _roughness = texture(texture_roughness1, TexCoords).r * roughness;
    }
    
    vec3 result = texColor.rgb * tintColor.rgb + emission;
    
    // Team coloring / custom logic
    if (u_CustomPorts[0] > 0.5 && u_CustomPorts[0] < 5.0) {
        if (u_CustomPorts[0] < 1.5) result *= vec3(1.0, 0.2, 0.2); // Enemy: Red
        else if (u_CustomPorts[0] < 2.5) result *= vec3(0.2, 0.5, 1.0); // Ally: Blue
    }
    
    if (u_CustomPorts[1] > 0.5) {
        result += vec3(0.3, 0.3, 0.1); // yellow glow
    }

    gAlbedoSpec.rgb = result;
    gAlbedoSpec.a = _roughness;
    gEntityID = entityID;
}
