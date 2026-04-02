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
    if (depth == 1.0) discard; // 1.0 is background in depth buffer

    uint tagValue = texture(gID, screenUV).r;
    uint tagBit = texelFetch(tagMap, int(tagValue), 0).r;
    if (tagBit != 0u && (tagBit & allowedTagsMask) == 0u) discard;

    vec3 worldPos = texture(gPosition, screenUV).rgb;
    vec4 localPos = invModel * vec4(worldPos, 1.0);
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5;
    vec4 texColor = texture(decalAlbedo, decalUV);
    vec3 linearColor = pow(texColor.rgb, vec3(2.2));
    
    gAlbedoSpec = vec4(linearColor, texColor.a * opacity) * tintColor;
}


