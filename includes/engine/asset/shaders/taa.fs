#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D historyTexture;

uniform mat4 invViewProj;
uniform mat4 prevViewProj;
uniform vec2 jitterOffset; // Current frame jitter (in NDC space, or pixel space if preferred, but usually NDC/View space)

// Adjustable parameters
const float feedbackMin = 0.88; 
const float feedbackMax = 0.97;

vec3 RGBToYCoCg(vec3 rgb)
{
    float Y = dot(rgb, vec3(1, 2, 1));
    float Co = dot(rgb, vec3(2, 0, -2));
    float Cg = dot(rgb, vec3(-1, 2, -1));
    return vec3(Y, Co, Cg);
}

vec3 YCoCgToRGB(vec3 ycocg)
{
    float Y = ycocg.x * 0.25;
    float Co = ycocg.y * 0.25;
    float Cg = ycocg.z * 0.25;
    
    float R = Y + Co - Cg;
    float G = Y + Cg;
    float B = Y - Co - Cg;
    return vec3(R, G, B);
}

// Simple clipping/clamping to reduce ghosting
vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 prevSample)
{
    vec3 p_clip = 0.5 * (aabbMax + aabbMin);
    vec3 e_clip = 0.5 * (aabbMax - aabbMin);

    vec3 v_clip = prevSample - p_clip;
    vec3 v_unit = v_clip / e_clip;
    vec3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return p_clip + v_clip / ma_unit;
    else
        return prevSample;
}

void main()
{
    // 1. Reconstruct world position from depth
    float depth = texture(depthTexture, TexCoords).r;
    
    // If depth is skybox (often 1.0), we might need special handling, but usually velocity is just 0 unless camera moved.
    // However, for TAA we need world pos to reproject.
    
    vec4 clipPos;
    clipPos.xy = TexCoords * 2.0 - 1.0;
    clipPos.z = depth * 2.0 - 1.0;
    clipPos.w = 1.0;
    
    vec4 worldPos = invViewProj * clipPos;
    worldPos /= worldPos.w;
    
    // 2. Reproject to finding previous UV
    vec4 prevClipPos = prevViewProj * worldPos;
    prevClipPos /= prevClipPos.w;
    
    // Convert to UV [0, 1]
    vec2 prevUV = prevClipPos.xy * 0.5 + 0.5;
    
    // Calculate Velocity (optional visualization)
    vec2 velocity = (TexCoords - prevUV);
    
    // 3. Sample
    vec3 color = texture(screenTexture, TexCoords).rgb;
    
    // Access neighborhood for clamping
    vec2 texSize = textureSize(screenTexture, 0);
    vec2 du = vec2(1.0 / texSize.x, 0.0);
    vec2 dv = vec2(0.0, 1.0 / texSize.y);
    
    vec3 cTL = texture(screenTexture, TexCoords - du - dv).rgb;
    vec3 cTC = texture(screenTexture, TexCoords - dv).rgb;
    vec3 cTR = texture(screenTexture, TexCoords + du - dv).rgb;
    vec3 cML = texture(screenTexture, TexCoords - du).rgb;
    vec3 cMR = texture(screenTexture, TexCoords + du).rgb;
    vec3 cBL = texture(screenTexture, TexCoords - du + dv).rgb;
    vec3 cBC = texture(screenTexture, TexCoords + dv).rgb;
    vec3 cBR = texture(screenTexture, TexCoords + du + dv).rgb;
    
    vec3 colorMin = min(color, min(cTL, min(cTC, min(cTR, min(cML, min(cMR, min(cBL, min(cBC, cBR))))))));
    vec3 colorMax = max(color, max(cTL, max(cTC, max(cTR, max(cML, max(cMR, max(cBL, max(cBC, cBR))))))));
    
    // 4. Sample History
    // We should un-jitter previous UV if the history buffer is not jittered. 
    // Usually history buffer is aligned (resolved). 
    // Here we reproject from current (jittered) to previous (jittered).
    // The previous projection matrix passed in should ideally contain the PREVIOUS jitter.
    // OR we track pure camera movement and apply jitter offset manually.
    // For simplicity, let's assume prevViewProj includes prev jitter.
    
    vec3 history = texture(historyTexture, prevUV).rgb;
    
    // Clip history to neighborhood
    history = ClipAABB(colorMin, colorMax, history);
    
    // 5. Blend
    // Detect invalid history (offscreen)
    float blendFactor = 0.95; // High blend = more smoothing, more ghosting
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0)
    {
        blendFactor = 0.0;
    }
    
    // Simple motion detection can reduce blend factor on fast motion to reduce ghosting
    float velocityLen = length(velocity * texSize);
    float motionFactor = clamp(1.0 - velocityLen * 0.05, 0.8, 0.97);
    blendFactor = blendFactor * motionFactor;
    
    FragColor = vec4(mix(color, history, blendFactor), 1.0);
}
