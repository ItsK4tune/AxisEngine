#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
layout (location = 5) out vec4 gPBRParams;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D u_AlbedoMap;
uniform vec4 u_BaseColor = vec4(1.0);
uniform uint u_EntityID;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    vec4 texColor = texture(u_AlbedoMap, TexCoords);
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    
    vec3 finalColor = texColor.rgb * u_BaseColor.rgb;
    
    // Write to Albedo so lighting/u_Ambient pass can see it
    gAlbedoSpec = vec4(finalColor, 0.04); // Default FresnelBias (unlit, non-reflective)
    gEntityID = u_EntityID;
    gEmissive = finalColor;

    gPBRParams = vec4(0.0, 1.0, 0.0, 0.05); // Metallic=0, Roughness=1, Reflectivity=0, packed(noProbe + 5/100)
}
