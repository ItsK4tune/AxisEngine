#version 430 core

layout (location = 0) out vec4 gAlbedoSpec;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gEmissive;
layout (location = 3) out vec4 gPBRParams;

in vec4 v_ScreenSpace;

uniform sampler2D u_GPosition;
uniform sampler2D u_GNormalTex;
uniform sampler2D u_GDepth;
uniform sampler2D u_DecalAlbedo;
uniform sampler2D u_DecalNormal;

uniform mat4 u_InvModel;
uniform mat4 u_Model;
uniform float u_Opacity;
uniform int u_LightingMode; // 0=NONE (Unlit), 1=LIGHT, 2=LIGHT+SHADOW
uniform vec4 u_TintColor;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_Reflectivity;

void main()
{
    vec2 screenUV = (v_ScreenSpace.xy / v_ScreenSpace.w) * 0.5 + 0.5;
    
    float depth = texture(u_GDepth, screenUV).r;
    if (depth == 1.0) discard;

    vec3 worldPos = texture(u_GPosition, screenUV).rgb;
    vec4 localPos = u_InvModel * vec4(worldPos, 1.0);
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5; 
    vec4 texColor = texture(u_DecalAlbedo, decalUV);
    if (texColor.a < 0.1) discard;

    vec3 linearColor = pow(texColor.rgb, vec3(2.2)) * u_TintColor.rgb;
    float finalAlpha = texColor.a * u_Opacity * u_TintColor.a;

    // Normal handling
    vec3 wallNormal = texture(u_GNormalTex, screenUV).rgb;
    // For now, decals use the wall's normal to match deferred lighting expectation
    // A more advanced version would use u_DecalNormal + TBN, but we prioritize parity first.
    
    // Shadow Masking (Normal.a)
    // 1.0 = Receive Shadow (LightingMode 2), 0.0 = Skip Shadow (LightingMode 1)
    float receiveShadowBit = (u_LightingMode == 2) ? 1.0 : 0.0;
    gNormal = vec4(wallNormal, receiveShadowBit);

    if (u_LightingMode == 0) {
        // NONE (Unlit): Black out Albedo, Use Emissive
        gAlbedoSpec = vec4(0.0, 0.0, 0.0, finalAlpha);
        gEmissive = vec4(linearColor, finalAlpha);
    } else {
        // LIGHT or LIGHT+SHADOW: Use Albedo
        gAlbedoSpec = vec4(linearColor, finalAlpha);
        gEmissive = vec4(0.0, 0.0, 0.0, 0.0);
    }

    // PBR parameters: Decals replace material properties where they are opaque
    // We use finalAlpha as the blend factor for PBR params too.
    // R: Metallic, G: Roughness, B: Reflectivity, A: packed(noProbe + FresnelPower/100)
    gPBRParams = vec4(u_Metallic, u_Roughness, u_Reflectivity, 0.05) * finalAlpha;
}
