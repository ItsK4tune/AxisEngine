#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 tintColor;

// Custom Ports from Engine
uniform float u_CustomPorts[8];

// Global Data from Engine Gate (Binding 2)
layout (std140, binding = 2) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
};

// Camera Data from Engine (Binding 0)
layout (std140, binding = 0) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 result = texColor.rgb * tintColor.rgb;
    
    // logic: 
    // u_CustomPorts[0] == 1.0 -> Enemy (Red Outline)
    // u_CustomPorts[0] == 2.0 -> Ally (Blue Outline)
    // u_CustomPorts[1] == 1.0 -> Selected (White Glow)
    
    vec3 viewDir = normalize(camera.viewPos - FragPos);
    vec3 norm = normalize(Normal);
    float fresnel = 1.0 - max(dot(norm, viewDir), 0.0);
    fresnel = pow(fresnel, 3.0); // Sharpen the edge
    
    vec3 teamColor = vec3(0.0);
    if (u_CustomPorts[0] > 0.5 && u_CustomPorts[0] < 1.5) {
        teamColor = vec3(1.0, 0.0, 0.0); // Enemy: Red
    } else if (u_CustomPorts[0] > 1.5 && u_CustomPorts[0] < 2.5) {
        teamColor = vec3(0.0, 0.5, 1.0); // Ally: Blue
    }
    
    // Application of Fresnel Outline
    result += teamColor * fresnel * 2.0;
    
    // Selection Glow (White Overlay)
    if (u_CustomPorts[1] > 0.5) {
        float pulse = (sin(u_Time * 10.0) + 1.0) * 0.5;
        result = mix(result, vec3(1.0, 1.0, 1.0), 0.3 + pulse * 0.2);
    }
    
    FragColor = vec4(result, texColor.a * tintColor.a);
}
