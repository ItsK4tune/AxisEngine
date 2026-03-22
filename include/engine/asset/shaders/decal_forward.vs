#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;
out vec3 Normal;

void main()
{
    TexCoords = aPos.xy + 0.5;
    
    // The decal quad is aligned to the XY plane of the entity.
    // In our coordinate system for decals, +Z of the entity is the projection direction (into the wall).
    // So the normal of the decal quad (facing out) is -Z in local space.
    Normal = normalize(mat3(transpose(inverse(model))) * vec3(0.0, 0.0, -1.0));

    // Flatten to Z=0.01 to stay on top of surface
    vec3 pos = vec3(aPos.x, aPos.y, 0.01);
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
