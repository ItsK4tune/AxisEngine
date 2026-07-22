#version 460 core

layout (location = 0) out vec4 gAlbedoSpec;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gEmissive;
layout (location = 3) out vec4 gPBRParams;

in vec4 v_ScreenSpace;

uniform sampler2D u_GNormalTex;
uniform sampler2D u_GDepth;
uniform usampler2D u_GID;
uniform usampler1D u_TagMap;
uniform sampler2D u_DecalAlbedo;
uniform bool u_HasDecalTexture;
uniform uint u_AllowedTagsMask;

uniform mat4 u_InvModel;
uniform float u_Opacity;
uniform int u_LightingMode; // 0=NONE (Unlit), 1=LIGHT, 2=LIGHT+SHADOW
uniform vec4 u_TintColor;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_Reflectivity;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    vec4 clipPosition = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosition = camera.u_InvProjection * clipPosition;
    float safeW = abs(viewPosition.w) > 0.000001 ? viewPosition.w : 0.000001;
    viewPosition /= safeW;
    return (camera.u_InvView * viewPosition).xyz;
}

void main()
{
    vec2 screenUV = (v_ScreenSpace.xy / v_ScreenSpace.w) * 0.5 + 0.5;
    
    float depth = texture(u_GDepth, screenUV).r;
    if (depth == 1.0) discard;

    uint entityId = texture(u_GID, screenUV).r;
    if (entityId >= uint(textureSize(u_TagMap, 0))) discard;
    uint tagMask = texelFetch(u_TagMap, int(entityId), 0).r;
    if ((tagMask & u_AllowedTagsMask) == 0u) discard;

    vec3 worldPos = ReconstructWorldPosition(screenUV, depth);
    vec4 localPos = u_InvModel * vec4(worldPos, 1.0);
    localPos /= localPos.w;

    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5) discard;

    vec2 decalUV = localPos.xy + 0.5; 
    vec4 texColor = u_HasDecalTexture ? texture(u_DecalAlbedo, decalUV) : vec4(1.0);
    if (texColor.a < 0.1) discard;

    vec3 linearColor = pow(texColor.rgb, vec3(2.2)) * u_TintColor.rgb;
    float finalAlpha = texColor.a * u_Opacity * u_TintColor.a;

    // Normal handling
    vec3 wallNormal = texture(u_GNormalTex, screenUV).rgb;
    // For now, decals use the wall's normal to match deferred lighting expectation
    // A more advanced version would use u_DecalNormal + TBN, but we prioritize parity first.
    
    // All deferred outputs expose the same source alpha so RGB blending is consistent.
    // The render state preserves destination alpha because it contains surface metadata.
    gNormal = vec4(wallNormal, finalAlpha);

    if (u_LightingMode == 0) {
        // NONE (Unlit): Black out Albedo, Use Emissive
        gAlbedoSpec = vec4(0.0, 0.0, 0.0, finalAlpha);
        gEmissive = vec4(linearColor, finalAlpha);
    } else {
        // LIGHT or LIGHT+SHADOW: Use Albedo
        gAlbedoSpec = vec4(linearColor, finalAlpha);
        gEmissive = vec4(0.0, 0.0, 0.0, 0.0);
    }

    // PBR RGB is blended by finalAlpha. Alpha is deliberately not written: it stores
    // packed reflection metadata belonging to the underlying surface.
    gPBRParams = vec4(u_Metallic, u_Roughness, u_Reflectivity, finalAlpha);
}
