#version 460 core

out vec4 FragColor;



in vec2 TexCoords;



uniform sampler2D screenTexture;

uniform sampler2D depthTexture;

uniform sampler2D historyTexture;



uniform mat4 invViewProj;

uniform mat4 prevViewProj;


uniform bool resetHistory;
uniform float historyFeedback;









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





vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 prevSample)

{

    vec3 p_clip = 0.5 * (aabbMax + aabbMin);

    vec3 e_clip = max(0.5 * (aabbMax - aabbMin), vec3(0.00001));



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

    vec3 color = texture(screenTexture, TexCoords).rgb;

    if (resetHistory)

    {

        FragColor = vec4(color, 1.0);

        return;

    }

    float depth = texture(depthTexture, TexCoords).r;




    vec4 clipPos;

    clipPos.xy = TexCoords * 2.0 - 1.0;

    clipPos.z = depth * 2.0 - 1.0;

    clipPos.w = 1.0;

    

    vec4 worldPos = invViewProj * clipPos;

    if (abs(worldPos.w) < 0.000001)

    {

        FragColor = vec4(color, 1.0);

        return;

    }

    worldPos /= worldPos.w;

    



    vec4 prevClipPos = prevViewProj * worldPos;

    if (abs(prevClipPos.w) < 0.000001)

    {

        FragColor = vec4(color, 1.0);

        return;

    }

    prevClipPos /= prevClipPos.w;

    



    vec2 prevUV = prevClipPos.xy * 0.5 + 0.5;

    

    bool validPrevUV = prevUV.x == prevUV.x && prevUV.y == prevUV.y &&

                       prevUV.x >= 0.0 && prevUV.x <= 1.0 &&

                       prevUV.y >= 0.0 && prevUV.y <= 1.0;


    vec2 velocity = validPrevUV ? (TexCoords - prevUV) : vec2(0.0);

    


    vec2 texSize = vec2(textureSize(screenTexture, 0));

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

    















    

    vec3 history = validPrevUV ? texture(historyTexture, prevUV).rgb : color;

    if (any(isnan(history)) || any(isinf(history)))

    {

        history = color;

    }

    



    history = ClipAABB(colorMin, colorMax, history);

    





    float blendFactor = validPrevUV ? clamp(historyFeedback, 0.0, 0.999) : 0.0;

    



    float velocityLen = length(velocity * texSize);

    float motionFactor = clamp(1.0 - velocityLen * 0.05, 0.8, 0.97);

    blendFactor = blendFactor * motionFactor;

    

    FragColor = vec4(mix(color, history, blendFactor), 1.0);

}
