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

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 result = texColor.rgb * tintColor.rgb;
    
    // logic: 
    // u_CustomPorts[0] == 1.0 -> Enemy (Red)
    // u_CustomPorts[0] == 2.0 -> Ally (Green)
    
    float pulse = (sin(u_Time * 5.0) + 1.0) * 0.5; // Pulse effect using global u_Time
    
    if (u_CustomPorts[0] > 0.5 && u_CustomPorts[0] < 1.5) {
        // Enemy: Red Pulse
        result = mix(result, vec3(1.0, 0.0, 0.0), 0.5 + pulse * 0.5);
    } else if (u_CustomPorts[0] > 1.5 && u_CustomPorts[0] < 2.5) {
        // Ally: Green Pulse
        result = mix(result, vec3(0.0, 1.0, 0.0), 0.5 + pulse * 0.5);
    }
    
    FragColor = vec4(result, texColor.a * tintColor.a);
}
