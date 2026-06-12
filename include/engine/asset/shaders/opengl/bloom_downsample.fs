#version 460 core









uniform sampler2D srcTexture;

uniform vec2 srcResolution;

uniform float threshold;



in vec2 TexCoords;

layout (location = 0) out vec3 downsample;



vec3 KarisAverage(vec3 color) {

    float res = 1.0 / (1.0 + max(max(color.r, color.g), color.b));

    return color * res;

}



void main()

{

    vec2 srcTexelSize = 1.0 / srcResolution;

    float x = srcTexelSize.x;

    float y = srcTexelSize.y;



















    vec3 e = texture(srcTexture, vec2(TexCoords.x, TexCoords.y)).rgb;



    vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;

    vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

    vec3 i = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;

    vec3 j = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;



    vec3 a = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;

    vec3 b = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;

    vec3 c = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;

    vec3 h = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;

    vec3 k = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;

    vec3 l = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;

    vec3 m = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

















    downsample = e*0.125;

    downsample += (d+f+i+j)*0.125;

    downsample += (a+c+k+m)*0.03125;

    downsample += (b+g+h+l)*0.0625;





    float brightness = max(max(downsample.r, downsample.g), downsample.b);

    if (brightness < threshold) {

        downsample = vec3(0.0);

    }

}


