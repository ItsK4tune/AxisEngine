#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D u_AlbedoMap;
uniform vec4 u_BaseColor = vec4(1.0);
uniform uint entityID;
uniform bool u_isWireframe;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    mat4 invProjection;
    mat4 invView;
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
    
    // Write to Albedo so lighting/ambient pass can see it
    gAlbedoSpec = vec4(finalColor, 1.0); // Roughness 1.0
    gEntityID = entityID;
    gEmissive = finalColor;

    if (u_isWireframe) {
        gAlbedoSpec.rgb = vec3(0.0, 1.0, 0.0);
        gEmissive = vec3(0.0);
    }
}
