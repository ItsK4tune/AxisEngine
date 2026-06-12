#version 460 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec2 aTexCoords;

layout (location = 2) in vec4 aInstanceColor;

layout (location = 3) in vec3 aInstanceOffset;

layout (location = 4) in float aInstanceScale;



out vec2 TexCoords;

out vec4 ParticleColor;



layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;



void main()

{

    TexCoords = aTexCoords;

    ParticleColor = aInstanceColor;

    



    vec3 CameraRight_worldspace = vec3(camera.u_View[0][0], camera.u_View[1][0], camera.u_View[2][0]);

    vec3 CameraUp_worldspace = vec3(camera.u_View[0][1], camera.u_View[1][1], camera.u_View[2][1]);

    

    vec3 vertexPosition_worldspace = 

        aInstanceOffset

        + CameraRight_worldspace * aPos.x * aInstanceScale

        + CameraUp_worldspace * aPos.y * aInstanceScale;

        

    gl_Position = camera.u_Projection * camera.u_View * vec4(vertexPosition_worldspace, 1.0);



    gl_Position.z -= 0.005 * gl_Position.w;

}






