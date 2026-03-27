#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 Normal;

void main()
{
    // Use the actual UV attributes from the quad mesh
    TexCoords = aTexCoord;
    
    // Calculate normal in world space
    Normal = normalize(mat3(transpose(inverse(model))) * vec3(0.0, 0.0, 1.0));

    // Apply a tiny z-offset to help with z-fighting
    vec3 localPos = vec3(aPos.x, aPos.y, 0.01);
    gl_Position = projection * view * model * vec4(localPos, 1.0);
}
