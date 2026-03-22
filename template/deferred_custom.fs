#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// 1. Standard Samplers
layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2D texture_normal1;
layout (binding = 2) uniform sampler2D texture_metallic1;
layout (binding = 3) uniform sampler2D texture_roughness1;

// 2. Standard UBOs
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
uniform vec4 tintColor;
uniform bool debug_noTexture;
uniform uint entityID;
uniform float u_CustomPorts[8];

void main()
{    
    // Output world position
    gPosition = FragPos;
    
    // Encode normal to [0, 1] range
    gNormal = normalize(Normal) * 0.5 + 0.5;
    
    // Default Albedo
    vec3 color = tintColor.rgb;
    if (!debug_noTexture) {
        color *= pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    }
    
    // --- START CUSTOM LOGIC ---
    // Example: change color based on custom port 0
    if (u_CustomPorts[0] > 0.0) {
        color *= vec3(1.0, 0.5, 0.5); 
    }
    // --- END CUSTOM LOGIC ---

    gAlbedoSpec.rgb = color;
    gAlbedoSpec.a = roughness; // Store roughness in alpha
    gEntityID = entityID;
}
