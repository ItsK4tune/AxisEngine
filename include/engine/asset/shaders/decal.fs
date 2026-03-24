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
uniform uint allowedTagsMask;

void main()
{
    ivec2 texCoord = ivec2(gl_FragCoord.xy);
    
    vec3 worldPos = texelFetch(gPosition, texCoord, 0).rgb;
    if (dot(worldPos, worldPos) < 0.0001) discard;
    
    vec3 worldNormal = texelFetch(gNormal, texCoord, 0).rgb * 2.0 - 1.0;
    vec4 localPos = invModel * vec4(worldPos, 1.0);
    
    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5)
        discard;

    vec3 localNormal = normalize(mat3(invModel) * worldNormal);
    float cosTheta = abs(localNormal.z); 
    float angleFade = smoothstep(0.05, 0.15, cosTheta);
    if (angleFade < 0.001) discard;

    uint id = texelFetch(gID, texCoord, 0).r;
    uint tagBit = texelFetch(tagMap, int(id & 0xFFFF), 0).r;
    if (allowedTagsMask != 0 && (tagBit & allowedTagsMask) == 0)
        discard;

    vec2 uv = localPos.xy + 0.5;
    vec4 albedoSample = texture(decalAlbedo, uv);
    
    if (albedoSample.a < 0.5) discard; 
    
    float finalAlpha = albedoSample.a * opacity * angleFade * tintColor.a;
    vec3 linearAlbedo = pow(albedoSample.rgb, vec3(2.2)) * tintColor.rgb;
    
    gAlbedoSpec = vec4(linearAlbedo, finalAlpha);
}
