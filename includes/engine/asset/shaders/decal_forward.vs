#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    // For surface decals in Forward mode, we map the cube's front vertices to a planar quad.
    // GameState.cpp aligns +Z into the wall. So Z=0 is the hit point.
    // We place the decal quad at Z = -0.01 (slightly towards the viewer) to avoid Z-fighting.
    
    TexCoords = aPos.xy + 0.5;
    
    // We only care about the quad part of the cube for the forward mesh rendering.
    // We flatten it to a plane at Z=-0.01.
    vec3 pos = vec3(aPos.x, aPos.y, -0.01);

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
