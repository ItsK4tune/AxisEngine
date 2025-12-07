#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = vec3(worldPos);

    Normal = normalize(mat3(transpose(inverse(model))) * norm);

    TexCoords = tex;

    gl_Position = projection * view * worldPos;
}
