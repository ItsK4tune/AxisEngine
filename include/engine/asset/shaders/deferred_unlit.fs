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
uniform vec4 u_BaseColor;
uniform uint entityID;
uniform bool u_isWireframe;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal) * 0.5 + 0.5;
    
    vec4 texColor = texture(u_AlbedoMap, TexCoords);
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    
    gAlbedoSpec = vec4(0.0, 0.0, 0.0, 1.0); // No lighting contribution
    gEntityID = entityID;
    gEmissive = texColor.rgb * u_BaseColor.rgb;

    if (u_isWireframe) {
        gAlbedoSpec.rgb = vec3(0.0, 1.0, 0.0);
        gEmissive = vec3(0.0);
    }
}
