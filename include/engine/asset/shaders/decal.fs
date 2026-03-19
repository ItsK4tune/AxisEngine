#version 430 core
layout (location = 0) out vec4 gAlbedoSpec;

uniform sampler2D gPosition;
uniform sampler2D decalAlbedo;
uniform usampler2D gID; // Entity IDs
uniform sampler2D gDepth; // Still needed for some things maybe? Keep if used.

// uniform mat4 invProj; // Removed reconstruction
// uniform mat4 invView; // Removed reconstruction
uniform mat4 invModel; // World to Local

uniform float opacity;
uniform vec2 screenSize;
uniform uint allowedTagsMask; // Bitmask of tags this decal can stick to

// 1D texture mapping EntityID -> TagBitmask
uniform usampler1D tagMap;

void main()
{
    // Use texelFetch instead of texture() to avoid interpolation issues at high resolutions/zoom
    ivec2 texCoord = ivec2(gl_FragCoord.xy);
    vec3 worldPos = texelFetch(gPosition, texCoord, 0).rgb;
    if (dot(worldPos, worldPos) < 0.0001) discard; // Don't project on background
    
    
    // Project into Decal Local Space
    vec4 localPos = invModel * vec4(worldPos, 1.0);
    
    // Clip if outside unit cube (-0.5 to 0.5)
    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5)
        discard;

    // Check Entity ID / Tag filtering
    uint id = texelFetch(gID, texCoord, 0).r;
    uint tagBit = texelFetch(tagMap, int(id & 0xFFFF), 0).r;
    if (allowedTagsMask != 0 && (tagBit & allowedTagsMask) == 0)
        discard;

    // Map localPos (-0.5 to 0.5) to UV (0 to 1)
    vec2 uv = localPos.xy + 0.5;
    
    vec4 albedoSample = texture(decalAlbedo, uv);
    albedoSample.rgb = pow(albedoSample.rgb, vec3(2.2));
    gAlbedoSpec = vec4(albedoSample.rgb, albedoSample.a * opacity);
}
