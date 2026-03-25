#version 430 core
layout (location = 0) out vec4 gAlbedoSpec;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D decalAlbedo;
uniform usampler2D gID;
uniform usampler1D tagMap;

uniform mat4 invModel;
uniform float opacity;
uniform vec4 tintColor;
uniform sampler2D gDepth;
uniform uint allowedTagsMask;

void main()
{
    vec2 screenUV = gl_FragCoord.xy / textureSize(gDepth, 0).xy;
    float depth = texture(gDepth, screenUV).r;
    if (depth == 1.0) discard;

    uint tagValue = texture(gID, screenUV).r;
    uint tagBit = texelFetch(tagMap, int(tagValue), 0).r;
    if ((tagBit & allowedTagsMask) == 0u) discard;

    vec4 clipPos = vec4(screenUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 localPos = invModel * clipPos;
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5;
    vec4 texColor = texture(decalAlbedo, decalUV);
    
    // CUSTOM LOGIC HERE
    vec3 finalColor = texColor.rgb * tintColor.rgb;
    
    gAlbedoSpec = vec4(finalColor, texColor.a * opacity * tintColor.a);
}
