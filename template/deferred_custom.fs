#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// 1. Standard Samplers
layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;

// 2. Standard UBOs
layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec3 viewPos;
} camera;

layout(std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
} globalData;

// 3. Common Uniforms
uniform float u_Roughness;
uniform float u_Metallic;
uniform vec4 u_BaseColor;
uniform bool debug_noTexture;
uniform uint u_EntityID;
uniform float u_CustomPorts[8];

void main()
{    
    // Output world position
    gPosition = FragPos;
    
    // Encode normal to [0, 1] range
    gNormal = normalize(Normal) * 0.5 + 0.5;
    
    // Default Albedo
    vec3 color = u_BaseColor.rgb;
    if (!debug_noTexture) {
        color *= pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2));
    }
    
    // --- START CUSTOM LOGIC ---
    // Example: change color based on custom port 0
    if (u_CustomPorts[0] > 0.0) {
        color *= vec3(1.0, 0.5, 0.5); 
    }
    // --- END CUSTOM LOGIC ---

    gAlbedoSpec.rgb = color;
    gAlbedoSpec.a = u_Roughness; // Store u_Roughness in alpha
    gEmissive = vec3(0.0); // Output emissive if needed
    gEntityID = u_EntityID;
}
