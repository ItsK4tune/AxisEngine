#version 460 core

out vec4 FragColor;



in vec2 TexCoords;

in vec3 WorldPos;

in vec3 Normal;





layout (binding = 27) uniform sampler2D splatMap;

layout (binding = 28) uniform sampler2D textureLayer0;

layout (binding = 29) uniform sampler2D textureLayer1;

layout (binding = 30) uniform sampler2D textureLayer2;

layout (binding = 31) uniform sampler2D textureLayer3;





layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;



layout(std140, binding = 21) uniform LightData {

    mat4 lightSpaceMatricesDir[16];

    mat4 lightSpaceMatricesSpot[16];

    int numDirLights;

    int nrPointLights;

    int nrSpotLights;

    int u_ReceiveShadow;

    float farPlanePoint;

    float farPlaneSpot;

} light;



struct DirLight {

    vec3 direction; float shadowIndex;

    vec3 color; float intensity;

    vec3 u_Ambient; float pad1;

    vec3 diffuse; float pad2;

    vec3 u_Specular; float pad3;

};

layout(std430, binding = 23) buffer DirLightBuffer { DirLight dirLights[]; };



uniform float textureScale;

uniform bool debug_noTexture;



void main()

{



    vec3 albedo;

    if (debug_noTexture) {

        albedo = vec3(1.0);

    } else {

        vec4 splat = texture(splatMap, TexCoords);

        vec2 tiledCoords = TexCoords * textureScale;

        vec3 col0 = pow(texture(textureLayer0, tiledCoords).rgb, vec3(2.2));

        vec3 col1 = pow(texture(textureLayer1, tiledCoords).rgb, vec3(2.2));

        vec3 col2 = pow(texture(textureLayer2, tiledCoords).rgb, vec3(2.2));

        vec3 col3 = pow(texture(textureLayer3, tiledCoords).rgb, vec3(2.2));

        

        albedo = col0 * splat.r + 

                 col1 * splat.g + 

                 col2 * splat.b + 

                 col3 * (1.0 - clamp(splat.r + splat.g + splat.b, 0.0, 1.0));

    }

    



    vec3 N = normalize(Normal);

    vec3 V = normalize(camera.viewPos.xyz - WorldPos);

    vec3 Lo = vec3(0.0);



    for(int i = 0; i < light.numDirLights; i++) {

        vec3 L = normalize(-dirLights[i].direction);

        float diff = max(dot(N, L), 0.0);

        vec3 radiance = dirLights[i].color * dirLights[i].intensity;

        Lo += (0.1 * radiance + 0.8 * diff * radiance);

    }

    

    FragColor = vec4(albedo * Lo, 1.0);

}





