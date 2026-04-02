#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
uniform uint entityID;
void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    vec2 check = floor(TexCoords * 8.0);
    float m = mod(check.x + check.y, 2.0);
    gAlbedoSpec = vec4(0.0, 0.0, 0.0, 1.0);
    gEmissive = mix(vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 0.0), m);
    gEntityID = entityID;
}


