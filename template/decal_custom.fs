#version 430 core
layout (location = 0) out vec4 gAlbedoSpec;

uniform sampler2D u_GPosition;
uniform sampler2D u_GNormalTex;
uniform sampler2D u_DecalAlbedo;
uniform usampler2D u_GID;
uniform usampler1D u_TagMap;

uniform mat4 u_InvModel;
uniform vec4 u_BaseColor;
uniform sampler2D u_GDepth;
uniform uint u_AllowedTagsMask;

void main()
{
    vec2 screenUV = gl_FragCoord.xy / textureSize(u_GDepth, 0).xy;
    float depth = texture(u_GDepth, screenUV).r;
    if (depth == 1.0) discard;

    uint tagValue = texture(u_GID, screenUV).r;
    uint tagBit = texelFetch(u_TagMap, int(tagValue), 0).r;
    if ((tagBit & u_AllowedTagsMask) == 0u) discard;

    vec4 clipPos = vec4(screenUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 localPos = u_InvModel * clipPos;
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5;
    vec4 texColor = texture(decalAlbedo, decalUV);
    
    // CUSTOM LOGIC HERE
    vec3 finalColor = texColor.rgb * u_BaseColor.rgb;
    
    gAlbedoSpec = vec4(finalColor, texColor.a * u_BaseColor.a);
}
