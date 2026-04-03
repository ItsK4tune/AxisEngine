#version 430 core
layout (location = 0) out vec4 gAlbedoSpec;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gPBRParams;

uniform sampler2D gPosition;
uniform sampler2D gNormalTex; 
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
    vec2 screenSize = textureSize(gDepth, 0).xy;
    vec2 screenUV = gl_FragCoord.xy / screenSize;
    float depth = texture(gDepth, screenUV).r;
    if (depth == 1.0) discard; 

    uint tagValue = texture(gID, screenUV).r;
    uint tagBit = texelFetch(tagMap, int(tagValue), 0).r;
    if (tagBit != 0u && (tagBit & allowedTagsMask) == 0u) discard;

    vec3 worldPos = texture(gPosition, screenUV).rgb;
    vec4 localPos = invModel * vec4(worldPos, 1.0);
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5;
    vec4 texColor = texture(decalAlbedo, decalUV);
    
    // 1. HARD SHARPENING
    if (texColor.a < 0.1) discard;

    // 2. GAMMA & COLOR
    vec3 linearColor = pow(texColor.rgb, vec3(2.2)) * tintColor.rgb;
    
    // 3. ALPHA OVERDRIVE (BLACK FIX)
    float isDark = 1.0 - clamp(dot(linearColor, vec3(0.333)), 0.0, 1.0);
    
    // Aggressive hardening: Cracks MUST be SOLID (Alpha 1.0)
    // If we blend, the crack will inherit the cube's reflectivity, making it "faded".
    float finalAlpha = texColor.a * opacity * tintColor.a;
    if (isDark > 0.5) finalAlpha = 1.0; 
    
    linearColor *= mix(1.0, 0.7, isDark * 0.7); 
    finalAlpha = clamp(finalAlpha, 0.0, 1.0);

    gAlbedoSpec = vec4(linearColor, finalAlpha);
    
    // 4. NORMAL PASS (Sync with wall)
    gNormal = texture(gNormalTex, screenUV).rgb;
    
    // 5. AMBIENT SUPPRESSION (PBR)
    // Kill reflection (Reflectivity=0) and maximize Roughness (1.0) on the decal
    gPBRParams = vec4(0.0, 1.0, 0.0, 0.0); 
}
